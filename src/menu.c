#include <wiringPi.h>
#include "../lib/rotaryencoder.c"
#include <lcd.h>
#include <softPwm.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include "../shared/shared.h"
#include <string.h>

void* LoadSharedLib(const char* path)
{
	void* ptr = dlopen(path, RTLD_NOW);
	if (ptr == NULL) {
		fprintf(stderr, "%s\n", dlerror());
	}
	return ptr;
}

void* LoadFunction(void* sharedLib, const char* name)
{
	char* error = dlerror();
	void* ptr = dlsym(sharedLib, name);
	if (ptr == NULL || (error = dlerror()) != NULL) {
		fprintf(stderr, "%s\n", dlerror());
	}
	return ptr;
}

void UnloadSharedLib(void* sharedLib)
{
	dlclose(sharedLib);
}

int initHardware() {
	wiringPiSetupGpio();
	pinMode(BUTTON_A, INPUT);
	pinMode(BUTTON_B, INPUT);
	pullUpDnControl(BUTTON_A, PUD_UP);
	pullUpDnControl(BUTTON_B, PUD_UP);
	softPwmCreate(LCD_LED_PIN, 100, 100);
	int fd = lcdInit(2, 16, 4, 7, 8, 25, 24, 23, 18, 0, 0, 0, 0);
	lcdClear(fd);
	lcdPosition(fd, 6, 0);
	lcdPuts(fd, "Menu");
	lcdPosition(fd, 3, 1);
	lcdPuts(fd, "initialized");
	usleep(450000);
	int i;
	for (i=0;i<14;i++)
	{
		lcdSendCommand(fd, 0x18);//scroll display left
		usleep(35000);
	}
	return fd;
}

typedef int(*ModuleMainFunc)(void);
typedef void*(*ModuleMainFuncLoop)(void*);
typedef appInfo(*ModuleAppInfo)(void);

void* loadSharedLibFromFolder(char *folder) {
	char libName[MAX_FOLDER_NAME_LENGTH+8];
	strcpy(libName, "./lib");
	strcat(libName, folder);
	strcat(libName, ".so");
	void* moduleHandleLoop = LoadSharedLib(libName);
	if (moduleHandleLoop == NULL) {
		fprintf(stderr, "Could not load shared libary of app folder '%s'.\n", folder);
	}
	return moduleHandleLoop;
}

appInfo getAppInfo(void* appLibaryHandle) {
	appInfo appinfo;
	appinfo.app_type = 0;
	appinfo.name = "NoName";
	if (appLibaryHandle == NULL) {
		appinfo.app_type = -1;
		appinfo.name = "invalidAppFolder";
		return appinfo;
	}
	ModuleAppInfo appInfoFunc;
	appInfoFunc = LoadFunction(appLibaryHandle, "getAppInfo");
	if (appInfoFunc == NULL) {
		fprintf(stderr, "Could not get app info of app.\n");
		appinfo.app_type = -1;
		appinfo.name = "invalidAppCodeGetAppInfo";
		return appinfo;
	}
	appinfo = appInfoFunc();

	return appinfo;
}

pthread_t runApp(void* appLibaryHandle, arg_struct* args) {
	ModuleMainFuncLoop appMain;
	appMain = LoadFunction(appLibaryHandle, "appMain");
	if (appMain == NULL) {
		fprintf(stderr, "Could not get app main.\n");
	}
	pthread_t appThread;
	if (pthread_create(&appThread, NULL, appMain, &*args)) {
		fprintf(stderr, "Error creating thread.\n");
	}

	return appThread;
}

int main(int argc, char **argv) {
	int fd = initHardware();
	struct encoder *encoder = setupencoder(ENCODER_A, ENCODER_B);

	if (argc > 1){
		char appFolderName[MAX_FOLDER_NAME_LENGTH] = "";
		argc--;
		*argv++;
		while (argc--){
			strcat(appFolderName, (char*) *argv++);
			if (argc != 0)
				strcat(appFolderName, " ");
		}
		void* appLibaryHandle = loadSharedLibFromFolder(appFolderName);
		printf("Trying to load app folder '%s' directly.\n", appFolderName);
		appInfo appinfo = getAppInfo(appLibaryHandle);
		if (appinfo.app_type == -1) {
			fprintf(stderr, "Error getting app info.\n");
			return 3;
		}
		arg_struct args = { .running = 1, .fd = fd, .encoder = encoder };
		pthread_t appThread = runApp(appLibaryHandle, &args);
		sleep(10);
		args.running = 0;
		if (pthread_join(appThread, NULL)) {

			fprintf(stderr, "Error joining thread.\n");
			return 2;

		}
		UnloadSharedLib(appLibaryHandle);
		lcdClear(fd);
		lcdPosition(fd, 8-(7/2), 0);
		lcdPuts(fd, "Program");
		lcdPosition(fd, 8 - (7 / 2), 1);
		lcdPuts(fd, "exited.");
		sleep(1);
		softPwmWrite(LCD_LED_PIN, 0);
		usleep(100000);
		softPwmWrite(LCD_LED_PIN, 100);
		usleep(300000);
		softPwmWrite(LCD_LED_PIN, 0);
		usleep(100000);
		softPwmWrite(LCD_LED_PIN, 50);
		usleep(200000);
		softPwmWrite(LCD_LED_PIN, 0);
		usleep(100000);
		printf("Exiting.\n");
		return 0;
	}
	void* moduleHandle = LoadSharedLib("./libShell_1_HelloWorld.so");
	ModuleMainFunc foo;
	foo = LoadFunction(moduleHandle, "appMain");	
	foo();
	UnloadSharedLib(moduleHandle);

	


	void* moduleHandleLoop = LoadSharedLib("./libBasic_1_HelloWorldDisplayLoop.so");
	ModuleMainFuncLoop appMain;
	appMain = LoadFunction(moduleHandleLoop, "appMain");
	
	ModuleAppInfo appInfoFunc;
	appInfoFunc = LoadFunction(moduleHandleLoop, "getAppInfo");

	appInfo loadedAppInfo = appInfoFunc();

	printf(loadedAppInfo.name);

	pthread_t appThread;
	arg_struct args = { .running = 1,.fd = fd,.encoder = encoder };
		if (pthread_create(&appThread, NULL, appMain, &args)) {
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}
	printf("Sleeping a bit\n");
	sleep(5);
	printf("Attempt to stop app func loop by setting running argument to false.\n");
	args.running = 0;
	if (pthread_join(appThread, NULL)) {

		fprintf(stderr, "Error joining thread\n");
		return 2;

	}
	printf("Joined threads.\n");
	UnloadSharedLib(moduleHandleLoop);
	return 0;
}
