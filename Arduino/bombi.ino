#include <SPI.h>
#include <WiFi.h>

#define MAX_COMPONENT_COUNT 16
#define MAX_COMPONENT_NAME_LENGTH 20
#define MAX_INSTRUCTION_COUNT 42

#define ARRAY_COUNT(ar) (sizeof((ar))/sizeof((ar)[0]))

#define BUTTON_DOWN HIGH
#define BUTTON_UP LOW

// NOTE: Instructions example.
// BUTTON, pin 2, LOW (button pressed)
// BUTTON, pin 2, HIGH (button released)
// BUTTON, pin 3, LOW
// ...

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
  int pin;
  int state;

  unsigned long stateDuration;
} Component;

typedef struct Instruction
{
  int componentIndex;
  int newState;

  unsigned long minDuration;
  unsigned long maxDuration;
} Instruction;

typedef struct Bomb
{
  Component allComponents[MAX_COMPONENT_COUNT]; // Hard coded.
  Instruction allInstructions[MAX_INSTRUCTION_COUNT]; // Given by server.

  long timeLeftInMs;
        
  int componentCount, instructionCount;
  int currentInstruction;

  BombState state;
} Bomb;

unsigned long lastTick;
unsigned long totalTime;

int componentCount = 0;

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

// Could just be an int oldComponentStates[ARRAY_COUNT(allComponents)].
Bomb oldState = Bomb{};

void clearInstructions()
{
    memset(bomb.allInstructions, 0, sizeof(Instruction) * bomb.instructionCount);
    bomb.instructionCount = 0;
    bomb.currentInstruction = 0;
}

int pushInstruction_(struct Instruction instruction)
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

    Serial.println("toto");
    Serial.println(component->state);
                
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
int compareBombToOldState(unsigned long timeDelta)
{
  Instruction instruction = bomb.allInstructions[bomb.currentInstruction];
  Component instructionComponent = bomb.allComponents[instruction.componentIndex];
        
  for (int i = 0; i < bomb.componentCount; ++i)
  {          
    Component component = bomb.allComponents[i];
    bomb.allComponents[i].stateDuration += timeDelta;

    if (instructionComponent.pin == component.pin)
    {
      int minDurationValid = ((instruction.minDuration == 0) ||
            (component.stateDuration >= instruction.minDuration));
      int maxDurationValid = ((instruction.maxDuration == 0) ||
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
  pushInstruction(2, BUTTON_DOWN, 0, 1000);
  pushInstruction(2, BUTTON_UP, 1000);
  
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
      //Serial.println(bomb.timeLeftInMs / 1000.0f);
        
      if (bomb.timeLeftInMs <= 0)
      {
        bomb.timeLeftInMs = 0;
        // explode():
        Serial.println("TIME OUT!!");
        digitalWrite(allComponents[0].pin, HIGH);
        bomb.state = BOMB_STATE_EXPLODED;
      }
        
      int state = compareBombToOldState(delta);

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
          //digitalWrite(allComponents[0].pin, HIGH);
          //bomb.isActivated = false;
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


