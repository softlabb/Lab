/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Krzysztof Furmaniak <softlab@poczta.onet.pl>
 * Copyright (C) 2018 SotfLab
 * Full contributor list: 
 *
 * Documentation: https://github.com/softlabb/Lab
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Krzysztof Furmaniak
 * 
 * DESCRIPTION
 * This sketch provides an eatching machine with two heater and temp sensors 
 * https://github.com/softlabb/Lab
 */


#if 1

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <TimedAction.h>

#define MINPRESSURE 200
#define MAXPRESSURE 1000

#define STOP		1
#define TRAWIENIE	2
#define MENU		3

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// piny dla Arduino MEGA2560
#define pinG1		53
#define pinG2		51
#define pinLED		49
#define pinAiR		47

MCUFRIEND_kbv tft;

// ALL Touch panels and wiring is DIFFERENT
// copy-paste results from TouchScreen_Calibr_native.ino
//const int XP = 6, XM = A2, YP = A1, YM = 7; //ID=0x9341
const int XP=6, YM=7, YP=A1,XM=A2; //ID=0x9341
//const int TS_LEFT = 907, TS_RT = 136, TS_TOP = 942, TS_BOT = 139;
const int TS_LEFT=270,TS_RT=850,TS_TOP=180,TS_BOT=890;

int pixel_x, pixel_y;	//Touch_getXY() updates global vars
int stan	= STOP;		  // default STAN
int Tzad	= 40;		    // default temperatura zadana, standardowa temperatura roztworu w śrdku B327
float T1   = 0.0f;    // aktualna temperatura z T1
float T2   = 0.0f;    // aktualna temperatura z T2
float T    = 0.0f;    // aktualna temperatura, albo to samo co T1 lub średnia z T1 i T2
float lastT = 0.0f;
int minutaN=0, sekundaN=0, lastsekunda, minutaT=0, sekundaT=0,;
char buf4[4];
bool ss = false, led = false, air = false, menu = false, czasN=true, czasT=false;

struct{
	bool G2          = false;  // czy istnieje grzałka G2
	bool LED         = true;   // czy LED ma być sterowane automatycznie
	bool Buz         = true;   // czy Buzzer ma być sterowany automatycznie
	bool T2          = false;  // czy istnieje termometr T2
	bool AIR         = true;   // czy napowietrzacz ma być sterowany automatycznie
	int TG2             = 5;      // default=5 <0..10> skok co 1 wartość w stopniach C
	int AIRRun          = 4;      // default=4 <2..98> skok co 2 wartość w [sek]
	int AIROf           = 16;     // default=16 <2..98> skok co 2 wartość w [sek]
	float Td            = 0.1f;   // default=0.1 <0.0..5.0> skok co 0.1 wartość w stopniach C HISTEREZA
}konfig;

struct
{
	boolean G1			= false;	// czy grzałka G1 jest ON/OFF
	boolean G2			= false;
	boolean LED			= false;
	boolean AiR			= false;
}module;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 500);

Adafruit_GFX_Button led_btn, air_btn, menu_btn, ss_btn, plus_btn, minus_btn; // buttony z ekranu main_scr
Adafruit_GFX_Button mg2_btn, mtg2plus_btn, mtg2minus_btn, mbuz_btn, mt2_btn, mled_btn, mair_btn, mairrunplus_btn, mairrunminus_btn, mairoffplus_btn, mairoffminus_btn, mtdplus_btn, mtdminus_btn;

void czasISR();
void tempISR();

TimedAction  timerISR =  TimedAction(1000, czasISR);
TimedAction  temperaturaISR =  TimedAction(2000, tempISR);  //pomiar temperatury co 2sek

bool Touch_getXY(void)
{
    TSPoint p = ts.getPoint();
    pinMode(YP, OUTPUT);      //restore shared pins
    pinMode(XM, OUTPUT);
    digitalWrite(YP, HIGH);   //because TFT control pins
    digitalWrite(XM, HIGH);
    bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
    if (pressed) 
	{
        //pixel_x = map(p.x, TS_LEFT, TS_RT, 0, tft.width()); //.kbv makes sense to me
        //pixel_y = map(p.y, TS_TOP, TS_BOT, 0, tft.height());
        pixel_x = map(p.x, TS_BOT, TS_TOP, 0, tft.width());
        pixel_y = map(p.y, TS_LEFT, TS_RT, 0, tft.height());
    }
    return pressed;
}

void czasISR()
{
	if(stan==TRAWIENIE)
	{
		if(czasN)
		{
			sekundaN+=1;
			if(sekundaN>59)
			{
				sekundaN=0;
				minutaN+=1;
			}   			
		}
		if(czasT)
		{
			sekundaT+=1;
			if(sekundaT>59)
			{
				sekundaT=0;
				minutaT+=1;
			}   			
		}
		
	}
}

void tempISR()
{
	// DANE TESTOWE
	// pobierz temp z T1
	T1+=2.0;
	if(konfig.T2)
	{
		// pobierz temp z T2
		T2=24.0;
		T = (T1+T2)/2;
	}
	else
		T = T1;
	
	if(T1>46) T1=26;
}

void main_scr()
{
	tft.fillRect(0,0,200,tft.height()-20,BLACK);

	plus_btn.drawButton(false);
	minus_btn.drawButton(false);

	tft.setTextColor(WHITE);
	tft.setTextSize(2);

	tft.setCursor(15, 2);
	tft.print("G1");
	tft.drawCircle(25, 30, 8, WHITE);

	tft.setCursor(60, 2);
	tft.print("G2");
	tft.drawCircle(70, 30, 8, WHITE);

	tft.setCursor(105, 2);
	tft.print("AiR");
	tft.drawCircle(120, 30, 8, WHITE);

	tft.setCursor(155, 2);
	tft.print("LED");
	tft.drawCircle(170, 30, 8, WHITE);

	//################################################
	
	tft.drawRoundRect(15, 65, 110, 80, 5, WHITE);
	tft.setTextColor(GREEN);
	tft.setCursor(50, 90);
	tft.setTextSize(4);
	//tft.print("40");
	tft.print(Tzad, DEC);

	tft.drawRoundRect(15, 165, 110, 40, 5, WHITE);
	tft.setTextColor(RED);
	tft.setCursor(40, 175);
	tft.setTextSize(3);
	dtostrf(T, 1, 1, buf4);
	tft.print(buf4);

	tft.setTextColor(WHITE);
	tft.setCursor(140, 175);
	tft.setTextSize(1);
	tft.print("T2: ");
	
	if(konfig.T2)
	{
		dtostrf(T2, 1, 1, buf4);
		tft.print(buf4);
	}
	else
		tft.print("-");
    
	tft.setCursor(140, 190);
	tft.print("T1: ");
	dtostrf(T1, 1, 1, buf4);
	tft.print(buf4);
}

void menu_scr()
{
	//ramka
	tft.fillRect(0,0,200,tft.height()-36,BLACK);
	tft.drawRoundRect(0,0,200,tft.height()-35, 4,WHITE);

	//button grzałki G2
	if(konfig.G2)
		mg2_btn.initButton(&tft, 45, 15, 60, 20, WHITE, GREEN, BLACK, "G2 ON", 1);
	else
		mg2_btn.initButton(&tft, 45, 15, 60, 20, WHITE, BLACK, WHITE, "G2 OFF", 1);
	
	tft.setTextColor(WHITE);
	tft.setTextSize(1);
	tft.setCursor(15, 35);
	tft.print("TG2          : ");
	tft.print(konfig.TG2, DEC);
	mtg2plus_btn.initButton(&tft, 150, 40, 20, 20, WHITE, CYAN, BLACK, "+", 1);
	mtg2minus_btn.initButton(&tft, 180, 40, 20, 20, WHITE, CYAN, BLACK, "-", 1);

	if(konfig.LED)
		mled_btn.initButton(&tft, 160, 65, 60, 20, WHITE, GREEN, BLACK, "LED ON", 1);
	else
		mled_btn.initButton(&tft, 160, 65, 60, 20, WHITE, BLACK, WHITE, "LED OFF", 1);
	
	if(konfig.Buz)
		mbuz_btn.initButton(&tft, 45, 65, 60, 20, WHITE, GREEN, BLACK, "Buzz ON", 1);
	else
		mbuz_btn.initButton(&tft, 45, 65, 60, 20, WHITE, BLACK, WHITE, "Buzz OFF", 1);
	
	if(konfig.T2)
		mt2_btn.initButton(&tft, 45, 90, 60, 20, WHITE, GREEN, BLACK, "T2 ON", 1);
	else
		mt2_btn.initButton(&tft, 45, 90, 60, 20, WHITE, BLACK, WHITE, "T2 OFF", 1);

	if(konfig.AIR)
		mair_btn.initButton(&tft, 160, 90, 60, 20, WHITE, GREEN, BLACK, "AiR ON", 1);
	else
		mair_btn.initButton(&tft, 160, 90, 60, 20, WHITE, BLACK, WHITE, "AiR OFF", 1);

	tft.setCursor(15, 115);
	tft.print("AiR Run [sek]: 4");
	mairrunplus_btn.initButton(&tft, 150, 120, 20, 20, WHITE, CYAN, BLACK, "+", 1);
	mairrunminus_btn.initButton(&tft, 180, 120, 20, 20, WHITE, CYAN, BLACK, "-", 1);
	
	tft.setCursor(15, 145);
	tft.print("AiR Off [sek]: 16");
	mairoffplus_btn.initButton(&tft, 150, 150, 20, 20, WHITE, CYAN, BLACK, "+", 1);
	mairoffminus_btn.initButton(&tft, 180, 150, 20, 20, WHITE, CYAN, BLACK, "-", 1);
	
	tft.setCursor(15, 175);
	tft.print("Td           : 0.1");
	mtdplus_btn.initButton(&tft, 150, 180, 20, 20, WHITE, CYAN, BLACK, "+", 1);
	mtdminus_btn.initButton(&tft, 180, 180, 20, 20, WHITE, CYAN, BLACK, "-", 1);

	mled_btn.drawButton(false);
	mg2_btn.drawButton(false);
	mbuz_btn.drawButton(false);
	mt2_btn.drawButton(false);
	mair_btn.drawButton(false);

	mtg2plus_btn.drawButton(false);
	mtg2minus_btn.drawButton(false);

	mairrunminus_btn.drawButton(false);
	mairrunplus_btn.drawButton(false);

	mairoffminus_btn.drawButton(false);
	mairoffplus_btn.drawButton(false);

	mtdminus_btn.drawButton(false);
	mtdplus_btn.drawButton(false);
}

void menu_foot()
{
	if(stan==TRAWIENIE)
	{
		if(sekundaN!=lastsekunda && czasN)
		{
			tft.setTextColor(WHITE);
			tft.setCursor(0, tft.height()-18);
			tft.setTextSize(2);
    
			tft.print("N ");
			tft.print(minutaN, DEC);
			tft.print(":");
			
			if(sekundaN<10)
				tft.print("0");
			tft.print(sekundaN, DEC);				
			
			lastsekunda=sekundaN;
		}	
			
		if(sekundaT!=lastsekunda && czasT)
		{		
			//czas trawienia
			tft.setCursor(10, tft.height()-18);
			tft.print("T ");
			tft.print(minutaT, DEC);
			tft.print(":");
			if(sekundaN<10)
				tft.print("0");
			tft.print(sekundaT, DEC);	
			
			lastsekunda=sekundaT;
		}			
	}
	
	if(stan==STOP)
		tft.fillRect(0, tft.height()-20, tft.width(), 20, BLACK);      
}

void menu_temp()
{
	if(lastT!=T)
	{
		lastT=T;
		tft.fillRoundRect(16, 166, 108, 38, 5, BLACK);
		//tft.drawRoundRect(15, 165, 110, 40, 5, WHITE);
		
		if(T>=Tzad)
			tft.setTextColor(GREEN);
		else
			tft.setTextColor(RED);
    
		tft.setCursor(40, 175);
		tft.setTextSize(3);
		dtostrf(T, 1, 1, buf4);
		tft.print(buf4);

		tft.fillRect(160, 175, 40, 10, BLACK);
		tft.setTextColor(WHITE);
		tft.setCursor(140, 175);
		tft.setTextSize(1);
		tft.print("T2: ");
		
		if(konfig.T2)
		{
			dtostrf(T2, 1, 1, buf4);
			tft.print(buf4);
		}
		else
			tft.print("-");
    
		tft.fillRect(160, 190, 40, 10, BLACK);
		tft.setCursor(140, 190);
		tft.print("T1: ");
		dtostrf(T1, 1, 1, buf4);
		tft.print(buf4);
		// wyswietli nowe wartosci temperatur T1 T2(jak jest) oraz T
	}
}

void histereza()
{
	if(T>=Tzad)
	{
		if(module.G1)
		{
			digitalWrite(pinG1, LOW);
			module.G1=false;
			tft.fillCircle(25, 30, 8, BLACK);
			tft.drawCircle(25, 30, 8, WHITE);
			czasN=false;	//zatrzymujemy czas nagrzewania
			czasT=true;
			// można uruchomić czas zliczający trawienie, zliczanie następuje do naciśnięcia buttona STOP
		}
	}
	else
	{
		if(T<Tzad-Td)
		{
			digitalWrite(pinG1, HIGH);
			module.G1=true;
			tft.fillCircle(25, 30, 8, GREEN);	//G1

			if(T<Tzad-TG2)
			{
				if(konfig.G2)
				{
					digitalWrite(pinG2, HIGH); 
					module.G2=true;
					tft.fillCircle(70, 30, 8, GREEN);	//G2
				}
			}
			else
			{
				if(module.G2)
				{
					digitalWrite(pinG2, LOW);
					module.G2=false;
					tft.fillCircle(70, 30, 8, BLACK);	//G2
					tft.drawCircle(70, 30, 8, WHITE);
				}
			}
		}
	}
		
}

void setup(void)
{
    uint16_t ID = tft.readID();
    
	pinMode(pinG1, OUTPUT);
	pinMode(pinG2, OUTPUT);
	pinMode(pinLED, OUTPUT);
	pinMode(pinAiR, OUTPUT);
	
	digitalWrite(pinG1, LOW);	// sprawdzić jak być powinno czy LOW oznacza wyłączony czy odwrotnie
	digitalWrite(pinG2, LOW);
	digitalWrite(pinLED, LOW);
	digitalWrite(pinAiR, LOW);
	
    if (ID == 0xD3D3) ID = 0x9486; // write-only shield
    tft.begin(ID);
    tft.setRotation(3);            //PORTRAIT
    tft.fillScreen(BLACK);
    
    ss_btn.initButton(&tft, 263, 20, 100, 40, WHITE, CYAN, BLACK, "Start", 2);
    air_btn.initButton(&tft,  263, 85, 100, 40, WHITE, CYAN, BLACK, "AiR", 2);
    led_btn.initButton(&tft, 263, 135, 100, 40, WHITE, CYAN, BLACK, "LED", 2);
    menu_btn.initButton(&tft, 263, 185, 100, 40, WHITE, CYAN, BLACK, "Menu", 2);
    plus_btn.initButton(&tft, 170, 85, 40, 40, WHITE, CYAN, BLACK, "+", 3);
    minus_btn.initButton(&tft, 170, 135, 40, 40, WHITE, CYAN, BLACK, "-", 3);
    
    ss_btn.drawButton(false);
    air_btn.drawButton(false);
    led_btn.drawButton(false);
    menu_btn.drawButton(false);
    plus_btn.drawButton(false);
    minus_btn.drawButton(false);

    main_scr();
}

void loop(void)
{
    bool down = Touch_getXY();

    timerISR.check();
    temperaturaISR.check();
	
    //**********************************************************************************
    // stan STOP / DEFAULT
    //
    if(stan==STOP)
	{
		ss_btn.press(down && ss_btn.contains(pixel_x, pixel_y));
		air_btn.press(down && air_btn.contains(pixel_x, pixel_y));
		led_btn.press(down && led_btn.contains(pixel_x, pixel_y));
		menu_btn.press(down && menu_btn.contains(pixel_x, pixel_y));
		plus_btn.press(down && plus_btn.contains(pixel_x, pixel_y));
		minus_btn.press(down && minus_btn.contains(pixel_x, pixel_y));

		if (ss_btn.justReleased())
		{
			ss_btn.initButton(&tft, 263, 20, 100, 40, WHITE, RED, WHITE, "STOP", 2);
			ss_btn.drawButton();
			stan = TRAWIENIE;
			minutaN=0;
			sekundaN=0;
			minutaT=0;
			sekundaT=0;
			czasN=true;
			czasT=false;
			delay(100);     
		}		

		if (air_btn.justReleased())
		{
			if(air)
			{
				air_btn.initButton(&tft,  263, 85, 100, 40, WHITE, RED, WHITE, "AiR", 2);
				tft.fillCircle(120, 30, 8, GREEN);
				air=false;
				module.AiR=true;
				digitalWrite(pinAiR, HIGH);
			}
			else
			{
				air_btn.initButton(&tft,  263, 85, 100, 40, WHITE, CYAN, BLACK, "AiR", 2);
				tft.fillCircle(120, 30, 8, BLACK);
				tft.drawCircle(120, 30, 8, WHITE);
				air=true;
				module.AiR=false;
				digitalWrite(pinAiR, LOW);
			}
			
			air_btn.drawButton();
			delay(100);
		}

		if (led_btn.justReleased())
		{
			if(led)
			{
				led_btn.initButton(&tft, 263, 135, 100, 40, WHITE, RED, WHITE, "LED", 2);
				tft.fillCircle(170, 30, 8, GREEN);
				led=false;
				module.LED=true;
				digitalWrite(pinLED, HIGH);				
			}
			else
			{
				led_btn.initButton(&tft, 263, 135, 100, 40, WHITE, CYAN, BLACK, "LED", 2);
				tft.fillCircle(170, 30, 8, BLACK);
				tft.drawCircle(170, 30, 8, WHITE);
				led=true;
				module.LED=false;
				digitalWrite(pinLED, LOW);				
			}
			
			led_btn.drawButton();
			delay(100);
		}
    
		if (plus_btn.justReleased())
		{
			Tzad+=1;
			if(Tzad>50) Tzad=50;
			tft.fillRoundRect(16, 66, 108, 78, 5, BLACK);
			tft.setTextColor(GREEN);
			tft.setCursor(50, 90);
			tft.setTextSize(4);
			tft.print(Tzad, DEC);
			delay(100);
		}

		if (minus_btn.justReleased())
		{
			Tzad-=1;
			if(Tzad<30) Tzad=30;
			tft.fillRoundRect(16, 66, 108, 78, 5, BLACK);
			tft.setTextColor(GREEN);
			tft.setCursor(50, 90);
			tft.setTextSize(4);
			//tft.print("40");
			tft.print(Tzad, DEC);
			delay(100);
		}
		
		if (menu_btn.justReleased())
		{
			menu_btn.initButton(&tft, 263, 185, 100, 40, WHITE, RED, WHITE, "Save", 2);
			menu_btn.drawButton();
			delay(100);
			stan=MENU;
			menu=true;
			menu_scr();
		}

		menu_temp();
	}
    
		//**********************************************************************************
		// stan TRAWIENIE
		//	
	if(stan==TRAWIENIE)
	{
		ss_btn.press(down && ss_btn.contains(pixel_x, pixel_y));
		air_btn.press(down && air_btn.contains(pixel_x, pixel_y));
		led_btn.press(down && led_btn.contains(pixel_x, pixel_y));
		plus_btn.press(down && plus_btn.contains(pixel_x, pixel_y));
		minus_btn.press(down && minus_btn.contains(pixel_x, pixel_y));

		if(ss_btn.justReleased())
		{
			ss_btn.initButton(&tft, 263, 20, 100, 40, WHITE, CYAN, BLACK, "START", 2);          
			ss_btn.drawButton();
			delay(100);
			stan=STOP;
			minutaN=0;
			sekundaN=0;
			czasN=true;
			czasT=false;
			
			digitalWrite(pinG1, LOW);
			digitalWrite(pinG2, LOW);
			module.G1=false;
			module.G2=false;
			
			tft.fillCircle(25, 30, 8, BLACK);	//G1
			tft.drawCircle(25, 30, 8, WHITE); 	//G1
			tft.fillCircle(70, 30, 8, BLACK);	//G2
			tft.drawCircle(70, 30, 8, WHITE);	//G2

			//tft.fillRect(0, tft.height()-20, tft.width(), 20, GREEN);
			//ss_btn.drawButton();
		}
      		
		if (air_btn.justReleased())
		{
			if(air)
			{
				air_btn.initButton(&tft,  263, 85, 100, 40, WHITE, RED, WHITE, "AiR", 2);
				tft.fillCircle(120, 30, 8, GREEN);
				air=false;
				module.AiR=true;
				digitalWrite(pinAiR, HIGH);				
			}
			else
			{
				air_btn.initButton(&tft,  263, 85, 100, 40, WHITE, CYAN, BLACK, "AiR", 2);
				tft.fillCircle(120, 30, 8, BLACK);
				tft.drawCircle(120, 30, 8, WHITE);
				air=true;
				module.AiR=false;
				digitalWrite(pinAiR, LOW);				
			}
			air_btn.drawButton();
			delay(100);
		}

		if (led_btn.justReleased())
		{
			if(led)
			{
				led_btn.initButton(&tft, 263, 135, 100, 40, WHITE, RED, WHITE, "LED", 2);
				tft.fillCircle(170, 30, 8, GREEN);
				led=false;
				module.LED=true;
				digitalWrite(pinLED, HIGH);				
			}
			else
			{
				led_btn.initButton(&tft, 263, 135, 100, 40, WHITE, CYAN, BLACK, "LED", 2);
				tft.fillCircle(170, 30, 8, BLACK);
				tft.drawCircle(170, 30, 8, WHITE);
				led=true;
				module.LED=false;
				digitalWrite(pinLED, LOW);				
			}
			led_btn.drawButton();
			delay(100);
		}

		if (plus_btn.justReleased())
		{
			Tzad+=1;
			if(Tzad>50) Tzad=50;
			tft.fillRoundRect(16, 66, 108, 78, 5, BLACK);
			tft.setTextColor(GREEN);
			tft.setCursor(50, 90);
			tft.setTextSize(4);
			tft.print(Tzad, DEC);
			delay(100);
		}

		if (minus_btn.justReleased())
		{
			Tzad-=1;
			if(Tzad<30) Tzad=30;
			tft.fillRoundRect(16, 66, 108, 78, 5, BLACK);
			tft.setTextColor(GREEN);
			tft.setCursor(50, 90);
			tft.setTextSize(4);
			tft.print(Tzad, DEC);
			delay(100);
		}
    
		menu_temp();
		menu_foot();
		histereza();
	}

    //**********************************************************************************
    // stan MENU
    //

    if(stan==MENU)
    {
		ss_btn.press(down && ss_btn.contains(pixel_x, pixel_y));
		menu_btn.press(down && menu_btn.contains(pixel_x, pixel_y));

		mg2_btn.press(down && mg2_btn.contains(pixel_x, pixel_y));
		mled_btn.press(down && mled_btn.contains(pixel_x, pixel_y));
		mbuz_btn.press(down && mbuz_btn.contains(pixel_x, pixel_y));
		mt2_btn.press(down && mt2_btn.contains(pixel_x, pixel_y));
		mair_btn.press(down && mair_btn.contains(pixel_x, pixel_y));
		mtg2plus_btn.press(down && mtg2plus_btn.contains(pixel_x, pixel_y));
		mtg2minus_btn.press(down && mtg2minus_btn.contains(pixel_x, pixel_y));
		mairrunplus_btn.press(down && mairrunplus_btn.contains(pixel_x, pixel_y));
		mairrunminus_btn.press(down && mairrunminus_btn.contains(pixel_x, pixel_y));
		mairoffplus_btn.press(down && mairoffplus_btn.contains(pixel_x, pixel_y));
		mairoffminus_btn.press(down && mairoffminus_btn.contains(pixel_x, pixel_y));
		mtdplus_btn.press(down && mtdplus_btn.contains(pixel_x, pixel_y));
		mtdminus_btn.press(down && mtdminus_btn.contains(pixel_x, pixel_y));

		if (menu_btn.justReleased())
		{
			menu_btn.initButton(&tft, 263, 185, 100, 40, WHITE, CYAN, BLACK, "Menu", 2);  
			menu_btn.drawButton();
			delay(100);
			menu=true;
			stan=STOP; 
			main_scr();       			
			//tft.fillRect(0, tft.height()-20, tft.width(), 20, WHITE);  
		}

		//*****************************************
		// obsługa klawiszy menu
		if (mg2_btn.justReleased())
		{
			if(konfig.G2)
			{
				mg2_btn.initButton(&tft, 45, 15, 60, 20, WHITE, BLACK, WHITE, "G2 OFF", 1);
				konfig.G2=false;
			}
			else
			{
				mg2_btn.initButton(&tft, 45, 15, 60, 20, WHITE, GREEN, BLACK, "G2 ON", 1);
				konfig.G2=true;
			}
			mg2_btn.drawButton();
			delay(100);    
		}

		if (mled_btn.justReleased())
		{
			if(konfig.LED)
			{
				mled_btn.initButton(&tft, 160, 65, 60, 20, WHITE, BLACK, WHITE, "LED OFF", 1);
				konfig.LED=false;          
			}
			else
			{
				mled_btn.initButton(&tft, 160, 65, 60, 20, WHITE, GREEN, BLACK, "LED ON", 1);
				konfig.LED=true;          
			}
			mled_btn.drawButton();  
			delay(100);  
		}

		if (mbuz_btn.justReleased())
		{
			if(konfig.Buz)
			{
				mbuz_btn.initButton(&tft, 45, 65, 60, 20, WHITE, BLACK, WHITE, "Buzz OFF", 1);
				konfig.Buz=false;          
			}
			else
			{
				mbuz_btn.initButton(&tft, 45, 65, 60, 20, WHITE, GREEN, BLACK, "Buzz ON", 1);
				konfig.Buz=true;          
			}
			mbuz_btn.drawButton(); 
			delay(100);   
		}

		if (mt2_btn.justReleased())
		{
			if(konfig.T2)
			{
				mt2_btn.initButton(&tft, 45, 90, 60, 20, WHITE, BLACK, WHITE, "T2 OFF", 1);
				konfig.T2=false;          
			}
			else
			{
				mt2_btn.initButton(&tft, 45, 90, 60, 20, WHITE, GREEN, BLACK, "T2 ON", 1);
				konfig.T2=true;          
			}
			mt2_btn.drawButton(); 
			delay(100);   
		}

		if(mair_btn.justReleased())
		{
			if(konfig.AIR)
			{
				mair_btn.initButton(&tft, 160, 90, 60, 20, WHITE, BLACK, WHITE, "AiR OFF", 1);
				konfig.AIR=false;          
			}
			else
			{
				mair_btn.initButton(&tft, 160, 90, 60, 20, WHITE, GREEN, BLACK, "AiR ON", 1);
				konfig.AIR=true;          
			}
			mair_btn.drawButton();  
			delay(100);  
		}

		if(mtg2plus_btn.justReleased())
		{
			konfig.TG2+=1;
			if(konfig.TG2>10) konfig.TG2=10;
			tft.setTextColor(WHITE);
			tft.fillRect(100,34, 20, 10, BLACK);
			tft.setTextSize(1);
			tft.setCursor(15, 35);
			tft.print("TG2          : ");    
			tft.print(konfig.TG2, DEC);    
			delay(100);
			//tft.fillRoundRect(16, 66, 108, 78, 5, BLACK);        
	        //tft.print(konfig.TG2, DEC);
		}

		if(mtg2minus_btn.justReleased())
		{
			konfig.TG2-=1;
			if(konfig.TG2<0) konfig.TG2=0;
			tft.setTextColor(WHITE);
			tft.fillRect(100,34, 20, 10, BLACK);
			tft.setTextSize(1);
			tft.setCursor(15, 35);
			tft.print("TG2          : ");    
			tft.print(konfig.TG2, DEC);    
			delay(100);
			//tft.fillRoundRect(16, 66, 108, 78, 5, BLACK);
		}

		if(mairrunplus_btn.justReleased())
		{
			konfig.AIRRun+=2;
			if(konfig.AIRRun>98) konfig.AIRRun=98;
			tft.setTextColor(WHITE);
			tft.fillRect(100,113, 20, 10, BLACK);
			tft.setTextSize(1);
			tft.setCursor(15, 115);
			tft.print("AiR Run [sek]: ");    
			tft.print(konfig.AIRRun, DEC);    
			delay(100);
			//tft.fillRoundRect(16, 66, 108, 78, 5, BLACK);        
			//tft.print(konfig.TG2, DEC);
		}

		if(mairrunminus_btn.justReleased())
		{
			konfig.AIRRun-=2;
			if(konfig.AIRRun<2) konfig.AIRRun=2;
			tft.setTextColor(WHITE);
			tft.fillRect(100,113, 20, 10, BLACK);
			tft.setTextSize(1);
			tft.setCursor(15, 115);
			tft.print("AiR Run [sek]: ");    
			tft.print(konfig.AIRRun, DEC);    
			delay(100);
			//tft.fillRoundRect(16, 66, 108, 78, 5, BLACK);
		}

		if(mairoffplus_btn.justReleased())
		{
			konfig.AIROf+=2;
			if(konfig.AIROf>98) konfig.AIROf=98;
			tft.setTextColor(WHITE);
			tft.fillRect(100, 143, 20, 10, BLACK);
			tft.setTextSize(1);
			tft.setCursor(15, 145);
			tft.print("AiR Off [sek]: ");
			tft.print(konfig.AIROf, DEC);
			delay(100);
			//tft.fillRoundRect(16, 66, 108, 78, 5, BLACK);        
			//tft.print(konfig.TG2, DEC);
		}

		if(mairoffminus_btn.justReleased())
		{
			konfig.AIROf-=2;
			if(konfig.AIROf<2) konfig.AIROf=2;
			tft.setTextColor(WHITE);
			tft.fillRect(100, 143, 20, 10, BLACK);
			tft.setTextSize(1);
			tft.setCursor(15, 145);
			tft.print("AiR Off [sek]: ");
			tft.print(konfig.AIROf, DEC);
			delay(100);
			//tft.fillRoundRect(16, 66, 108, 78, 5, BLACK);
		}

		if(mtdplus_btn.justReleased())
		{
			konfig.Td+=0.1;
			if(konfig.Td>5) konfig.Td=5;
			dtostrf(konfig.Td, 1, 1, buf4);
			tft.setTextColor(WHITE);
			tft.fillRect(100, 173, 25, 10, BLACK);
			tft.setTextSize(1);
			tft.setCursor(15, 175);
			tft.print("Td           : ");    
			tft.print(buf4);    
			delay(100);
			//tft.fillRoundRect(16, 66, 108, 78, 5, BLACK);        
			//tft.print(konfig.TG2, DEC);
		}

		if(mtdminus_btn.justReleased())
		{
			konfig.Td-=0.1;
			if(konfig.Td<0.1) konfig.Td=0.1;
			dtostrf(konfig.Td, 1, 1, buf4);
			tft.setTextColor(WHITE);
			tft.fillRect(100, 173, 25, 10, BLACK);
			tft.setTextSize(1);
			tft.setCursor(15, 175);
			tft.print("Td           : ");    
			tft.print(buf4);    
			delay(100);
			//tft.fillRoundRect(16, 66, 108, 78, 5, BLACK);
		}
      // obsługa klawiszy menu - END
      //*****************************************
    }
}

#endif


