#ifndef TERMINAL_H
#define TERMINAL_H

#include <errno.h>
#ifndef WIN32
#	include <sys/ioctl.h>
#	include <termios.h>
#else
#	include <windows.h>
#   include "../termios/termios.h"
#endif

extern int last_input_char;

enum editorKey {
	BACKSPACE = 127,
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN
};


void logger(const int tag, const char *msg, ...);
void quit(void);
void die(const char *s);
void disableRawMode(void);
void enableRawMode(void);
int editorReadKey(void);
#ifndef _WIN32
int getCursorPosition(int *rows, int *cols);
#endif
int getWindowSize(int *rows, int *cols);

#endif
