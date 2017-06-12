#include <stdio.h>
#include <wiringPi.h>
#include <lcd.h>
#include "../shared/shared.h"

appInfo getAppInfo() {
	appInfo appinfo;
	appinfo.name = "Hello world";
	appinfo.app_type = basic;
	return appinfo;
}

void appMain(arg_struct* args) {
	printf("Hello World from helloworldDiplayLoop!\n");
	//printf("Running... %i\n", args->running);
	lcdClear(args->fd);
	lcdPosition(args->fd, 5, 0);
	lcdPuts(args->fd, "Hello");
	lcdPosition(args->fd, 5, 1);
	lcdPuts(args->fd, "world!");
	int i = 0;
	//printf("Running... %i\n", args->running);
	while (args->running) {
		lcdPosition(args->fd, 0, 0);
		lcdPutchar(args->fd, (char)i++);
		usleep(10000);
		//printf("Running... %i\n", args->running);
	}
}