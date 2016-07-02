#include <SPI.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Constellation.h>

// TODO: Add visual info when bomb exploded/deactivated?

#define MAX_COMPONENT_COUNT 12
#define MAX_COMPONENT_NAME_LENGTH 16
#define MAX_INSTRUCTION_COUNT 50
#define HAS_FLAG(test, flag) ((test) / (flag) & 1)

#define ARRAY_COUNT(ar) (sizeof((ar))/sizeof((ar)[0]))

#define BUTTON_DOWN HIGH
#define BUTTON_UP LOW

#define ALL_MAC_ADDRESSES "FF:FF:FF:FF:FF:FF"

#define PUZZLE 1

typedef enum ComponentType
{
  COMPONENT_TYPE_NONE,
  COMPONENT_TYPE_LED,
  COMPONENT_TYPE_BUTTON,
} ComponentType;

typedef enum BombState
{
  BOMB_STATE_IDLE,
  BOMB_STATE_ACTIVATED,
  BOMB_STATE_PAUSED,
  BOMB_STATE_EXPLODED,
  BOMB_STATE_DEACTIVATED
} BombState;

typedef struct Component
{
  ComponentType type;
  char pin;
  char state;

  unsigned long stateDuration;
} Component;

typedef struct Instruction
{
  char componentIndex;
  char newState;

  unsigned long minDuration;
  unsigned long maxDuration;
} Instruction;

typedef struct Bomb
{
  Component allComponents[MAX_COMPONENT_COUNT]; // Hard coded.
  Instruction allInstructions[MAX_INSTRUCTION_COUNT]; // Given by server.

  long timeLeftInMs;
        
  char componentCount, instructionCount;
  char currentInstruction;

  BombState state;
} Bomb;

typedef enum GameMode
{
  GAME_MODE_CLASSICAL,
  GAME_MODE_SOLVE_LIGHT,  
} GameMode;


WiFiClient wifiClient;
char ssid[] = "Connectify-neko";      //  your network SSID (name)
char* password = "settings";
char *server = "192.168.238.1";
byte mac[6] = {};
char strMac[18] = {};

/* Create the Constellation connection */
Constellation constellation(wifiClient, server, 8088, "ArduinoCard", "Bombi", "630547c1fada14c61e876be55ac877e13f5c03d7");//89.156.77.212 // Outdated comment

StaticJsonBuffer<1024> jsonBuffer;
JsonObject& jsonBombInfoObject = jsonBuffer.createObject();
JsonArray& jsonAllComponents = jsonBuffer.createArray();
JsonArray& jsonAllInstructions = jsonBuffer.createArray();
JsonObject* jsonComponents[MAX_COMPONENT_COUNT] = {};
JsonObject* jsonInstructions[MAX_INSTRUCTION_COUNT] = {};

unsigned long lastTick;
unsigned long totalTime;

char gameSeries[4][4] = {
    {1,3,5,0},
    {1,9,8,0},
    {0,1,3,0},
    {2,12,14,0},
};

int linkActionToButton[4] = {0,1,2,3}; 

GameMode gameMode = GAME_MODE_CLASSICAL;

char componentCount = 0;

Component allComponents[MAX_COMPONENT_COUNT] = {};
char allComponentNames[MAX_COMPONENT_COUNT][MAX_COMPONENT_NAME_LENGTH] = {};

#define NEW_COMPONENT(type_, name_, pin_, state_) allComponents[componentCount].type = COMPONENT_TYPE_##type_; \
  allComponents[componentCount].pin = pin_;             \
  allComponents[componentCount].state = state_;           \
  strncpy(allComponentNames[componentCount], #name_, MAX_COMPONENT_NAME_LENGTH - 1); \
  ++componentCount;                         \
                                            
#define NEW_LED(name, pin, state) NEW_COMPONENT(LED, name##_LED, pin, state)

#define NEW_BUTTON(name, pin) NEW_COMPONENT(BUTTON, name##_BUTTON, pin, BUTTON_UP)

Bomb bomb = Bomb{};

// Could just be a char oldComponentStates[MAX_COMPONENT_COUNT].
Bomb oldState = Bomb{};

void clearInstructions()
{
    memset(bomb.allInstructions, 0, sizeof(Instruction) * bomb.instructionCount);
    bomb.instructionCount = 0;
    bomb.currentInstruction = 0;
}

char pushInstruction_(struct Instruction instruction)
{
  if (bomb.instructionCount < MAX_INSTRUCTION_COUNT)
  {
    bomb.allInstructions[bomb.instructionCount] = instruction;
    ++bomb.instructionCount;

    return 1;
  }

  return 0;
}

#define pushInstruction(componentIndex, state, ...) pushInstruction_(Instruction{componentIndex, state, __VA_ARGS__});
#define PRESS(index, ...) pushInstruction(index, BUTTON_DOWN); \
  pushInstruction(index, BUTTON_UP, __VA_ARGS__);

void initBomb()
{
  Component *bombComponent = bomb.allComponents,
          *component = allComponents;
        
  for (int i = 0; i < componentCount; ++i)
  {
    bombComponent->type = component->type;
    bombComponent->pin = component->pin;
    bombComponent->state = component->state;
    bombComponent->stateDuration = 0;
                
    switch (component->type)
    {
      case COMPONENT_TYPE_LED:
      {
        pinMode(component->pin, OUTPUT);
        digitalWrite(component->pin, component->state);
        break;
      }

      case COMPONENT_TYPE_BUTTON:
      {
        pinMode(component->pin, INPUT);
        break;
      }

      default:
        break;
    }

    ++bombComponent;
    ++component;
  }

  bomb.componentCount = componentCount;

  bomb.timeLeftInMs = totalTime;

    
  // TODO: CHANGE THIS TO BOMB_STATE_IDLE!!!
  bomb.state = BOMB_STATE_ACTIVATED;
}

void resetBomb()
{
    clearInstructions();
    initBomb();
}

// Get differences between current and last state.
// If the differences are not the ones expected from the instruction, BAD MOVE!
// Return -1 if fail, 0 if nothing new, 1 if success.
char compareBombToOldState(unsigned long timeDelta)
{
  Instruction instruction = bomb.allInstructions[bomb.currentInstruction];
  Component instructionComponent = bomb.allComponents[instruction.componentIndex];

  char stateDiff = 0;
  
  for (int i = 0; i < bomb.componentCount; ++i)
  {
    Component component = bomb.allComponents[i];
    bomb.allComponents[i].stateDuration += timeDelta;

    if (instructionComponent.pin == component.pin)
    {
      char minDurationValid = ((instruction.minDuration == 0) ||
                   (component.stateDuration >= instruction.minDuration));
      char maxDurationValid = ((instruction.maxDuration == 0) ||
                   (component.stateDuration <= instruction.maxDuration));
                
      if (component.state != oldState.allComponents[i].state)
      {
        bomb.allComponents[i].stateDuration = 0;
                    
        if ((component.state == instruction.newState) &&
          minDurationValid && maxDurationValid)
        {
          if (stateDiff == 0)
          {
            stateDiff = 1;
          }
        }
        else
        {
          stateDiff = -1;
        }
      }
      else
      {
        if (!maxDurationValid)
        {
          stateDiff = -1;
        }
      }
    }
    else
    {
      if (component.state != oldState.allComponents[i].state)
      {
        component.stateDuration = 0;
        stateDiff = -1;
      }
    }
  }

  return stateDiff;
}

void updateComponentsState()
{
    Component *component = bomb.allComponents;
    
    for(int i = 0; i < bomb.componentCount; ++i)
    {
    component->state = digitalRead(component->pin);
    ++component;
    }  
}

void setComponentState(int index, char state)
{
  bomb.allComponents[index].state = state;
  digitalWrite(bomb.allComponents[index].pin, state);
}

void messageReceived(JsonObject& json)
{
  const char *macDest = json["Data"]["MacAddress"].asString();

  if ((strcmp(macDest, strMac) != 0) &&
    (strcmp(macDest, ALL_MAC_ADDRESSES) != 0))
  {
    return;
  }

  Serial.println("Message received!");

  const char *key = json["Key"].asString();

  if (strcmp(key, "ActivateBomb") == 0)
  {
    bomb.state = BOMB_STATE_ACTIVATED;
  }
  else if (strcmp(key, "PauseBomb") == 0)
  {
    bomb.state = BOMB_STATE_PAUSED;
  }
  else if (strcmp(key, "ResumeBomb") == 0)
  {
    bomb.state = BOMB_STATE_ACTIVATED;
  }
  else if (strcmp(key, "ResetBomb") == 0)
  {
    resetBomb();
  }
  else if (strcmp(key, "ConfigureBomb") == 0)
  {
    JsonObject& data = json["Data"];
    JsonObject& configuration = data["Configuration"];
    
    totalTime = configuration["TimeInMs"].as<long>();
    resetBomb();

    JsonArray& instructions = configuration["Instructions"].asArray();
    int instructionCount = instructions.size();
    
    for (int i = 0; i < instructionCount; ++i)
    {
      JsonObject& instruction = instructions[i];
      
      pushInstruction((char) instruction["ComponentIndex"].as<int>(), (char) instruction["ComponentState"].as<int>(),
                      instruction["MinDuration"].as<unsigned long>(), instruction["MaxDuration"].as<unsigned long>());
    }
  }
}

void sendBombInfo()
{
  int size = bomb.componentCount;
  char isFresh = jsonAllComponents.size() < size;
  
  for (int i = 0; i < size; ++i)
  {
    (*(jsonComponents[i]))["Name"] = allComponentNames[i];
    (*(jsonComponents[i]))["Type"] = (int) bomb.allComponents[i].type;
    (*(jsonComponents[i]))["State"] = (int) bomb.allComponents[i].state;

    if (isFresh)
    {
      jsonAllComponents.add((*(jsonComponents[i])));
    }
    else
    {
      jsonAllComponents.set(i, (*(jsonComponents[i])));
    }
  }

  size = bomb.instructionCount;
  isFresh = jsonAllInstructions.size() < size;
  
  for (int i = 0; i < size; ++i)
  {
    int index = bomb.allInstructions[i].componentIndex;
  
    (*(jsonInstructions[i]))["ComponentIndex"] = index;
    (*(jsonInstructions[i]))["ComponentState"] = (int) bomb.allComponents[index].state; 
    (*(jsonInstructions[i]))["MinDuration"] = bomb.allInstructions[i].minDuration;
    (*(jsonInstructions[i]))["MaxDuration"] = bomb.allInstructions[i].maxDuration;

    if (isFresh)
    {
      jsonAllInstructions.add((*(jsonInstructions[i])));
    }
    else
    {
      jsonAllInstructions.set(i, (*(jsonInstructions[i])));
    }
  }

  jsonBombInfoObject["MacAddress"] = strMac;
  jsonBombInfoObject["TimeInMs"] = bomb.timeLeftInMs;
  jsonBombInfoObject["State"] = (int) bomb.state;
  jsonBombInfoObject["Components"] = jsonAllComponents;
  jsonBombInfoObject["Instructions"] = jsonAllInstructions;

  constellation.pushStateObject("BombInfo", &jsonBombInfoObject);
}

void setup()
{
  randomSeed(analogRead(0));
  Serial.begin(9600);
  //constellation.setDebugMode(true);

  WiFi.macAddress(mac);
  sprintf(strMac, "%02x:%02x:%02x:%02x:%02x:%02x",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  // Connect to Wifi  
  Serial.print("Connecting to ");
  Serial.println(ssid);  
  /*WiFi.begin(ssid, password);  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }*/
  Serial.println("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());
  
  // Get sentinel name
  //Serial.println(constellation.getSentinelName());
  Serial.println("wololo");
  //constellation.writeInfo("hello, world!\n");
  
  // initialisation des boutons et leds
  NEW_BUTTON(BUTTON_0, 13);
  NEW_BUTTON(BUTTON_1,12);        
  NEW_BUTTON(BUTTON_2, 11);
  NEW_BUTTON(BUTTON_3,10);

  NEW_LED(GREEN_0,3,LOW);
  NEW_LED(RED_0, 2, HIGH);
        
  NEW_LED(GREEN_1,5,LOW);
  NEW_LED(RED_1,4,HIGH);
        
  NEW_LED(GREEN_2,7,LOW);
  NEW_LED(RED_2, 6, HIGH);
        
  NEW_LED(GREEN_3,9,LOW);
  NEW_LED(RED_3,8,HIGH);


  //give instruction to button
  for(int i=0;i<10;++i)
  {
    int sheep = random(0,4);
    int beardy = random(0,4);
    int cat = linkActionToButton[sheep];
    linkActionToButton[sheep]=linkActionToButton[beardy];
    linkActionToButton[beardy]=cat;
  }

  for (int i = 0; i < MAX_COMPONENT_COUNT; i++)
  {
    jsonComponents[i] = &jsonBuffer.createObject();
  }

  for (int i = 0; i < MAX_INSTRUCTION_COUNT; i++)
  {
    jsonInstructions[i] = &jsonBuffer.createObject();
  }
        
  // This should be in loop with a boolean to indicate when to reset...
  // Only the connection to constellation should be here.
  totalTime = 900000;
  resetBomb();

  // Digital Root
#if PUZZLE == 1
  PRESS(1);

  
  // ZERO
#elif PUZZLE == 2
  
  PRESS(3);
  PRESS(3);
  
  PRESS(2);
  PRESS(2);
  PRESS(2);

  PRESS(1);

  // Not sure!
  PRESS(0);
  PRESS(0);
  PRESS(0);

  
  // Morse
#elif PUZZLE == 3
  // TODO: Add durations.
#define LONG(index) PRESS(index, 500);
#define SHORT(index) PRESS(index, 0, 500);

  LONG(3);
  SHORT(3);
  LONG(3);
  SHORT(3);

  SHORT(2);
  SHORT(2);
  SHORT(2);
  SHORT(2);

  SHORT(1);
  LONG(1);

  LONG(0);


  // Emperor
#elif PUZZLE == 4
  PRESS(3);
  PRESS(3);

  PRESS(2);

  PRESS(1);

  PRESS(0);
  PRESS(0);
  

  // guess what?
#elif PUZZLE == 5
  PRESS(1);
  PRESS(3);
  PRESS(0);


  // gray
#elif PUZZLE == 6
  pushInstruction(0, BUTTON_DOWN);
  pushInstruction(1, BUTTON_DOWN);
  pushInstruction(0, BUTTON_UP);
  pushInstruction(2, BUTTON_DOWN);
  pushInstruction(0, BUTTON_DOWN);


  // Konami Code
#elif PUZZLE == 7
#define LETTER(index) pushInstruction(index, BUTTON_DOWN);  \
  pushInstruction(index - 1, BUTTON_DOWN);        \
  pushInstruction(index, BUTTON_UP);            \
  pushInstruction(index - 1, BUTTON_UP);
#define UP PRESS(3)
#define DOWN PRESS(2)
#define LEFT PRESS(1)
#define RIGHT PRESS(0)
#define B LETTER(3);
#define A LETTER(1);
  
  UP;
  UP;
  DOWN;
  DOWN;
  LEFT;
  RIGHT;
  LEFT;
  RIGHT;
  B;
  A;
#endif
  
  constellation.setMessageReceiveCallback(messageReceived);
  constellation.subscribeToMessage();
  
  lastTick = millis();
}

void loop()
{
  constellation.pollConstellation(100);
  delay(50);
  
  switch (bomb.state)
  {
    case BOMB_STATE_ACTIVATED:
    {
      oldState = bomb;

      updateComponentsState();
            
      unsigned long currentTime = millis(), delta = currentTime - lastTick;
      bomb.timeLeftInMs -= delta;
      lastTick = currentTime;
            
      //Serial.print("Time left: ");
      Serial.println(bomb.timeLeftInMs / 1000.0f);
            
      if (bomb.timeLeftInMs <= 0)
      {
        bomb.timeLeftInMs = 0;
        // explode():
        Serial.println("TIME OUT!!");
        digitalWrite(allComponents[0].pin, HIGH);

        for (int i = 0; i < componentCount; ++i)
        {
          setComponentState(i, LOW);
        }
            
        bomb.state = BOMB_STATE_EXPLODED;

        sendBombInfo();

        return;
      }
          
      switch (gameMode)
      {
        case GAME_MODE_CLASSICAL:
        {
          char state = compareBombToOldState(delta);
        
          switch (state)
          {
            case 1:
            {
              ++bomb.currentInstruction;
        
              if (bomb.currentInstruction == bomb.instructionCount)
              {
                // shutdown();
                for(int i = 4;i < 12; ++i)
                {
                  setComponentState(i, (i%2 == 0));
                }
                
                
                bomb.state = BOMB_STATE_DEACTIVATED;
              }
            
              break;
            }
        
            case -1:
            {
              // explode():
              for (int i = 0; i < componentCount; ++i)
              {
                setComponentState(i, LOW);
              }
              
              bomb.state = BOMB_STATE_EXPLODED;
              break;
            }
            
            default:
              break;
          }

          break;   
        }
        
        case GAME_MODE_SOLVE_LIGHT:
        {
          for(int i = 0;i<4;i++)
          {
            char state = bomb.allComponents[i].state;
            
            if((state != oldState.allComponents[i].state) &&
               (state == BUTTON_DOWN))
            {
              char *allLightFlags = gameSeries[linkActionToButton[i]];
              char lightFlag = allLightFlags[allLightFlags[3]];
              
              for(int j = 0; j < 4; j++)
              {
                if(HAS_FLAG(lightFlag, 1 << j))
                {
                  setComponentState(4+2*j,!bomb.allComponents[4+2*j].state);
                  setComponentState(5+2*j,!bomb.allComponents[5+2*j].state);
                }
              }
              
              allLightFlags[3] = (allLightFlags[3] + 1) % 3;
            }
          }
          
          boolean gameSolved = true;
          
          for(int i = 4;i < 12; i += 2){
            if(bomb.allComponents[i].state == LOW)
            {
              gameSolved = false;
              break;
            }
          }
          
          if(gameSolved)
          {
            bomb.state = BOMB_STATE_DEACTIVATED;
          }
          
          break;
        }

        default:
          break;
      }

      sendBombInfo();
      
      break;
    }
  
    default:
    {
      delay(500);
    
      break;
    }
  }
}


