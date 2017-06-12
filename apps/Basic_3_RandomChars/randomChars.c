#include <stdio.h>
#include <wiringPi.h>
#include <lcd.h>
#include "../shared/shared.h"

#include <time.h>

appInfo getAppInfo() {
	appInfo appinfo;
	appinfo.name = "random Chars";
	appinfo.app_type = basic;
	return appinfo;
}

void appMain(arg_struct* args) {
	lcdClear(args->fd);
	srand(time(NULL)); 
	while (args->running) {
		lcdPosition(args->fd, rand()%16, rand()%2);
		lcdPutchar(args->fd, rand());
		usleep(rand()%654321+100000);
	}
}