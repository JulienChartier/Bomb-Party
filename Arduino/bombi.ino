#include <SPI.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Constellation.h>

#define MAX_COMPONENT_COUNT 12
#define MAX_COMPONENT_NAME_LENGTH 15
#define MAX_INSTRUCTION_COUNT 20
#define HAS_FLAG(test, flag) ((test) / (flag) & 1)

#define ARRAY_COUNT(ar) (sizeof((ar))/sizeof((ar)[0]))

#define BUTTON_DOWN HIGH
#define BUTTON_UP LOW

#define ALL_MAC_ADDRESSES "FF:FF:FF:FF:FF:FF"

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
char* ssid     = "quartus";
char* password = "settings";
byte mac[6] = {};
char *strMac[18] = {};

/* Create the Constellation connection */
Constellation constellation(wifiClient, "89.156.77.212", 8088, "ArduinoCard", "Bombi", "07284abc978bcdc9cb4713c5292d3900a54107b8");//89.156.77.212

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

GameMode gameMode = GAME_MODE_SOLVE_LIGHT;

char componentCount = 0;

Component allComponents[MAX_COMPONENT_COUNT] = {};
char allComponentNames[MAX_COMPONENT_COUNT][MAX_COMPONENT_NAME_LENGTH] = {};

#define NEW_COMPONENT(type_, name_, pin_, state_) allComponents[componentCount].type = COMPONENT_TYPE_##type_; \
	allComponents[componentCount].pin = pin_;							\
	allComponents[componentCount].state = state_;						\
	strncpy(allComponentNames[componentCount], #name_, MAX_COMPONENT_NAME_LENGTH - 1); \
	++componentCount;													\
                                            
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
		resetBomb(true);
	}
	else if (strcmp(key, "ConfigureBomb") == 0)
	{
		JsonObject& data = json["Data"];
		
		totalTime = data["TimeInMs"].as<long>();
		resetBomb();

		JsonObject& configuration = json["Configuration"];
		JsonArray& instructions = data["Instructions"].asArray();
		int instructionCount = instructions.mesureLength();
		
		for (int i = 0; i < instructionCount; ++i)
		{
			JsonObject& instruction = instructions[i];
			
			pushInstruction(instruction["ComponentIndex"].as<char>(), instruction["ComponentState"].as<char>(),
							instruction["MinDuration"].as<unsigned long>(), instruction["MaxDuration"].as<unsigned long>());
		}
	}
}

void sendBombInfo()
{
	for (int i = 0; i < bomb.componentCount; ++i)
	{
		(*(jsonComponents[i]))["Name"] = allComponentNames[i];
		(*(jsonComponents[i]))["Type"] = (int) bomb.allComponents[i].type;
		(*(jsonComponents[i]))["State"] = (int) bomb.allComponents[i].state;

		jsonAllComponents.add((*(jsonComponents[i])));
	}

	for (int i = 0; i < instructionCount; ++i)
	{
		int index = bomb.allInstructions[i].componentIndex;
	
		(*(jsonInstructions[i]))["ComponentName"] = allComponentNames[index];
		(*(jsonInstructions[i]))["ComponentState"] = (int) bomb.allComponents[index].state; 
		(*(jsonInstructions[i]))["MinDuration"] = bomb.allInstructions[i].minDuration;
		(*(jsonInstructions[i]))["MaxDuration"] = bomb.allInstructions[i].maxDuration;

		jsonAllInstructions.add((*(jsonInstructions[i])));
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
	randomSeed(millis());
	Serial.begin(9600);

	WiFi.macAddress(mac);
	sprintf(strMac, "%02d:%02d:%02d:%02d:%02d:%02d",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	
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

	for (int i = 0; i < MAX_COMPONENT_COUNT; i++)
	{
		jsonComponents[i] = &jsonBuffer.createObject();
	}

	for (int i = 0; i < MAX_INSTRUCTION_COUNT; i++)
	{
		jsonInstruction[i] = &jsonBuffer.createObject();
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

	constellation.setMessageCallback(messageReceived);
	constellation.subscribeToMessage();
	
	lastTick = millis();
}

void loop()
{
	constellation.pollConstellation(400);
	delay(100);
	
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
					for(int i = 0;i<4;i++)
					{
						ComponentState state = bomb.allComponents[i].state;
						
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


