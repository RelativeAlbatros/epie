#ifndef EDITOR_H
#define EDITOR_H

#define EPIE_VERSION "0.1.4"

#include <time.h>

#include "erow.h"

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

enum editorMode {
	NORMAL = 0,
	INPUT,
	COMMAND,
	SEARCH
};

struct editorConfig {
	unsigned short int number;
	unsigned short int numberlen;
	unsigned short int line_indent;
	unsigned short int message_timeout;
	unsigned short int tab_stop;
	char separator;
	char statusmsg[80];
	time_t statusmsg_time;
	erow *row;
	struct editorSyntax *syntax;
	char *config_path;
	unsigned short int mode;
	unsigned short int cx, cy;
	unsigned short int rx;
	unsigned short int rowoff;
	unsigned short int coloff;
	int screenrows;
	int screencols;
	unsigned short int numrows;
	unsigned short int dirty;
	char *filename;
};

extern struct editorConfig E;

extern int last_input_char;

void die(const char*);
void initEditor(void);
void editorInsertChar(int c);
void editorInsertNewLine();
void editorDelChar();

#endif
