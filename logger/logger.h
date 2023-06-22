#ifndef LOGGER_H
#define LOGGER_H

#define LOGFILE "log.txt"

int log_format(const char* tag, const char* message, va_list args);
void log_error(const char* message, ...);
void log_info(const char* message, ...);
void log_debug(const char* message, ...);

#endif
