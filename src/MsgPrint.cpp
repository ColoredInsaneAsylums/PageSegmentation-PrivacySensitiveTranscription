#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include "MsgPrint.h"

void MsgPrint::msgPrint(SEVERE s, const char *msg) {
	time_t rawTime;
	char timeStr[10];
	struct tm * timeInfo;

	time(&rawTime);
	timeInfo = localtime(&rawTime);
	strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeInfo);
	char typeStr[5];

	switch(s) {
		case INFO:
			strcpy(typeStr, "INFO");
			break;
		case WARN:
			strcpy(typeStr, "WARN");
			break;
		case ERR:
			strcpy(typeStr, "ERR");
			break;
	}
	fprintf(stderr, "[%s][%s] %s\n", typeStr, timeStr, msg);
	if (s == ERR)
		exit(1);
}
