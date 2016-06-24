#include <SPI.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Constellation.h>

WiFiClient wifiClient;
char* ssid     = "quartus";
char* password = "settings";


/* Create the Constellation connection */
Constellation constellation(wifiClient, "89.156.77.212", 8088, "ArduinoCard", "Bombi", "07284abc978bcdc9cb4713c5292d3900a54107b8");//89.156.77.212

#define MAX_COMPONENT_COUNT 12
#define MAX_COMPONENT_NAME_LENGTH 15
#define MAX_INSTRUCTION_COUNT 20
#define HAS_FLAG(test, flag) ((test) / (flag) & 1)

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


char gameSeries[4][4] = {
    {1,3,5,0},
    {1,9,8,0},
    {0,1,3,0},
    {2,12,14,0},
};

int linkActionToButton[4] = {0,1,2,3}; 

typedef enum GameMode
{
  GAME_MODE_CLASSICAL,
  GAME_MODE_SOLVE_LIGHT,
  
} GameMode;

GameMode gameMode = GAME_MODE_SOLVE_LIGHT;


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

void setComponentState(int index, char state)
{
  bomb.allComponents[index].state = state;
  digitalWrite(bomb.allComponents[index].pin, state);
}

void setup()
{
  randomSeed(millis());
  Serial.begin(9600);

         Serial.begin(9600);  delay(10);
 
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
  
  // Get sentinel name
        Serial.println(constellation.getSentinelName());
        Serial.println("wololo");
        constellation.writeInfo("hello, world!\n");


        // initialisation des boutons et leds
        NEW_BUTTON(BUTTON_0, 13);
        NEW_BUTTON(BUTTON_1,10);        
        NEW_BUTTON(BUTTON_2, 7);
        NEW_BUTTON(BUTTON_3,4);

        NEW_LED(GREEN_0,12,LOW);
        NEW_LED(RED_0, 11, HIGH);
        
        NEW_LED(GREEN_1,9,LOW);
        NEW_LED(RED_1,8,HIGH);
        
        NEW_LED(GREEN_2,6,LOW);
        NEW_LED(RED_2, 5, HIGH);
        
        NEW_LED(GREEN_3,3,LOW);
        NEW_LED(RED_3,2,HIGH);


        //give instruction to button
        for(int i=0;i<10;++i)
        {
          int sheep = random(0,4);
          int beardy = random(0,4);
          int cat = linkActionToButton[sheep];
          linkActionToButton[sheep]=linkActionToButton[beardy];
          linkActionToButton[beardy]=cat;
        }
        
        
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
   /*    
  pushInstruction(2, BUTTON_DOWN);
  pushInstruction(2, BUTTON_UP);

  pushInstruction(5, BUTTON_DOWN);
  pushInstruction(5, BUTTON_UP);
  */
  
  lastTick = millis();
}

void loop()
{
      // onStateObjectReceive...
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
        case GAME_MODE_SOLVE_LIGHT:
        {
          for(int i = 0;i<4;i++){
            if((bomb.allComponents[i].state != oldState.allComponents[i].state) &&
               (bomb.allComponents[i].state == BUTTON_DOWN)){
                char *allLightFlags = gameSeries[linkActionToButton[i]];
              char lightFlag = allLightFlags[allLightFlags[3]];
              for(int j = 0;j<4;j++){
                if(HAS_FLAG(lightFlag, 1 << j )){
                  setComponentState(4+2*j,!bomb.allComponents[4+2*j].state);
                  setComponentState(5+2*j,!bomb.allComponents[5+2*j].state);
                }
              }
              
              allLightFlags[3] = (allLightFlags[3] + 1) % 3;
            }
          }
          boolean gameSolved = true;
          for(int i=4;i<12;i+=2){
            if(bomb.allComponents[i].state==LOW){
              gameSolved = false;
              break;
            }
          }
          if(gameSolved){
            bomb.state = BOMB_STATE_DEACTIVATED;
          }
          break;
        }

        default:
          break;
      }

      break;
    }
  
    default:
    {
      delay(1000);
    
      break;
    }
  }
}


