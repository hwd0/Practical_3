/*
 * BinClock.c
 * Jarrod Olivier
 * Modified for EEE3095S/3096S by Keegan Crankshaw
 * August 2019
 * 
 * <frdjon009> <wldham001>
 * Date
*/

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h> //For printf functions
#include <stdlib.h> // For system functions
#include <softPwm.h>
#include <math.h>

#include "BinClock.h"
#include "CurrentTime.h"

//Global variables
int hours, mins, secs;
long lastInterruptTime = 0; //Used for button debounce
int RTC; //Holds the RTC instance

int HH,MM,SS;

void initGPIO(void){
	/* 
	 * Sets GPIO using wiringPi pins. see pinout.xyz for specific wiringPi pins
	 * You can also use "gpio readall" in the command line to get the pins
	 * Note: wiringPi does not use GPIO or board pin numbers (unless specifically set to that mode)
	 */
	printf("Setting up\n");
	wiringPiSetup(); //This is the default mode. If you want to change pinouts, be aware
	
	RTC = wiringPiI2CSetup(RTCAddr); //Set up the RTC
	
	//Set up the LEDS
	for(int i=0; i < sizeof(LEDS)/sizeof(LEDS[0]); i++){
	    pinMode(LEDS[i], OUTPUT);
	}
	
	//Set Up the Seconds LED for PWM
	//Write your logic here
	pinMode(SECS, PWM_OUTPUT);
    //softPwmCreate(SECS, 0, 100); //try 60 instead of 100, maybe 60 could be fully on?
   
	printf("LEDS done\n");
	
	//Set up the Buttons
	for(int j=0; j < sizeof(BTNS)/sizeof(BTNS[0]); j++){
		pinMode(BTNS[j], INPUT);
		pullUpDnControl(BTNS[j], PUD_UP);
	}
	
	//Attach interrupts to Buttons
	//Write your logic here;
	/*if (wiringPiISR(5, INT_EDGE_FALLING, &hourInc)!=0){
   printf("Registering ISR failed\n");
   return 2;
   }
   if (wiringPiISR(30, INT_EDGE_FALLING, &minInc)!=0){
   printf("Registering ISR failed\n");
   return 2;
   }*/
   wiringPiISR(5, INT_EDGE_FALLING, &hourInc);
   wiringPiISR(30, INT_EDGE_FALLING, &minInc);
   
	printf("BTNS done\n");
	printf("Setup done\n");
}


/*
 * The main function
 * This function is called, and calls all relevant functions we've written
 */
int main(void){
	initGPIO();

	//Set random time (3:04PM)
	//You can comment this file out later
	wiringPiI2CWriteReg8(RTC, HOUR, 0x01+TIMEZONE);
	wiringPiI2CWriteReg8(RTC, MIN, 0x00);
	wiringPiI2CWriteReg8(RTC, SEC, 0b10000000);
	
	//toggleTime();                               //use this when above is commented out?
   
	// Repeat this until we shut down
	for (;;){
		//Fetch the time from the RTC
		//Write your logic here
      
     hours = wiringPiI2CReadReg8(RTC, HOUR);
     mins = wiringPiI2CReadReg8(RTC, MIN);
     secs = wiringPiI2CReadReg8(RTC, SEC) -0b10000000;
		
		//Function calls to toggle LEDs
		//Write your logic here
      lightHours(hours);
      lightMins(mins);
      secPWM(secs);
		
		// Print out the time we have stored on our RTC
		printf("The current time is: %x:%x:%x\n", hours, mins, hexCompensation(secs));

		//using a delay to make our program "less CPU hungry"
		delay(1000); //milliseconds
	}
	return 0;
}

/*
 * Change the hour format to 12 hours
 */
int hFormat(int hours){
	/*formats to 12h*/
	if (hours >= 24){
		hours = 0;
	}
	else if (hours > 12){
		hours -= 12;
	}
	return (int)hours;
}

/*
 * Turns on corresponding LED's for hours
 */
void lightHours(int units){
	// Write your logic to light up the hour LEDs here	
    
       units = hexCompensation(units);
       int a = 0;
       for (int i =0; i<4; i++){
	       a = pow(2,i+1);
	       if(units%a){
		       digitalWrite(LEDS[i],1);
		       units -=units%a;
		       }
		else{
			digitalWrite(LEDS[i],0);}
	       }
}

/*
 * Turn on the Minute LEDs
 */
void lightMins(int units){
	//Write your logic to light up the minute LEDs here

	   units = hexCompensation(units);
       int a = 0;
       int b = 4;
       for (int i =0; i<6; i++){
	       a = pow(2,i+1);
	       if(units%a){
		       digitalWrite(LEDS[i + b],1);
		       units -=units%a;
		       }
		else{
			digitalWrite(LEDS[i + b],0);}
	       }

}

/*
 * PWM on the Seconds LED
 * The LED should have 60 brightness levels
 * The LED should be "off" at 0 seconds, and fully bright at 59 seconds
 */
void secPWM(int units){
	// Write your logic here
   /*if(0<=units && units<60){                  //not sure if this works in C?
       softPwmWrite(SECS, units);
   }
   else{
       softPwmWrite(SECS, 0);
   }*/
   pwmWrite(SECS, round(units*18));
}

/*
 * hexCompensation
 * This function may not be necessary if you use bit-shifting rather than decimal checking for writing out time values
 */
int hexCompensation(int units){
	/*Convert HEX or BCD value to DEC where 0x45 == 0d45 
	  This was created as the lighXXX functions which determine what GPIO pin to set HIGH/LOW
	  perform operations which work in base10 and not base16 (incorrect logic) 
	*/
	int unitsU = units%0x10;

	if (units >= 0x50){
		units = 50 + unitsU;
	}
	else if (units >= 0x40){
		units = 40 + unitsU;
	}
	else if (units >= 0x30){
		units = 30 + unitsU;
	}
	else if (units >= 0x20){
		units = 20 + unitsU;
	}
	else if (units >= 0x10){
		units = 10 + unitsU;
	}
	return units;
}


/*
 * decCompensation
 * This function "undoes" hexCompensation in order to write the correct base 16 value through I2C
 */
int decCompensation(int units){
	int unitsU = units%10;

	if (units >= 50){
		units = 0x50 + unitsU;
	}
	else if (units >= 40){
		units = 0x40 + unitsU;
	}
	else if (units >= 30){
		units = 0x30 + unitsU;
	}
	else if (units >= 20){
		units = 0x20 + unitsU;
	}
	else if (units >= 10){
		units = 0x10 + unitsU;
	}
	return units;
}


/*
 * hourInc
 * Fetch the hour value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 23 hours in a day
 * Software Debouncing should be used
 */
void hourInc(void){
	//Debounce
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		printf("Interrupt 1 triggered, %x\n", hours);
		//Fetch RTC Time
      hours = hexCompensation(wiringPiI2CReadReg8(RTC, HOUR));
		//Increase hours by 1, ensuring not to overflow
      if (0<=hours && hours<23){
          hours++;
      }
      else if(hours==23){
          hours = 0;
      }
      else{
          hours = hours;
      }
      hours = hFormat(hours);
		//Write hours back to the RTC
      wiringPiI2CWriteReg8(RTC, HOUR, decCompensation(hours));
	}
	lastInterruptTime = interruptTime;
}

/* 
 * minInc
 * Fetch the minute value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 60 minutes in an hour
 * Software Debouncing should be used
 */
void minInc(void){
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		printf("Interrupt 2 triggered, %x\n", mins);
		//Fetch RTC Time
       mins = hexCompensation(wiringPiI2CReadReg8(RTC, MIN));
		//Increase minutes by 1, ensuring not to overflow
      if (0<=mins && mins<59){
          mins++;
      }
      else if(hours==59){
          mins = 0;
          hours++;
      }
      else{
          mins = mins;
      }

		//Write minutes back to the RTC
      wiringPiI2CWriteReg8(RTC, MIN, decCompensation(mins));
	}
	lastInterruptTime = interruptTime;
}

//This interrupt will fetch current time from another script and write it to the clock registers
//This functions will toggle a flag that is checked in main
void toggleTime(void){
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		HH = getHours();
		MM = getMins();
		SS = getSecs();

		HH = hFormat(HH);
		HH = decCompensation(HH);
		wiringPiI2CWriteReg8(RTC, HOUR, HH);

		MM = decCompensation(MM);
		wiringPiI2CWriteReg8(RTC, MIN, MM);

		SS = decCompensation(SS);
		wiringPiI2CWriteReg8(RTC, SEC, 0b10000000+SS);

	}
	lastInterruptTime = interruptTime;
}
