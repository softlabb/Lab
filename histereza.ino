#define pinG1
#define pinG2
#define pinLED
#define pinAiR

struct
{
	boolean G1	=false;
	boolean G2	=false;
	boolean LED	=false;
	boolean AiR	=false;
}module;

void setup()
{
	pinMode(pinG1, OUTPUT);
	pinMode(pinG2, OUTPUT);
	pinMode(pinLED, OUTPUT);
	pinMode(pinAiR, OUTPUT);
	
	digitalWrite(pinG1, LOW);
	digitalWrite(pinG2, LOW);
	digitalWrite(pinLED, LOW);
	digitalWrite(pinAiR, LOW);
	
}

void histereza()
{
	if(T>=Tzad)
	{
		if(module.G1)
		{
			digitalWrite(pinG1, LOW);
			module.G1=false;
		}
	}
	else
	{
		if(T<Tzad-Td)
		{
			digitalWrite(pinG1, HIGH);
			module.G1=true;
			if(T<Tzad-TG2)
			{
				if(konfig.G2)
				{
					digitalWrite(pinG2, HIGH); 
					module.G2=true;
				}
			}
			else
			{
				if(module.G2)
				{
					digitalWrite(pinG2, LOW);
					module.G2=false;
				}
			}
		}
	}
		
}