#include <stdio.h>
#include "../shared/shared.h"

appInfo getAppInfo() {
	appInfo appinfo;
	appinfo.name = "Hello world Non Loop";
	appinfo.app_type = shell;
	return appinfo;
}

int appMain() {
	printf("Hello World!\n");
	return 0;
}