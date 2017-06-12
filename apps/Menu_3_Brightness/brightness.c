#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <lcd.h>
#include "../shared/shared.h"
#include <string.h>
#include <math.h>
#include "../../lib/rotaryencoder.h"
#include <softPwm.h>

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
	int lastValue = args->encoder->value;
	int lastLength = 1;
	int lastState = LOW;
	int lastState2 = LOW;
	lcdClear(args->fd);
	while (args->running) {
		if (digitalRead(BUTTON_A) == HIGH && lastState == LOW) {
			lastState = HIGH;
		}
		else if (lastState == HIGH && digitalRead(BUTTON_A) == LOW) {
			//			printf("read BUTTON_A\n");
			//lcdPutchar(args->fd, (char)0);
			lastState = LOW;
		}
		if (digitalRead(BUTTON_B) == HIGH && lastState2 == LOW) {
			lastState2 = HIGH;
		}
		else if (lastState2 == HIGH && digitalRead(BUTTON_B) == LOW) {
			//lcdPutchar(args->fd, (char)2);
			lastState2 = LOW;
			args->running = 0;
			//			printf("read BUTTON_B\n");
		}

		if (args->encoder->value != lastValue) {
			if (args->encoder->value < 0) {
				args->encoder->value = 0;
			}
			if (args->encoder->value > 400) {
				args->encoder->value = 400;
			}
			if (args->encoder->value > 0) {
				if (args->encoder->value <= 400)
					softPwmWrite(LCD_LED_PIN, args->encoder->value / 4);
				else
					softPwmWrite(LCD_LED_PIN, 100);
			}
			else {
				softPwmWrite(LCD_LED_PIN, 0);
			}

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
			
			switch ((int)((float)(lastValue / 4) / 100 * 80) % 5) {
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
		}
		usleep(1000);
	}
}