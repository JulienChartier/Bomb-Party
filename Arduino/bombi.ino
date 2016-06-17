#define MAX_COMPONENT_COUNT 10
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

        int isActivated;
} Bomb;

unsigned long lastTick; 

Component allComponents[] =
{
	Component{COMPONENT_TYPE_LED, 5, LOW},
	Component{COMPONENT_TYPE_BUTTON, 3, BUTTON_UP},
	//Component{COMPONENT_TYPE_LED, 3, LOW}
};

Bomb bomb = Bomb{};

// Could just be an int oldComponentStates[ARRAY_COUNT(allComponents)].
Bomb oldState = Bomb{};

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
        
	for (int i = 0; i < ARRAY_COUNT(allComponents); ++i)
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

        bomb.componentCount = ARRAY_COUNT(allComponents);
        bomb.isActivated = true;
}

// Get differences between current and last state.
// If the differences are not the ones expected from the instruction, BAD MOVE!
// Return -1 if fail, 0 if nothing new, 1 if success.
int compareBombToOldState(unsigned long timeDelta)
{
        Instruction instruction = bomb.allInstructions[bomb.currentInstruction];
        Component instructionComponent = bomb.allComponents[instruction.componentIndex];
        
        Serial.println(instruction.minDuration);
        
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
        
        // This should be in loop with a boolean to indicate when to reset...
        // Only the connection to constellation should be here.
	initBomb();

        for (int i = 0; i < 10; ++i)
        {
            pushInstruction(1, BUTTON_DOWN);
            pushInstruction(1, BUTTON_UP, 0, 1000);
        }
        
        pushInstruction(1, BUTTON_DOWN, 0, 1000);
        pushInstruction(1, BUTTON_UP, 1000);

        bomb.timeLeftInMs = 30000;
	
        lastTick = millis();
}

void loop()
{
  // Use a State/Status enum instead (Idle, Activated, Paused, Exploded).
  if (bomb.isActivated)
  {
	oldState = bomb;

        updateComponentsState();
        
        unsigned long currentTime = millis(), delta = currentTime - lastTick;
	bomb.timeLeftInMs -= delta;
        lastTick = currentTime;
        
        Serial.print("Time left: ");
        Serial.println(bomb.timeLeftInMs / 1000.0f);
        
        if (bomb.timeLeftInMs <= 0)
        {
          bomb.timeLeftInMs = 0;
          // explode():
          Serial.println("TIME OUT!!");
          digitalWrite(allComponents[0].pin, HIGH);
          bomb.isActivated = false;
        }
        
	int status = compareBombToOldState(delta);

	switch (status)
	{
		case 1:
		{
			++bomb.currentInstruction;

			if (bomb.currentInstruction == bomb.instructionCount)
			{
				// shutdown();
                                Serial.println("Success!!");
                                bomb.isActivated = false;
			}
		
			break;
		}

		case -1:
		{
			// explode():
                        Serial.println("Fail!!");
                        digitalWrite(allComponents[0].pin, HIGH);
                        bomb.isActivated = false;
			break;
		}
		
		default:
			break;
	}
  }
  else
  {
    delay(1000);
  }
}

