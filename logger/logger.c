#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "logger.h"

int log_format(const char* tag, const char* message, va_list args) {
	FILE *log = fopen(LOGFILE, "a");
	char *buf = malloc(80 * sizeof(char));

	time_t now;
	time(&now);
	char *date = ctime(&now);
	date[strlen(date) - 1] = '\0';

	if (snprintf(buf, 80, "[%s] at (%s): %s", tag, date, message) == -1) return -1;
	if (vfprintf(log, buf, args)) return -1;

	fclose(log);
	free(buf);
	return 0;
}

void log_error(const char* message, ...) {
	va_list args;
	va_start(args, message);
	log_format("ERROR", message, args);
	va_end(args);
}

void log_info(const char* message, ...) {
	va_list args;
	va_start(args, message);
	log_format("INFO", message, args);
	va_end(args);
}

void log_debug(const char* message, ...) {
	va_list args;
	va_start(args, message);
	log_format("DEBUG", message, args);
	va_end(args);
}
