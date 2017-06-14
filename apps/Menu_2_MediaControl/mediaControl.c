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

int sendCommand(char * command, char* buffer) {
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	//char buffer[256];
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
	lcdClear(args->fd);
	static unsigned char arrowRight[8] = {
		0b00000,
		0b01000,
		0b01100,
		0b01110,
		0b01110,
		0b01100,
		0b01000,
		0b00000
	};
	static unsigned char arrowRightInverted[8] = {
		0b11111,
		0b10111,
		0b10011,
		0b10001,
		0b10001,
		0b10011,
		0b10111,
		0b11111
	};
	static unsigned char  arrowLeft[8] = {
		0b00000,
		0b00010,
		0b00110,
		0b01110,
		0b01110,
		0b00110,
		0b00010,
		0b00000
	};
	static unsigned char arrowLeftInverted[8] = {
		0b11111,
		0b11101,
		0b11001,
		0b10001,
		0b10001,
		0b11001,
		0b11101,
		0b11111
	};
	static unsigned char pause[8] = {
		0b00000,
		0b00000,
		0b01010,
		0b01010,
		0b01010,
		0b01010,
		0b00000,
		0b00000
	};
	static unsigned char pauseInverted[8] = {
		0b11111,
		0b11111,
		0b10101,
		0b10101,
		0b10101,
		0b10101,
		0b11111,
		0b11111
	};
	static unsigned char speakerSymbol[8] = {
		0b00010,
		0b00110,
		0b11110,
		0b11110,
		0b11110,
		0b11110,
		0b00110,
		0b00010
	};
	static unsigned char speakerSymbolInverted[8] = {
		0b11101,
		0b11001,
		0b00001,
		0b00001,
		0b00001,
		0b00001,
		0b11001,
		0b11101
	};
#define ARROW_RIGHT 0
#define ARROW_RIGHT_INVERTED 1
#define ARROW_LEFT 2
#define ARROW_LEFT_INVERTED 3
#define PAUSE 4
#define PAUSE_INVERTED 5
#define SPEAKER_SYMBOL 6
#define SPEAKER_SYMBOL_INVERTED 7
	
	lcdCharDef(args->fd, ARROW_RIGHT, arrowRight);
	lcdCharDef(args->fd, ARROW_RIGHT_INVERTED, arrowRightInverted);
	lcdCharDef(args->fd, ARROW_LEFT, arrowLeft);
	lcdCharDef(args->fd, ARROW_LEFT_INVERTED, arrowLeftInverted);
	lcdCharDef(args->fd, PAUSE, pause);
	lcdCharDef(args->fd, SPEAKER_SYMBOL, speakerSymbol);
	lcdCharDef(args->fd, SPEAKER_SYMBOL_INVERTED, speakerSymbolInverted);

	enum controlElement {
		previousTrack,
		playPause,
		nextTrack,
		switchAudio
	};

	int currentControlElement = playPause;
	int lastValue = args->encoder->value;
	int lastState = LOW;
	int lastState2 = LOW;
	int menuPos = 1;
	char Title[256] = { 0 };
	int offset = 0;
	int delayCounter = 0;
	while (args->running) {
		if (digitalRead(BUTTON_A) == HIGH && lastState == LOW) {
			lastState = HIGH;
		}
		else if (lastState == HIGH && digitalRead(BUTTON_A) == LOW) {
			lastState = LOW;
			switch (currentControlElement) {
			case playPause    : sendCommand("play\n"  , (char *)&Title);  break;
			case nextTrack    : sendCommand("next\n"  , (char *)&Title);  break;
			case previousTrack: sendCommand("prev\n"  , (char *)&Title);  break;
			case switchAudio  : sendCommand("switch\n", (char *)&Title); break;
			}
			printf("buffer: %s\n", Title);
			lcdPosition(args->fd, 0, 0);
			lcdPuts(args->fd, "                ");
		}
		if (digitalRead(BUTTON_B) == HIGH && lastState2 == LOW) {
			lastState2 = HIGH;
		}
		else if (lastState2 == HIGH && digitalRead(BUTTON_B) == LOW) {
			args->running = 0,
			lastState2 = LOW;
		}
		if (args->encoder->value / 16  != lastValue) {
			if (args->encoder->value / 16  > lastValue) {
				if (currentControlElement != switchAudio)
					currentControlElement++;
			}
			else {
				if (currentControlElement != previousTrack)
					currentControlElement--;
			}
			switch (currentControlElement)
			{
			case previousTrack:
				lcdPosition(args->fd, 3, 1);
				lcdPutchar(args->fd, (char)ARROW_LEFT_INVERTED);
				lcdPutchar(args->fd, (char)ARROW_LEFT_INVERTED);
				lcdPutchar(args->fd, (char)254);
				lcdPutchar(args->fd, (char)ARROW_RIGHT);
				lcdPutchar(args->fd, (char)PAUSE);
				lcdPutchar(args->fd, (char)254);
				lcdPutchar(args->fd, (char)ARROW_RIGHT);
				lcdPutchar(args->fd, (char)ARROW_RIGHT);
				lcdPutchar(args->fd, (char)254);
				lcdPutchar(args->fd, (char)SPEAKER_SYMBOL);
				break;
			case playPause:
				lcdPosition(args->fd, 3, 1);
				lcdPutchar(args->fd, (char)ARROW_LEFT);
				lcdPutchar(args->fd, (char)ARROW_LEFT);
				lcdPutchar(args->fd, (char)254);
				lcdPutchar(args->fd, (char)ARROW_RIGHT_INVERTED);
				lcdPutchar(args->fd, (char)PAUSE_INVERTED);
				lcdPutchar(args->fd, (char)254);
				lcdPutchar(args->fd, (char)ARROW_RIGHT);
				lcdPutchar(args->fd, (char)ARROW_RIGHT);
				lcdPutchar(args->fd, (char)254);
				lcdPutchar(args->fd, (char)SPEAKER_SYMBOL);
				break;
			case nextTrack:
				lcdPosition(args->fd, 3, 1);
				lcdPutchar(args->fd, (char)ARROW_LEFT);
				lcdPutchar(args->fd, (char)ARROW_LEFT);
				lcdPutchar(args->fd, (char)254);
				lcdPutchar(args->fd, (char)ARROW_RIGHT);
				lcdPutchar(args->fd, (char)PAUSE);
				lcdPutchar(args->fd, (char)254);
				lcdPutchar(args->fd, (char)ARROW_RIGHT_INVERTED);
				lcdPutchar(args->fd, (char)ARROW_RIGHT_INVERTED);
				lcdPutchar(args->fd, (char)254);
				lcdPutchar(args->fd, (char)SPEAKER_SYMBOL);
				break;
			case switchAudio:
				lcdPosition(args->fd, 3, 1);
				lcdPutchar(args->fd, (char)ARROW_LEFT);
				lcdPutchar(args->fd, (char)ARROW_LEFT);
				lcdPutchar(args->fd, (char)254);
				lcdPutchar(args->fd, (char)ARROW_RIGHT);
				lcdPutchar(args->fd, (char)PAUSE);
				lcdPutchar(args->fd, (char)254);
				lcdPutchar(args->fd, (char)ARROW_RIGHT);
				lcdPutchar(args->fd, (char)ARROW_RIGHT);
				lcdPutchar(args->fd, (char)254);
				lcdPutchar(args->fd, (char)SPEAKER_SYMBOL_INVERTED);
				break;
			default:
				break;
			}
		}
		lastValue = args->encoder->value / 16 ;
		
		lcdPosition(args->fd, 0, 0);
		int i;
		delayCounter++;
		if (delayCounter % 50 == 49)
		{
			offset++;
			if (offset > strlen(Title)) {
				offset = 0;
			}
		}
		for (i = 0; i<16; i++) {
			if (Title[i + offset] != 0)
				lcdPutchar(args->fd, Title[i + offset]);
			else
				lcdPutchar(args->fd, (char)254);
		}
		delay(1);
	}
}