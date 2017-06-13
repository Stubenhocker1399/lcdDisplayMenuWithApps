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
#include <sys/types.h>
#include <dirent.h>
#include <signal.h>

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

static volatile mainRunning = 1;

void stopMainRunning(int handler) {
	mainRunning = 0;
}

int main(int argc, char **argv) {
	signal(SIGINT, stopMainRunning);
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
	typedef struct {
		char libPath[MAX_FOLDER_NAME_LENGTH];
		char appname[MAX_FOLDER_NAME_LENGTH];
	}libInfo;

	libInfo shellApps[MAX_APPS_PER_CATEGORY];
	int shellAppsn = 0;
	libInfo basicApps[MAX_APPS_PER_CATEGORY];
	int basicAppsn = 0;
	libInfo menuApps[MAX_APPS_PER_CATEGORY];
	int menuAppsn = 0;

	struct dirent **binFolderList;
	int n;		
	n = scandir("./", &binFolderList, 0, alphasort);
	if (n < 0)
		perror("scandir");
	else {
		while (n--) {
			if (!(!strcmp(binFolderList[n]->d_name, ".") || !strcmp(binFolderList[n]->d_name, "..") || !strcmp(binFolderList[n]->d_name, "lcdMenu"))) {
				char libPath[MAX_FOLDER_NAME_LENGTH];
				strcpy(libPath, "./");
				strcat(libPath, binFolderList[n]->d_name);
				void* appLibaryHandle = LoadSharedLib(libPath);
				appInfo appinfo = getAppInfo(appLibaryHandle);
				if (appinfo.app_type == -1) {
					fprintf(stderr, "%s: Error getting app info.\n", libPath);
					return 3;
				}
				printf("%s: Found app '%s' of type %i.\n", libPath, appinfo.name, appinfo.app_type);
				switch (appinfo.app_type)
				{
				case shell:
					strcpy(shellApps[shellAppsn].libPath,libPath);	
					strcpy(shellApps[shellAppsn++].appname, appinfo.name);
					break;
				case basic:
					strcpy(basicApps[basicAppsn].libPath, libPath);
					strcpy(basicApps[basicAppsn++].appname, appinfo.name);
					break;
				case menu:
					strcpy(menuApps[menuAppsn].libPath, libPath);
					strcpy(menuApps[menuAppsn++].appname, appinfo.name);
					break;
				default:
					fprintf(stderr, "%s: Error reading app_type, unexpected number: %i\n", appinfo.app_type);
					break;
				}
				UnloadSharedLib(appLibaryHandle); 
				
			}free(binFolderList[n]);
			
		}free(binFolderList);
	}
	enum states {
		top,
		bottom,
		menuapp
	};

	arg_struct args = { .running = 0,.fd = fd,.encoder = encoder };
	int currentTopApp = basicAppsn-1;
	int pointerPos = 7;
	int offset = 0;
	char menuString[MAX_FOLDER_NAME_LENGTH * MAX_APPS_PER_CATEGORY * 3];

	args.running = 1;
	void* appLibaryHandle = LoadSharedLib(basicApps[currentTopApp].libPath);
	pthread_t appThread = runApp(appLibaryHandle, &args);
	int lastEncoderValue = encoder->value / ENCODER_SUB_STEPS;
	enum states state = top;
	int changedState = 0;
	while (mainRunning) {
		switch (state)
		{
		case top:
			if (encoder->value/ENCODER_SUB_STEPS != lastEncoderValue){
				args.running = 0;
				if (pthread_join(appThread, NULL)) {

					fprintf(stderr, "Error joining thread.\n");
					return 2;
				}
				UnloadSharedLib(appLibaryHandle);
				if (encoder->value / ENCODER_SUB_STEPS > lastEncoderValue) {
					currentTopApp--;
					if (currentTopApp < 0) {
						currentTopApp = basicAppsn - 1;
					}
				}
				else {
					currentTopApp++;
					if (currentTopApp > basicAppsn - 1) {
						currentTopApp = 0;
					}
				}
				puts(basicApps[currentTopApp].appname);
					
				appLibaryHandle = LoadSharedLib(basicApps[currentTopApp].libPath);
				appThread = runApp(appLibaryHandle, &args);
				args.running = 1;
				lastEncoderValue = encoder->value / ENCODER_SUB_STEPS;
			}
			changedState = 0;
			if (digitalRead(BUTTON_A) == LOW) {
				usleep(10000);
				while (digitalRead(BUTTON_A) == LOW) {
					usleep(5000);
				}
				usleep(10000);
				state = bottom;
				changedState = 1;
				args.running = 0;
				if (pthread_join(appThread, NULL)) {

					fprintf(stderr, "Error joining thread.\n");
					return 2;
				}
				UnloadSharedLib(appLibaryHandle);

				static unsigned char pointerArrow[8] = {
					0b00100,
					0b01010,
					0b10001,
					0b00000,
					0b01110,
					0b10001,
					0b10001,
					0b00000,
				};
				lcdCharDef(fd, 8, pointerArrow);
			}
			break;
		case bottom:
			if (changedState) {
				int i = menuAppsn;
				for (--i; i >= 0; i--) {
					if (i == menuAppsn - 1)
						strcpy(menuString, menuApps[i].appname);
					else
						strcat(menuString, menuApps[i].appname);
					if(i!=0)
						strcat(menuString, "   ");				
				}
				lcdClear(fd);
			}
			if (encoder->value  != lastEncoderValue || changedState) {
				lcdPosition(fd, pointerPos, 1);
				lcdPutchar(fd, (char)254);//blank out cursor
				lcdPosition(fd, 0, 0);
				int i;
				for(i=0;i<16;i++){
					if(i+offset>=0 && i+offset<strlen(menuString))
						lcdPutchar(fd, menuString[i+offset]);
				}
				if (encoder->value > lastEncoderValue) {
					pointerPos++;
					if (pointerPos > 12)
					{
						pointerPos--;
						offset++;
					}
					if (offset > strlen(menuString)-16) {
						offset = strlen(menuString)-16;
					}
				}
				else {
					pointerPos--;
					if (pointerPos < 4)
					{
						pointerPos++;
						offset--;
					}
					if (offset < 0)
						offset = 0;
				}
				lcdPosition(fd, pointerPos, 1);
				lcdPutchar(fd, (char)8);

				changedState = 0;
				lastEncoderValue = encoder->value;
			}
			if (digitalRead(BUTTON_B) == LOW) {
				args.running = 1;
				appLibaryHandle = LoadSharedLib(basicApps[currentTopApp].libPath);
				appThread = runApp(appLibaryHandle, &args);
				lastEncoderValue = encoder->value / ENCODER_SUB_STEPS;
				state = top;
			}
			if (digitalRead(BUTTON_A) == LOW) {
				usleep(10000);
				while (digitalRead(BUTTON_A) == LOW){
					usleep(5000);
				}
				usleep(10000);
				//figure out selected app
				char menuString[MAX_FOLDER_NAME_LENGTH * MAX_APPS_PER_CATEGORY * 3];

				int i = menuAppsn;
				for (--i; i >= 0; i--) {
					if (i == menuAppsn - 1)
						strcpy(menuString, menuApps[i].appname);
					else
						strcat(menuString, menuApps[i].appname);
					if (i != 0)
						strcat(menuString, "   ");
					if (offset + pointerPos < strlen(menuString))
						break;
				}
				int selectedApp = i;
				args.running = 1;
				appLibaryHandle = LoadSharedLib(menuApps[selectedApp].libPath);
				appThread = runApp(appLibaryHandle, &args);
				lastEncoderValue = encoder->value / ENCODER_SUB_STEPS;
				state = menuapp;
				changedState = 1;
			}
			break;
		case menuapp:
			while (args.running){
				usleep(10000);
			}
			if (pthread_join(appThread, NULL)) {

				fprintf(stderr, "Error joining thread.\n");
				return 2;
			}
			while (digitalRead(BUTTON_A) == LOW) {
				usleep(5000);
			}
			while (digitalRead(BUTTON_B) == LOW) {
				usleep(5000);
			}
			UnloadSharedLib(appLibaryHandle);
			changedState = 1;
			state = bottom;
			static unsigned char pointerArrow[8] = {
				0b00100,
				0b01010,
				0b10001,
				0b00000,
				0b01110,
				0b10001,
				0b10001,
				0b00000,
			};
			lcdCharDef(fd, 8, pointerArrow);
			break;
		default:
			fprintf(stderr, "Invalid menu state: %i\n", state);
			break;
		}
		delay(1);
	}
	args.running = 0;
	if (pthread_join(appThread, NULL)) {

		fprintf(stderr, "Error joining thread.\n");
		return 2;
	}
	UnloadSharedLib(appLibaryHandle);
	printf("\nExiting...\n");
	return 0;
}
