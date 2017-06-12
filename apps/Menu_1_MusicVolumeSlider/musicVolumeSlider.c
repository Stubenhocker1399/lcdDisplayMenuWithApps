#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <lcd.h>
#include "../shared/shared.h"
#include <string.h>
#include <math.h>
#include "../../lib/rotaryencoder.h"

appInfo getAppInfo() {
	appInfo appinfo;
	appinfo.name = "Music Volume";
	appinfo.app_type = menu;
	return appinfo;
}

void appMain(arg_struct* args) {
	static unsigned char oneRow[8] = {
		0b10000,
		0b10000,
		0b10000,
		0b10000,
		0b10000,
		0b10000,
		0b10000,
		0b10000
	};
	static unsigned char twoRow[8] = {
		0b11000,
		0b11000,
		0b11000,
		0b11000,
		0b11000,
		0b11000,
		0b11000,
		0b11000
	};
	static unsigned char threeRow[8] = {
		0b11100,
		0b11100,
		0b11100,
		0b11100,
		0b11100,
		0b11100,
		0b11100,
		0b11100
	};
	static unsigned char fourRow[8] = {
		0b11110,
		0b11110,
		0b11110,
		0b11110,
		0b11110,
		0b11110,
		0b11110,
		0b11110
	};
	lcdCharDef(args->fd, 1, oneRow);
	lcdCharDef(args->fd, 2, twoRow);
	lcdCharDef(args->fd, 3, threeRow);
	lcdCharDef(args->fd, 4, fourRow);
	char valueStr[16] = "0";
	char winexeShellString[120] = "/home/pi/winexe/source/build/winexe --user=shutdownuser%shutdown //192.168.1.117 'nircmd.exe setsysvolume "; 
	int lastValue = args->encoder->value;
	int lastLength = 1;
	int lastState = LOW;
	int lastState2 = LOW;
	printf("Hello World from musicVolumeSlider!\n"); 
	lcdClear(args->fd);
	lcdPosition(args->fd, 5, 0);
	lcdPuts(args->fd, "Hello");
	lcdPosition(args->fd, 4, 1);
	lcdPuts(args->fd, "volume!");
	while (args->running) {
		if (digitalRead(BUTTON_A) == HIGH && lastState == LOW) {
			lastState = HIGH;
		}
		else if (lastState == HIGH && digitalRead(BUTTON_A) == LOW) {
			//			printf("read BUTTON_A\n");
			lcdPutchar(args->fd, (char)0);
			lastState = LOW;
		}
		if (digitalRead(BUTTON_B) == HIGH && lastState2 == LOW) {
			lastState2 = HIGH;
		}
		else if (lastState2 == HIGH && digitalRead(BUTTON_B) == LOW) {
			lcdPutchar(args->fd, (char)2);
			lastState2 = LOW;
			//			printf("read BUTTON_B\n");
		}

		if (args->encoder->value != lastValue) {
			char valuepoin[8];
			//			sprintf(valuepoin,"%i", (int)(((double)encoder->value/65535.0)*100));
			//			sprintf(valuepoin,"%i", (int)(encoder->value*100)/65535);
			//			sprintf(valuepoin,"%i", (int)(((double)((double)encoder->value/(double)4)/(double)100))*(double)65535);
			sprintf(valuepoin, "%i", (int)(((double)args->encoder->value) / 4 / 100 * 65535));
			//			printf(valuepoin);
			//			printf("\n");
			char longstr[200] = "";
			strcat(longstr, winexeShellString);
			strcat(longstr, valuepoin);
			strcat(longstr, "'");
			strcat(longstr, "");////////////////
								//			printf(longstr);
								//			printf(winexeShellString);
			system(longstr);
			//printf("Encoder value: %i\n",encoder->value);
			sprintf(valueStr, "%d", (args->encoder->value) / 4);
			if (lastValue == 0 || (lastLength != strlen(valueStr))) {
				lcdPosition(args->fd, 0, 1);
				lcdPuts(args->fd, "                ");
			}
			lastLength = strlen(valueStr);
			lcdPosition(args->fd, 8 - (strlen(valueStr) / 2), 1);
			lcdPuts(args->fd, valueStr);
			lastValue = args->encoder->value;
			lcdPosition(args->fd, 0, 0);
			lcdPutchar(args->fd, (char)(lastValue / 4));


			lcdPosition(args->fd, 0, 0);
			int i = 0;
			for (i; i<floor((float)(lastValue / 4) / 100 * 80 / 5); i++) {
				lcdPutchar(args->fd, (char)255);
			}
			//			printf("Abs:%i Switch:%i\n",(int)((float)(lastValue/4)/100*80),(int)((float)(lastValue/4)/100*80)%6);
			//			printf("Abs:%i Switch:%i snd:%f\n",(int)((float)(lastValue/4)/100*80),(((int)(((float)(lastValue/4)/100*80)))-(((int)floor(((float)(lastValue/4)/100*80/5)))*5)),floor(((float)(lastValue/4)/100*80/5)));

			switch ((int)((float)(lastValue / 4) / 100 * 80) % 5) {
				//			switch(((int)(((float)(lastValue/4)/100*80)))-(((int)floor(((float)(lastValue/4)/100*80/5)))*5)){
			case 0:
				lcdPutchar(args->fd, (char)254);
				break;
			case 1:
				lcdPutchar(args->fd, (char)1);
				break;
			case 2:
				lcdPutchar(args->fd, (char)2);
				break;
			case 3:
				lcdPutchar(args->fd, (char)3);
				break;
			case 4:
				lcdPutchar(args->fd, (char)4);
				break;
			}
			i = 0;
			for (i; i<15 - floor((float)(lastValue / 4) / 100 * 80 / 5); i++) {
				lcdPutchar(args->fd, (char)254);
			}

			//printf("Full:%i Sub:%i\n", ((int)((float)(lastValue/4)/100*80)%6)!=0),(int)((float)lastValue/100.0*80.0);
		}
		usleep(1000);
	}
}