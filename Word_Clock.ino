/*
 * Angie Word Clock
 *
 * This clock is built to display spelled out words depending on the time of day.
 * The code depends on both an RTC and Addressable RGB LEDs
 *
 * RTC Chip used: DS1307 and connect via I2C interface (pins SCL & SDA)
 * RGB LEDs used: WS2812B LEDs on a 5v strip connected to pin 6
 *
 * To set the RTC for the first time you have to send a string consisting of
 * the letter T followed by ten digit time (as seconds since Jan 1 1970) Also known as EPOCH time.
 *
 * You can send the text "T1357041600" on the next line using Serial Monitor to set the clock to noon Jan 1 2013
 * Or you can use the following command via linux terminal to set the clock to the current time (UTC time zone)
 * date +T%s > /dev/ttyACM0
 * Inside the processSyncMessage() function I'm offsetting the UTC time with Central time.
 * If you want the clock to be accurate for your time zone, you may need to update the value.
 */

#include <Adafruit_NeoPixel.h>
#include <Time.h>
#include <Wire.h>
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t

#define RGBLEDPIN    6
#define FWDButtonPIN 8
#define REVButtonPIN 9
#define LED13PIN     13

#define N_LEDS 114 // 10x11 + 4
#define TIME_HEADER  "T"   // Header tag for serial time sync message
#define BRIGHTNESSDAY 200 // full on
#define BRIGHTNESSNIGHT 75 // half on

Adafruit_NeoPixel grid = Adafruit_NeoPixel(N_LEDS, RGBLEDPIN, NEO_GRB + NEO_KHZ800);

// a few vars to keep the peace;
int intBrightness = BRIGHTNESSDAY; // the brightness of the clock (0 = off and 255 = 100%)
int intTestMode; // set when both buttons are held down
String strTime = ""; // used to detect if word time has changed
int intTimeUpdated = 0; // used to tell if we need to turn on the brightness again when words change
int timeSetting = 0; // used to know if fwd - back button is pressed
String strCurrentTime; // build the current time
int prev_min = 0;

// a few colors
uint32_t colorWhite = grid.Color(255, 255, 255);
uint32_t colorBlack = grid.Color(0, 0, 0);
uint32_t colorOrange = grid.Color(255, 165, 0);
uint32_t colorHotPink = grid.Color(255, 105, 180);
uint32_t colorPurple = grid.Color(160, 32, 240);
uint32_t colorGold = grid.Color(255, 215, 0);
uint32_t colorCyan = grid.Color(0, 255, 255);
uint32_t colorDimGray = grid.Color(105, 105, 105);
uint32_t colorBisque = grid.Color(255, 228, 196);
uint32_t colorChocolate = grid.Color(210, 105, 30);
uint32_t colorRed = grid.Color(255, 0, 0);
uint32_t colorGreen = grid.Color(0, 255, 0);
uint32_t colorBlue = grid.Color(0, 0, 255);
uint32_t colorJGreen = grid.Color(50, 179, 30);

// the words
//int arrI= {59,48,37,26,15,-1};
//int arrL= {15,16,17,28,39,50,61,-1};
//int arrO= {59,48,37,26,15,16,17,18,60,61,62,29,40,51,-1};
//int arrV= {59,63,48,52,37,41,27,29,17,-1};
//int arrE= {15,16,17,37,38,39,59,60,61,28,40,-1};
//int arrY= {59,63,49,51,39,28,17,-1};
//int arrU= {15,16,17,18,26,29,37,40,48,51,59,62,-1};

int arrANGIE[] = {109, 110, 111, 112, 113, 96, 95, 94, 93, 92, -1};
int arrMONE[] = {3, -1};
int arrMTWO[] = {3, 2, -1};
int arrMTHREE[] = {3, 2, 1, -1};
int arrMFOUR[] = {3, 2, 1, 0, -1};
int arrIT[] = {103, 104, -1};
int arrIS[] = {106, 107, -1};
int arrTWENTY[] = {102, 101, 100, 99, 98, 97, -1};
int arrQUARTER[] = {81, 82, 83, 84, 85, 86, 87, -1};
int arrMTEN[] = {88, 89, 90, -1};
int arrHALF[] = {80, 79, 78, 77, -1};
int arrMFIVE[] = {75, 74, 73, 72, -1};
int arrTO[] = {59, 60, -1};
int arrPAST[] = {61, 62, 63, 64, -1};
int arrONE[] = {58, 57, 56, -1};
int arrTWO[] = {45, 46, 47, -1};
int arrTHREE[] = {48, 49, 50, 51, 52, -1};
int arrFOUR[] = {37, 38, 39, 40, -1};
int arrFIVE[] = {41, 42, 43, 44, -1};
int arrSIX[] = {53, 54, 55, -1};
int arrSEVEN[] = {15, 16, 17, 18, 19, -1};
int arrEIGHT[] = {32, 33, 34, 35, 36, -1};
int arrNINE[] = {66, 67, 68, 69, -1};
int arrTEN[] = {14, 13, 12, -1};
int arrELEVEN[] = {31, 30, 29, 28, 27, 26, -1};
int arrTWELVE[] = {20, 21, 22, 23, 24, 25, -1};
int arrOCLOCK[] = {4, 5, 6, 7, 8, 9, -1};
int arrHEART[] = {89, 88, 84, 83,
                  79, 78, 77, 76, 74, 73, 72, 71,
                  68, 67, 66, 65, 64, 63, 62, 61, 60,
                  57, 56, 55, 54, 53, 52, 51, 50, 49,
                  45, 44, 43, 42, 41, 40, 39,
                  33, 32, 31, 30, 29,
                  21, 20, 19,
                  9, -1
                 };

//TODO button to display an effect
//effects: heart (done), rainbow ANGIE (random colors)
//center to bounds color spread, snake hunt,
//I LOVE YOU passing message
//TODO button to change letter's color
//TODO button to adjust brightness

void setup()
{
	// set up the debuging serial output
	Serial.begin(9600);
//  while(!Serial); // Needed for Leonardo only
	delay(200);
	setSyncProvider(RTC.get);   // the function to get the time from the RTC
	setSyncInterval(30); // sync the time every 30 seconds (0.5 minutes)
	if(timeStatus() != timeSet)
	{
		Serial.println("Unable to sync with the RTC");
		RTC.set(1406278800);   // set the RTC to Jul 25 2014 9:00 am
		setTime(1406278800);
	}
	else
	{
		Serial.println("RTC has set the system time");
	}
	// print the version of code to the console
	printVersion();

	// setup the LED strip
	grid.begin();
	grid.show();

	// set the bright ness of the strip
	grid.setBrightness(intBrightness);

	colorWipe(colorBlack, 0);
	displayHeart(1);
	colorWipe(colorBlack, 0);

	paintWord(arrANGIE, colorJGreen);
	grid.show();

	fadeIn(5);
	colorWipe(colorBlack, 0);

	// set the bright ness of the strip
	grid.setBrightness(intBrightness);

	// initialize the onboard led on pin 13
	pinMode(LED13PIN, OUTPUT);

	// initialize the buttons
	pinMode(FWDButtonPIN, INPUT);
	pinMode(REVButtonPIN, INPUT);

	// lets kick off the clock
	digitalClockDisplay();
}

void loop()
{

	timeSetting = 0;
	// if there is a serial connection lets see if we need to set the time
	if (Serial.available())
	{
		time_t t = processSyncMessage();
		if (t != 0)
		{
			Serial.print("Time set via connection to: ");
			Serial.print(t);
			Serial.println();
			RTC.set(t);   // set the RTC and the system time to the received value
			setTime(t);
		}
	}
	// check to see if the time has been set
	if (timeStatus() == timeSet)
	{
		// time is set lets show the time
		if((hour() < 7) | (hour() >= 19))
		{
			intBrightness =  BRIGHTNESSNIGHT;
		}
		else
		{
			intBrightness =  BRIGHTNESSDAY;
		}
		grid.setBrightness(intBrightness);

		// test to see if both buttons are being held down
		// if so  - start a self test till both buttons are held down again.
		if((digitalRead(FWDButtonPIN) == HIGH) && (digitalRead(REVButtonPIN) == HIGH))
		{
			intTestMode = !intTestMode;
			if(intTestMode)
			{
				Serial.println("Selftest Mode TRUE");
				// run through a quick test
				test_grid();
			}
			else
			{
				Serial.println("Selftest mode FALSE");
			}
		}
		// test to see if a forward button is being held down for time setting
		if(digitalRead(FWDButtonPIN) == HIGH)
		{
			digitalWrite(LED13PIN, HIGH);
			timeSetting = 1;
			Serial.println("Forward Button Down");
			incrementTime(60);
			delay(100);
			digitalWrite(LED13PIN, LOW);
		}

		// test to see if the back button is being held down for time setting
		if(digitalRead(REVButtonPIN) == HIGH)
		{
			digitalWrite(LED13PIN, HIGH);
			timeSetting = 1;
			Serial.println("Backwards Button Down");
			incrementTime(-60);
			delay(100);
			digitalWrite(LED13PIN, LOW);
		}

		// and finaly we display the time (provided we are not in self tes mode
		if(!intTestMode)
		{
			displayTime();
		}
	}
	else
	{
		colorWipe(colorBlack, 0);
		paintWord(arrONE, colorRed);
		grid.show();
		Serial.println("The time has not been set.  Please run the Time");
		Serial.println("TimeRTCSet example, or DS1307RTC SetTime example.");
		Serial.println();
		delay(4000);
	}
	delay(1000);
}

void incrementTime(int intSeconds)
{
	// increment the time counters keeping care to rollover as required
	if(timeStatus() == timeSet)
	{
		Serial.print("adding ");
		Serial.print(intSeconds);
		Serial.println(" seconds to RTC");
		adjustTime(intSeconds);
		RTC.set(now() + intSeconds);
		digitalClockDisplay();
		displayTime();
	}
}

void digitalClockDisplay()
{
	// digital clock display of the time
	Serial.print(hour());
	printDigits(minute());
	printDigits(second());
	Serial.print(" ");
	Serial.print(year());
	Serial.print("-");
	Serial.print(month());
	Serial.print("-");
	Serial.print(day());
	Serial.println();
}

void displayTime()
{
	//colorWipe(colorBlack, 0);
	strCurrentTime = "";
	int min = minute();
	int h = hour();
	if (prev_min == min)
		return;
	paintWord(arrANGIE, colorJGreen);
	paintWord(arrIT, colorWhite);
	paintWord(arrIS, colorWhite);
	paintMinutes(min);
	paintHours(min, h);
	grid.show();
	
	if(strCurrentTime != strTime)
	{
		digitalClockDisplay();
		strTime = strCurrentTime;
		prev_min = min;
		if(strTime == "")
		{
			fadeIn(20);
		}
		// display our tagline
		if((min == 0) | (min == 30) && (timeSetting == 0))
		{
			fadeOut(20);
			colorWipe(colorBlack, 0);
			grid.setBrightness(intBrightness);
			displayHeart(1);
			fadeOut(20);
			colorWipe(colorBlack, 10);
			grid.setBrightness(intBrightness);
		}
	}

}

void paintMinutes(int min)
{
	eraseMinutes();
	// turn on the bullets
	int bullets = min % 5;

	for(int i = 0; i < bullets; i++)
		grid.setPixelColor(3 - i, colorWhite);

	// now we display the appropriate minute counter
	if((min > 4) && (min < 10))
	{
		// FIVE MINUTES
		strCurrentTime = "five ";
		paintWord(arrMFIVE, colorWhite);
	}
	if((min > 9) && (min < 15))
	{
		//TEN MINUTES;
		strCurrentTime = "ten ";
		paintWord(arrMTEN, colorWhite);
	}
	if((min > 14) && (min < 20))
	{
		// QUARTER
		strCurrentTime = "a quarter ";
		paintWord(arrQUARTER, colorWhite);
	}
	if((min > 19) && (min < 25))
	{
		//TWENTY MINUTES
		strCurrentTime = "twenty ";
		paintWord(arrTWENTY, colorWhite);
	}
	if((min > 24) && (min < 30))
	{
		//TWENTY FIVE
		strCurrentTime = "twenty five ";
		paintWord(arrMFIVE, colorWhite);
		paintWord(arrTWENTY, colorWhite);
	}
	if((min > 29) && (min < 35))
	{
		strCurrentTime = "half ";
		paintWord(arrHALF, colorWhite);
	}
	if((min > 34) && (min < 40))
	{
		//TWENTY FIVE
		strCurrentTime = "twenty five ";
		paintWord(arrMFIVE, colorWhite);
		paintWord(arrTWENTY, colorWhite);

	}
	if((min > 39) && (min < 45))
	{
		strCurrentTime = "twenty ";
		paintWord(arrTWENTY, colorWhite);
	}
	if((min > 44) && (min < 50))
	{
		strCurrentTime = "a quarter ";
		paintWord(arrQUARTER, colorWhite);
	}
	if((min > 49) && (min < 55))
	{
		strCurrentTime = "ten ";
		paintWord(arrMTEN, colorWhite);
	}
	if(min > 54)
	{
		strCurrentTime = "five ";
		paintWord(arrMFIVE, colorWhite);
	}

	strCurrentTime = strCurrentTime + bullets + ' ';


}

void paintHours(int min, int h)
{
	eraseHours();

	if(min > 35)
		h = (h + 1) % 24;

	switch(h)
	{
		case 1:
		case 13:
			strCurrentTime = strCurrentTime + "one ";
			paintWord(arrONE, colorWhite);
			break;
		case 2:
		case 14:
			strCurrentTime = strCurrentTime + "two ";
			paintWord(arrTWO, colorWhite);
			break;
		case 3:
		case 15:
			strCurrentTime = strCurrentTime + "three ";
			paintWord(arrTHREE, colorWhite);
			break;
		case 4:
		case 16:
			strCurrentTime = strCurrentTime + "four ";
			paintWord(arrFOUR, colorWhite);
			break;
		case 5:
		case 17:
			strCurrentTime = strCurrentTime + "five ";
			paintWord(arrFIVE, colorWhite);
			break;
		case 6:
		case 18:
			strCurrentTime = strCurrentTime + "six ";
			paintWord(arrSIX, colorWhite);
			break;
		case 7:
		case 19:
			strCurrentTime = strCurrentTime + "seven ";
			paintWord(arrSEVEN, colorWhite);
			break;
		case 8:
		case 20:
			strCurrentTime = strCurrentTime + "eight ";
			paintWord(arrEIGHT, colorWhite);
			break;
		case 9:
		case 21:
			strCurrentTime = strCurrentTime + "nine ";
			paintWord(arrNINE, colorWhite);
			break;
		case 10:
		case 22:
			strCurrentTime = strCurrentTime + "ten ";
			paintWord(arrTEN, colorWhite);
			break;
		case 11:
		case 23:
			strCurrentTime = strCurrentTime + "eleven ";
			paintWord(arrELEVEN, colorWhite);
			break;
		case 0:
		case 12:
		case 24:
			strCurrentTime = strCurrentTime + "twelve ";
			paintWord(arrTWELVE, colorWhite);
			break;
	}

	if((min < 5))
	{

		strCurrentTime = strCurrentTime + "oclock ";
		paintWord(arrPAST, colorBlack);
		paintWord(arrOCLOCK, colorWhite);
		paintWord(arrTO, colorBlack);
	}
	else if(min < 35)
	{
		strCurrentTime = strCurrentTime + "past ";
		paintWord(arrPAST, colorWhite);
		paintWord(arrOCLOCK, colorBlack);
		paintWord(arrTO, colorBlack);

	}
	else
	{
		// if we are greater than 34 minutes past the hour then display
		// the next hour, as we will be displaying a 'to' sign
		strCurrentTime = strCurrentTime + "to ";
		paintWord(arrPAST, colorBlack);
		paintWord(arrOCLOCK, colorBlack);
		paintWord(arrTO, colorWhite);
	}


}

void eraseHours()
{
	for(int i = 4; i < 70; i++)
		grid.setPixelColor(i, colorBlack);
}

void eraseMinutes()
{
	paintWord(arrMFOUR, colorBlack);
	paintWord(arrMFIVE, colorBlack);
	paintWord(arrMTEN, colorBlack);
	paintWord(arrQUARTER, colorBlack);
	paintWord(arrHALF, colorBlack);
	paintWord(arrTWENTY, colorBlack);
}

void printDigits(int digits)
{
	// utility function for digital clock display: prints preceding colon and leading 0
	Serial.print(":");
	if(digits < 10)
		Serial.print('0');
	Serial.print(digits);
}

void rainbow(uint8_t wait)
{
	//secret rainbow mode
	uint16_t i, j;

	for(j = 0; j < 256; j++)
	{
		for(i = 0; i < grid.numPixels(); i++)
		{
			grid.setPixelColor(i, Wheel((i + j) & 255));
		}
		grid.show();
		delay(wait);
	}
}

static void chase(uint32_t color, uint8_t wait)
{
	for(uint16_t i = 0; i < grid.numPixels() + 4; i++)
	{
		grid.setPixelColor(i  , color); // Draw new pixel
		grid.setPixelColor(i - 4, 0); // Erase pixel a few steps back
		grid.show();
		delay(wait);
	}
}

void fadeOut(int time)
{
	for (int i = intBrightness; i > 0; --i)
	{
		grid.setBrightness(i);
		grid.show();
		delay(time);
	}
}

void fadeIn(int time)
{
	for(int i = 1; i < intBrightness; ++i)
	{
		grid.setBrightness(i);
		grid.show();
		delay(time);
	}
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t color, uint8_t wait)
{
	for(uint16_t i = 0; i < grid.numPixels(); i++)
	{
		grid.setPixelColor(i, color);
	}
	grid.show();
	delay(wait);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos)
{
	if(WheelPos < 85)
	{
		return grid.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
	}
	else if(WheelPos < 170)
	{
		WheelPos -= 85;
		return grid.Color(255 - WheelPos * 3, 0, WheelPos * 3);
	}
	else
	{
		WheelPos -= 170;
		return grid.Color(0, WheelPos * 3, 255 - WheelPos * 3);
	}
}

void paintWord(int arrWord[], uint32_t intColor)
{
	for(int i = 0; i < grid.numPixels() + 1; i++)
	{
		if(arrWord[i] == -1)
		{
			//grid.show();
			break;
		}
		else
		{
			grid.setPixelColor(arrWord[i], intColor);
		}
	}
}

void spellWord(int arrWord[], uint32_t intColor)
{
	for(int i = 0; i < grid.numPixels() + 1; i++)
	{
		if(arrWord[i] == -1)
		{
			break;
		}
		else
		{
			grid.setPixelColor(arrWord[i], intColor);
			grid.show();
			delay(500);
		}
	}
}

void displayHeart(int times)
{
	//grid.setBrightness(200);
	int x;
	if(intBrightness > 150)
		x = 10;
	else
		x = 20;
	paintWord(arrHEART, colorRed);
	grid.show();
	for(int i = 0; i < times; i++)
	{
		fadeOut(x);
		fadeIn(x);
	}
	paintWord(arrHEART, colorBlack);
	grid.show();
}

// print out the software version number
void printVersion(void)
{
	delay(2000);
	Serial.println();
	Serial.println("Angie World Clock - Arduino v1.0.0");
	Serial.println("(c)2016 Konstantinos Iliakis");
	Serial.println();
}

unsigned long processSyncMessage()
{
	unsigned long pctime = 0L;
	const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

	if(Serial.find(TIME_HEADER))
	{
		pctime = Serial.parseInt();
		pctime = pctime + 7200;
		return pctime;
		if( pctime < DEFAULT_TIME)   // check the value is a valid time (greater than Jan 1 2013)
		{
			pctime = 0L; // return 0 to indicate that the time is not valid
		}
		Serial.println();
		Serial.println("Time Set via Serial");
		Serial.println();
	}
	return pctime;
}

// runs throught the various displays, testing
void test_grid()
{
	printVersion();
	grid.setBrightness(intBrightness);

	rainbow(2);
	colorWipe(colorBlack, 0);

	displayHeart(2);
	colorWipe(colorBlack, 0);

	colorWipe(colorJGreen, 50);
	chase(colorRed, 2); // Red
	chase(colorGreen, 2); // Green
	chase(colorBlue, 2); // Blue
	chase(colorWhite, 2); // White
	colorWipe(colorBlack, 0);

	paintWord(arrIT, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrIS, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrHALF, colorWhite);
	grid.show();
	delay(100);

	colorWipe(colorBlack, 0);
	paintWord(arrQUARTER, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrPAST, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrOCLOCK, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrTO, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrONE, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrTWO, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrTHREE, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrFOUR, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrMFIVE, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrFIVE, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrSIX, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrSEVEN, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrEIGHT, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrNINE, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrMTEN, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrTEN, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrELEVEN, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrTWELVE, colorWhite);
	grid.show();
	delay(100);
	colorWipe(colorBlack, 0);
	paintWord(arrTWENTY, colorWhite);
	grid.show();
	delay(100);

	colorWipe(colorBlack, 0);
	paintWord(arrANGIE, colorJGreen);
	grid.show();
	delay(1000);
	fadeOut(50);
	colorWipe(colorBlack, 0);

	grid.setBrightness(intBrightness);
	intTestMode = !intTestMode;
}
