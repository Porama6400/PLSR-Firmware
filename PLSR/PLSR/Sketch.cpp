#include <Arduino.h>

#include <LiquidCrystal.h>
#include <Rotary.h>
#include <EEPROM.h>

LiquidCrystal lcd(8, 3, 4, 5, 6, 7);
Rotary rotary(2,9);

bool REenable = true; //Rotary encoder enable
double selectedTime = 10.0;
const double time_minimum = 0.5;
const double time_maximum = 150;

//Ports definition
#define BTN_TOGGLE A0
#define BTN_OK A1
#define BTN_RST A2

#define RELAY A3

#define EEPROM_ADDR_SEL 0 //Location on EEPROM: Store selected time in this location
/*
* Addresses log (Use to track of what address have been use since)
* 19/6/2018 -> 0
*/

//Uncomment line below if in production environment
#define PRODUCTION

const char *verString = "v2.1";
//Prototypes of methods
void renderMainMenu();
void handleRotaryEncoder();

void setup() {
	//Initialize I/O
	pinMode(BTN_RST,INPUT);
	pinMode(BTN_OK,INPUT);
	pinMode(BTN_TOGGLE,INPUT);
	pinMode(RELAY,OUTPUT);
	digitalWrite(RELAY,1);
	
	//Initialize for rotary encoder
	pinMode(9,INPUT);
	pinMode(2,INPUT);
	
	lcd.begin(16, 4);
	EEPROM.begin();
	
	//Splash screen (Production only)
	#ifdef PRODUCTION
	lcd.setCursor(4,0);
	lcd.print(F("PLSR "));
	lcd.print(verString);
	delay(1500);
	lcd.clear();
	#endif
	
	renderMainMenu();
}

void renderMainMenu(){
	REenable = true;
	lcd.clear();
	lcd.setCursor(5,0);
	lcd.print(F("Time:"));
	
	lcd.setCursor(selectedTime < 10 ? 6 : 5,1);
	lcd.print(selectedTime);
}

/************************************************************************/
/* Wait and measure time until logic change to specified logic level    */
/************************************************************************/
int waitFor(int btn,bool logic,int timeout){
	long timeStart = millis();
	while(digitalRead(btn) && (millis() - timeStart) < timeout);
	int output = millis() - timeStart;
	if(output >= timeout) return -1;
	return output;
}
/************************************************************************/
/*                            Rotary Handler                            */
/************************************************************************/


void handleRotaryEncoder(){
	char result = rotary.process();
	if(!result) return;
	bool CW = (result ==DIR_CW);
	if(CW){
		if(selectedTime < 10)	selectedTime += 0.5;
		else selectedTime += 1;
		
		if(selectedTime >= time_maximum) selectedTime = time_maximum;
		
	}
	else{ //CCW
		
		if(selectedTime <= 10)	selectedTime -= 0.5;
		else selectedTime -= 1;
		
		if(selectedTime <= time_minimum) selectedTime = time_minimum;
	}
	renderMainMenu();
}

/************************************************************************/
/*                      Handle OK Button Pressed                        */
/************************************************************************/

void handleButtonOK(){
	if(digitalRead(BTN_OK)){
		//When OK button pressed
		long timeStart = millis();
		REenable = false;
		
		//Turn relay on
		digitalWrite(RELAY,0);
		
		lcd.clear();
		lcd.setCursor(3,0);
		lcd.print(F("Exposing..."));
		
		while((millis() - timeStart) <= (selectedTime * 1000)){
			
			if(digitalRead(BTN_RST)){
				//Check if user tries to cancel
				digitalWrite(RELAY,1);
				lcd.setCursor(3,0);
				lcd.print(F("           "));
				lcd.setCursor(3,0);
				lcd.print(F("Canceled!"));
				lcd.setCursor(1,1);
				lcd.print(F("after "));
				lcd.print((millis() - timeStart) / 1000.0,2);
				lcd.print(F(" sec"));
				delay(500);
				
				//Check if user tries to skip "Canceled" page
				for(int i =0; i < 1500 ;i+=2){
					delay(2);
					if(digitalRead(BTN_RST) || digitalRead(BTN_OK) || digitalRead(BTN_TOGGLE)) break;
				}
				while(digitalRead(BTN_RST) || digitalRead(BTN_OK) || digitalRead(BTN_TOGGLE)); //Wait for all buttons to release
				
				renderMainMenu();
				return;
			}
			else{
				//No special command, continue display exposing status
				long timePassed = (millis() - timeStart);
				if(timePassed % 100 == 0){
					lcd.setCursor(6,1);
					lcd.print(F("          "));
					lcd.setCursor(6,1);
					
					lcd.print((double) (((selectedTime * 1000) - timePassed) / 1000.0), 1);
				}
			}
		}
		digitalWrite(RELAY,1);
		
		lcd.clear();
		lcd.setCursor(5,0);
		lcd.print(F("DONE!"));
		
		delay(1000);
		
		renderMainMenu();
		
	}
}

/************************************************************************/
/*                           Handle Toggle                              */
/************************************************************************/

void handleButtonTOGGLE(){
	if(!digitalRead(BTN_TOGGLE))	return;
	REenable = false; //Disable Rotary encoder
	digitalWrite(RELAY,0);
	lcd.clear();
	lcd.setCursor(2,0);
	lcd.print("...BYPASS...");
	
	while(digitalRead(BTN_TOGGLE));
	while(!(digitalRead(BTN_TOGGLE) || digitalRead(BTN_RST)));
	
	digitalWrite(RELAY,1);
	renderMainMenu();
	
	while(digitalRead(BTN_RST) || digitalRead(BTN_OK) || digitalRead(BTN_TOGGLE)); //Wait for all buttons to release
	
	REenable = true;
	
}

/************************************************************************/
/*                          Handle Reset Button                         */
/************************************************************************/

void handleButtonRESET(){
	if(!digitalRead(BTN_RST)) return;
	selectedTime = 8.0;
	renderMainMenu();
	while(digitalRead(BTN_RST));
}

/************************************************************************/
/*                             Main loop                                */
/************************************************************************/
void loop() {
	handleButtonOK();
	handleButtonTOGGLE();
	handleButtonRESET();
	handleRotaryEncoder();
}