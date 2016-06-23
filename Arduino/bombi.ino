#include <SPI.h>
#include <WiFi.h>
#include <Constellation.h>
#include <ArduinoJson.h>

WiFiClient wifiClient;
char* ssid     = "Xion";
char* password = "xionpassword";


/* Create the Constellation connection */
Constellation constellation(wifiClient, "89.156.77.212", 8088, "ArduinoCard", "Bombi", "PUT_ACCESS_KEY_HERE!");

byte mac[6];

#define MAX_COMPONENT_COUNT 12
#define MAX_COMPONENT_NAME_LENGTH 15
#define MAX_INSTRUCTION_COUNT 20

#define ARRAY_COUNT(ar) (sizeof((ar))/sizeof((ar)[0]))

#define BUTTON_DOWN HIGH
#define BUTTON_UP LOW

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

unsigned long lastTick;
unsigned long totalTime;

char gameSerieZero[4] = {1,3,5,0};
char gameSerieOne[4] = {1,9,8,0};
char gameSerieTwo[4] = {0,1,3,0};
char gameSerieThree[4] = {2,12,14,0};

char componentCount = 0;

Component allComponents[MAX_COMPONENT_COUNT] = {};
char allComponentNames[MAX_COMPONENT_COUNT][MAX_COMPONENT_NAME_LENGTH] = {};

#define NEW_COMPONENT(type_, name_, pin_, state_) allComponents[componentCount].type = COMPONENT_TYPE_##type_;\
                                                  allComponents[componentCount].pin = pin_;                   \
                                                  allComponents[componentCount].state = state_;               \
                                                  strncpy(allComponentNames[componentCount], #name_, MAX_COMPONENT_NAME_LENGTH - 1);  \
                                                  ++componentCount;                                                                  \
                                            
#define NEW_LED(name, pin, state) NEW_COMPONENT(LED, name##_LED, pin, state)

#define NEW_BUTTON(name, pin) NEW_COMPONENT(BUTTON, name##_BUTTON, pin, BUTTON_UP)

Bomb bomb = Bomb{};

// Could just be a char oldComponentStates[MAX_COMPONENT_COUNT].
Bomb oldState = Bomb{};

void clearInstructions(char totalClear = false)
{
  if (totalClear)
  {
    memset(bomb.allInstructions, 0, sizeof(Instruction) * bomb.instructionCount);
    bomb.instructionCount = 0;
  }
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
  bomb.state = BOMB_STATE_ACTIVATED;

  bomb.timeLeftInMs = totalTime;
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
          return 1;  
        }
        else
        {
          return -1;
        }
      }
      else
      {
        if (!maxDurationValid)
        {
          return -1;  
        }
      }
    }
    else
    {
      if (component.state != oldState.allComponents[i].state)
      {
        component.stateDuration = 0;
        return -1;
      }
    }
  }

  return 0;
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

void setup()
{
         Serial.begin(9600);
         delay(10);
  
        // Connect to Wifi  
        Serial.print("Connecting to ");
        Serial.println(ssid);  
        WiFi.begin(ssid, password);  
        while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          Serial.print(".");
        }
        Serial.println("WiFi connected. IP: ");
        Serial.println(WiFi.localIP());
        
        WiFi.macAddress(mac);
  
  // Get sentinel name
        Serial.println(constellation.getSentinelName());
        Serial.println("wololo");
        
        constellation.setMessageReceiveCallback(messageReceive);
        constellation.subscribeToMessage();
        
        NEW_LED(RED_0, 11, HIGH);
        NEW_LED(GREEN_0,12,LOW);
        NEW_BUTTON(BUTTON_0, 13);

        NEW_LED(GREEN_1,9,LOW);
        NEW_LED(RED_1,8,HIGH);
        NEW_BUTTON(BUTTON_1,10);

        NEW_LED(RED_2, 5, HIGH);
        NEW_LED(GREEN_2,6,LOW);
        NEW_BUTTON(BUTTON_2, 7);

        NEW_LED(GREEN_3,3,LOW);
        NEW_LED(RED_3,2,HIGH);
        NEW_BUTTON(BUTTON_3,4);
        
        // This should be in loop with a boolean to indicate when to reset...
  // Only the connection to constellation should be here.
        totalTime = 30000;
  resetBomb();

  /*for (int i = 0; i < 10; ++i)
  {
    pushInstruction(1, BUTTON_DOWN);
    pushInstruction(1, BUTTON_UP, 0, 1000);
  }
   */     
  pushInstruction(2, BUTTON_DOWN);
  pushInstruction(2, BUTTON_UP);

  pushInstruction(5, BUTTON_DOWN);
  pushInstruction(5, BUTTON_UP);
  
  lastTick = millis();
}

void loop()
{
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
        bomb.state = BOMB_STATE_EXPLODED;
      }
        
      char state = compareBombToOldState(delta);

      switch (state)
      {
        case 1:
        {
          ++bomb.currentInstruction;

          if (bomb.currentInstruction == bomb.instructionCount)
          {
            // shutdown();
            Serial.println("Success!!");
            bomb.state = BOMB_STATE_DEACTIVATED;
          }
    
          break;
        }

        case -1:
        {
          // explode():
          Serial.println("Wrong!!");
          bomb.state = BOMB_STATE_EXPLODED;
          break;
        }
    
        default:
          break;
      }

      break;
    }
  
    default:
    {
      delay(500);
    
      break;
    }
  }
  
  constellation.pollConstellation(400);
  delay(100);
}

void messageReceive(JsonObject& json)
{
    Serial.print("Message received: ");
    
    const char *macDestination = json["Data"]["MacAddress"].asString();
    
    if (stricmp(macDestination, (const char*) mac) != 0)
    {
      return;  
    }
    
    Serial.print("for me, and it's: ");
    
    const char *key = json["Key"].asString();
    
    Serial.println(key);
    
    if (stricmp(key, "ActivateBomb") == 0)
    {
       bomb.state = BOMB_STATE_ACTIVATED; 
    }
    else if (stricmp(key, "PauseBomb") == 0)
    {
       bomb.state = BOMB_STATE_PAUSED; 
    }
    else if (stricmp(key, "ResetBomb") == 0)
    {
      resetBomb();  
    }
    else if (stricmp(key, "ConfigureBomb") == 0)
    {
      totalTime = json["Data"]["TimeLeftInMs"].as<long>();
      
      JsonArray& instructions = json["Data"]["Instructions"].asArray();
    
      clearInstruction(true);
      
      for (JsonObject::iterator it = instructions.begin(); it != instructions.end(); ++it)
      {
        JsonObject instruction = it->value;
        pushInstruction(instruction["ComponentIndex"].as<char>(), instruction["State"].as<char>(),
                        instruction["MinDuration"].as<unsigned long>(), instruction["MaxDuration"]);
      }
      
      resetBomb();
    }
}