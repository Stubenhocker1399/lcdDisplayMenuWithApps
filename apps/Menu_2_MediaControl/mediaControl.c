#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <lcd.h>
#include "../shared/shared.h"
#include <string.h>
#include <math.h>
#include "../../lib/rotaryencoder.h"
#include <netdb.h>
#include <netinet/in.h>

appInfo getAppInfo() {
	appInfo appinfo;
	appinfo.name = "Media Control";
	appinfo.app_type = menu;
	return appinfo;
}

int sendCommand(char * command) {
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	char buffer[256];
	portno = 5000;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("ERROR opening socket\n");
		exit(1);
	}
	server = gethostbyname("192.168.1.117");
	if (server == NULL) {
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}
	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR connecting\n");
		exit(1);
	}
	/* Send message to the server */
	n = write(sockfd, command, strlen(command));
	if (n < 0) {
		perror("ERROR writing to socket\n");
		exit(1);
	}
	bzero(buffer, 256);
	n = read(sockfd, buffer, 255);
	if (n < 0) {
		perror("ERROR reading from socket\n");
		exit(1);
	}
	//printf("%s\n", buffer);
	return 0;
}

void appMain(arg_struct* args) {
	printf("Hello World!\n");
	lcdClear(args->fd);
	lcdPosition(args->fd, 2, 0);
	lcdPuts(args->fd, "SimpleMenu");
	lcdPosition(args->fd, 3, 1);
	lcdPuts(args->fd, "initialized");
	static unsigned char custChar[8] = {
		0b00100,
		0b00100,
		0b00000,
		0b00100,
		0b01110,
		0b11011,
		0b11011,
		0b10001
	};
	static unsigned char play[8] = {
		0b10000,
		0b11000,
		0b11100,
		0b11110,
		0b11110,
		0b11100,
		0b11000,
		0b10000
	};
	lcdCharDef(args->fd, 0, custChar);
	lcdCharDef(args->fd, 1, play);
	char valueStr[16] = "0";
	int lastValue = args->encoder->value;
	int lastLength = 1;
	int lastState = LOW;
	int lastState2 = LOW;
	int menuPos = 1;
	while (args->running) {
		if (digitalRead(BUTTON_A) == HIGH && lastState == LOW) {
			lastState = HIGH;
		}
		else if (lastState == HIGH && digitalRead(BUTTON_A) == LOW) {
			lcdPutchar(args->fd, (char)0);
			lastState = LOW;
			switch (args->encoder->value / 16 % 4) {
			case 0:sendCommand("play\n"); break;
			case 1:sendCommand("next\n"); break;
			case 2:sendCommand("prev\n"); break;
			case 3:sendCommand("switch\n"); break;
			}

		}
		if (digitalRead(BUTTON_B) == HIGH && lastState2 == LOW) {
			lastState2 = HIGH;
		}
		else if (lastState2 == HIGH && digitalRead(BUTTON_B) == LOW) {
			args->running = 0,
			lastState2 = LOW;
		}
		if (args->encoder->value / 16 % 4 != lastValue) {
			lcdPosition(args->fd, 0, 0);
			lcdPuts(args->fd, "                                ");
			switch (args->encoder->value / 16 % 4)
			{
			case 0:
				lcdPosition(args->fd, 3, 0);
				lcdPutchar(args->fd, (char)1);
				lcdPuts(args->fd, "Play/Pause");
				break;
			case 1:
				lcdPosition(args->fd, 6, 0);
				lcdPuts(args->fd, "Next");
				break;
			case 2:
				lcdPosition(args->fd, 4, 0);
				lcdPuts(args->fd, "Previous");
				break;
			case 3:
				lcdPosition(args->fd, 1, 0);
				lcdPuts(args->fd, "Switch Output");
				break;
			default:
				break;
			}
		}
		lastValue = args->encoder->value / 16 % 4;
		delay(1);
	}
}