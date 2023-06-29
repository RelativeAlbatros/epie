#include <stdlib.h>
#include <ctype.h>

#include "fileio.h"
#include "editor.h"
#include "find.h"
#include "output.h"
#include "input.h"
#include "terminal.h"
#include "logger.h"

char *editorPrompt(char *prompt, void (*callback)(char *, int)) {
	size_t bufsize = 128;
	char *buf = malloc(bufsize);

	size_t buflen = 0;
	buf[0] = '\0';

	while (1) {
		editorSetStatusMessage(prompt, buf);
		editorRefreshScreen();

		int c = editorReadKey();
		if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
			if(buflen != 0) buf[--buflen] = '\0';
		} else if (c == '\x1b'){
			editorSetStatusMessage("");
			if (callback) callback(buf, c);
			free(buf);
			return NULL;
		} else if (c == '\r') {
			if (buflen != 0) {
				editorSetStatusMessage("");
				if (callback) callback(buf, c);
				return buf;
			}
		} else if (!iscntrl(c) && c < 128) {
			if (buflen == bufsize - 1) {
				bufsize *= 2;
				buf = realloc(buf, bufsize);
			}
			buf[buflen++] = c;
			buf[buflen] = '\0';
		}
		if (callback) callback(buf, c);
	}
}

void editorMoveCursor(int key) {
	erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

	switch(key) {
		case 'h':
		case ARROW_LEFT:
			if (E.cx != 0) {
				E.cx--;
			} else if (E.cy > 0) {
				E.cy--;
				E.cx = E.row[E.cy].size;
			}
			break;
		case 'l':
		case ARROW_RIGHT:
			if (row && E.cx < row->size - 1)
				E.cx++;
			break;
		case 'k':
		case ARROW_UP:
			if (E.cy != 0) {
				E.cy--;
			}
			break;
		case 'j':
		case ARROW_DOWN:
			if (E.mode == INPUT && E.cy < E.numrows) {
				E.cy++;
			} else if (E.mode == NORMAL && E.cy < E.numrows - 1) {
				E.cy++;
			}
			break;
	}

	row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
	int rowlen = row ? row->size : 0;
	if (E.cx > rowlen) {
		E.cx = rowlen;
	}
}

void editorProcessKeypress() {
	static int quit_times = EPIE_QUIT_TIMES;

	int c = editorReadKey();

	if (E.mode == NORMAL) {
		switch (c) {
			case CTRL_KEY('q'):
				if (E.dirty && quit_times > 0) {
					editorSetStatusMessage("Error: no write since last change, press Ctrl-Q %d more times to quit.", quit_times);
					quit_times--;
				return;
				}
				quit();
				break;

			case CTRL_KEY('s'):
				editorSave();
				break;

			case '/':
				editorFind();
				break;

			case 'i':
				E.mode = INPUT;
				break;

			case 'I':
				E.cx = 0;
				E.mode = INPUT;
				break;

			case 'a':
				if (E.cx != 0) {
					E.cx += 1;
				}
				E.mode = INPUT;
				break;

			case 'A':
				E.cx = E.row[E.cy].size;
				E.mode = INPUT;
				break;

			case 'o':
				editorInsertRow(E.cy + 1, "", 0);
				editorMoveCursor(ARROW_DOWN);
				E.cx = 0;
				E.mode = INPUT;
				break;

			case 'O':
				editorInsertRow(E.cy, "", 0);
				E.cx = 0;
				E.mode = INPUT;
				break;

			case 'x':
				editorRowDelChar(&E.row[E.cy], E.cx);
				break;

			case 'e': {
				char *filename = editorPrompt("file: ", NULL);
				logger(1, "%s", filename);
				editorOpen(filename); break;
			}

			case 'r': {
				int c = editorReadKey();
				editorRowDelChar(&E.row[E.cy], E.cx);
				editorInsertChar(c);
				editorMoveCursor(ARROW_LEFT);
				break;
			}

			case 'd': {
				int c = editorReadKey();
				if (c == 'd') {
					editorDelRow(E.cy);
				} else if (c == 'k') {
					editorDelRow(E.cy);
					editorDelRow(E.cy - 1);
					editorMoveCursor(ARROW_UP);
				} else if (c == 'j') {
					editorDelRow(E.cy + 1);
					editorDelRow(E.cy);
				} else if (c == 'l') {
					editorRowDelChar(&E.row[E.cy], E.cx+1);
					editorMoveCursor(ARROW_LEFT);
				} else if (c == 'h') {
					editorRowDelChar(&E.row[E.cy], E.cx-1);
				} else if (c == 'w') {
					while (E.row[E.cy].chars[E.cx] != ' ' && E.cx != E.row[E.cy].size)
					editorRowDelChar(&E.row[E.cy], E.cx);
				}
				break;
			}

			case BACKSPACE:
				editorMoveCursor(ARROW_RIGHT);
				break;

			case CTRL_KEY('h'):
			case DEL_KEY:
				editorDelChar();
				break;

			case '\r':
				editorMoveCursor(ARROW_DOWN);
				break;

			case 'h':
			case 'j':
			case 'k':
			case 'l':
			case ARROW_LEFT:
			case ARROW_DOWN:
			case ARROW_UP:
			case ARROW_RIGHT:
				editorMoveCursor(c);
				break;

			case 'g': E.cy = 0; break;
			case 'G': E.cy = E.numrows; break;

			case '>':
				editorRowInsertChar(&E.row[E.cy], 0, '\t');
				editorMoveCursor(ARROW_RIGHT);
				break;

			case '<':
				if (E.row[E.cy].chars[0] == '\t') {
					editorRowDelChar(&E.row[E.cy], 0);
					editorMoveCursor(ARROW_LEFT);
				}
				break;

			case HOME_KEY: E.cx = 0; break;
			case END_KEY:
				if (E.cy < E.numrows)
					E.cx = E.row[E.cy].size;
				break;

			case PAGE_UP:
			case PAGE_DOWN: {
				if (c == PAGE_UP) {
					E.cy = E.rowoff;
				} else if (c == PAGE_DOWN) {
					E.cy = E.rowoff + E.screenrows - 1;
					if (E.cy > E.numrows) E.cy = E.numrows;
				}

				int times = E.screenrows;
				while (times--) {
					editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
				}
				break;
			}

			case CTRL_KEY('l'):
			case '\x1b':
				break;

			default:
				break;
		}
	} else if (E.mode == INPUT) {
		switch (c) {
			case CTRL_KEY('l'):
			case '\x1b':
				if (E.cx != 0) editorMoveCursor(ARROW_LEFT);
				E.mode = NORMAL;
				break;

			case CTRL_KEY('q'):
				if (E.dirty && quit_times > 0) {
					editorSetStatusMessage("Error: no write since last change, press Ctrl-Q %d more times to quit.", quit_times);
					quit_times--;
					return;
				}
				quit();
				break;

			case DEL_KEY:
			case BACKSPACE:
				if (c == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
				editorDelChar();
				break;

			case '\r':
				editorInsertNewLine();
				break;

			case ARROW_LEFT:
			case ARROW_DOWN:
			case ARROW_UP:
			case ARROW_RIGHT:
				editorMoveCursor(c);
				break;

			default:
				if ((c > 31 && c < 127) || c == 9)
					editorInsertChar(c);
				break;
		}
	}

	quit_times = EPIE_QUIT_TIMES;
}

