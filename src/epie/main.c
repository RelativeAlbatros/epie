#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>

#include "erow.h"
#include "editor.h"
#include "highlight.h"
#include "fileio.h"
#include "output.h"
#include "input.h"
#include "find.h"
#include "logger.h"

int main(int argc, char *argv[]) {
	initscr();
	initEditor();
	editorConfigSource();

	if (argc >= 2) {
		editorOpen(argv[1]);
	}

	editorSetStatusMessage("HELP: Ctrl-Q: Quit | Ctrl-S: Save");

	while (1) {
		editorRefreshScreen();
		editorProcessKeypress();
	}

	endwin();
	return 0;
}
