#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "terminal.h"

#define EPIE_LOG_PATH "/tmp/epie.log"

void logger(const int tag, const char *msg, ...) {
	va_list args;
	va_start(args, msg);
	FILE *log = fopen(EPIE_LOG_PATH, "a");
	if (log == NULL) die("log");
	char message[256];
	char tag_type[16];
	time_t now;
	time(&now);

	switch(tag) {
		case (0) : strcpy(tag_type, "INFO");       break;
		case (1) : strcpy(tag_type, "DEBUG");      break;
		case (2) : strcpy(tag_type, "ERROR!");     break;
		case (3) : strcpy(tag_type, "CRITICAL!!"); break;
	}
	char *date = ctime(&now);
	date[strlen(date) - 1] = '\0';
	snprintf(message, sizeof(message), "[%s] at (%s): %s",
			tag_type, date, msg);
	message[strlen(message)] = '\n';
	vfprintf(log, message, args);

	va_end(args);
	fclose(log);
}

