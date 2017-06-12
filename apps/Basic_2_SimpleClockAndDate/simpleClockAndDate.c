#include <stdio.h>
#include <wiringPi.h>
#include <lcd.h>
#include "../shared/shared.h"

#include <time.h>

appInfo getAppInfo() {
	appInfo appinfo;
	appinfo.name = "Simple Clock and Date";
	appinfo.app_type = basic;
	return appinfo;
}

void appMain(arg_struct* args) {
	lcdClear(args->fd);
	time_t current_time;
	struct tm * time_info;
	char timeString[9];  // space for "HH:MM:SS\0"
	char dateString[11]; // space for "DD.MM:JJJJ\0"
	
	while (args->running) {
		lcdPosition(args->fd, 0, 0);
		time(&current_time);
		time_info = localtime(&current_time);
		strftime(timeString, sizeof(timeString), "%H:%M:%S", time_info);
		strftime(dateString, sizeof(dateString), "%e.%m.%Y", time_info);
		lcdPuts(args->fd, timeString);
		lcdPosition(args->fd, 6, 1);
		lcdPuts(args->fd, dateString);
		usleep(100000);
	}
}