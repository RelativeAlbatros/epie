#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "../toml/toml.h"

#include "terminal.h"
#include "erow.h"
#include "editor.h"
#include "highlight.h"

#define EPIE_VERSION "0.1.4"
#define EPIE_QUIT_TIMES 3

#define CTRL_KEY(k) ((k) & 0x1f)

// row operations
// editor operations
static void editorInsertChar(int c);
static void editorInsertNewLine();
static void editorDelChar();
// file i/o
static char *editorRowsToString(int *buflen);
static void editorOpen(char *filename);
static int editorSave();
// find
static void editorFindCallback(char *query, int key);
static void editorFind();
// output
static void editorScroll();
static void editorRefreshScreen();
static void editorSetStatusMessage(const char* fmt, ...);
// input
static char *editorPrompt(char *prompt, void (*callback)(char *, int));
static void editorMoveCursor(int key);
static void editorProcessKeypress();


void editorInsertChar(int c) {
	if (E.cy == E.numrows) {
		editorInsertRow(E.numrows, "", 0);
	}
	editorRowInsertChar(&E.row[E.cy], E.cx, c);
	E.cx++;
}

void editorInsertNewLine() {
	int tabs = 0;
	if (E.cx == 0) {
		editorInsertRow(E.cy, "", 0);
	} else {
		erow *row = &E.row[E.cy];
		editorInsertRow(E.cy+1, &row->chars[E.cx], row->size - E.cx);
		for (int i = 0; i < row->size; i++) {
			if (row->chars[i] != '\t') {
				break;
			} else {
				editorRowInsertChar(&E.row[E.cy+1], 0, '\t');
				tabs++;
			}
		}
		row = &E.row[E.cy];
		row->size = E.cx;
		row->chars[row->size] = '\0';
		editorUpdateRow(row);
	}
	E.cy++;
	E.cx = tabs;
	E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
}

void editorDelChar() {
	if (E.cy == E.numrows) return;
	if (E.cx == 0 && E.cy == 0) return;

	erow *row = &E.row[E.cy];
	if (E.cx > 0) {
		editorRowDelChar(row, E.cx - 1);
		E.cx--;
	} else {
		E.cx = E.row[E.cy - 1].size;
		editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
		editorDelRow(E.cy);
		E.cy--;
	}
}


char *editorRowsToString(int *buflen) {
	int totlen = 0;
	int j;
	for (j = 0; j < E.numrows; j++)
		totlen += E.row[j].size + 1;
	*buflen = totlen;

	char *buf = malloc(totlen);
	char *p = buf;
	for (j = 0; j < E.numrows; j++) {
		memcpy(p, E.row[j].chars, E.row[j].size);
		p += E.row[j].size;
		*p = '\n';
		p++;
	}

	return buf;
}

void editorConfigSource() {
	FILE *config;
	char *path = malloc(80);
	strcpy(path, getenv("HOME"));
	strcat(path, E.config_path);
	strcat(path, "/settings.toml");
	char errbuf[200];

	if ((config = fopen(path, "r")) == NULL) return;
	toml_table_t *conf = toml_parse_file(config, errbuf, sizeof(errbuf));
	fclose(config);
	if (!conf) return;

	toml_table_t *settings       = toml_table_in(conf,      "settings");
	toml_datum_t number          = toml_bool_in(settings,   "number");
	toml_datum_t numberlen       = toml_int_in(settings,    "numberlen");
	toml_datum_t message_timeout = toml_int_in(settings,    "message-timeout");
	toml_datum_t tab_stop        = toml_int_in(settings,    "tab-stop");
	toml_datum_t separator       = toml_string_in(settings, "separator");

	if (number.ok && E.number)
		E.number          = number.u.b;
	if (E.number) {
		if (numberlen.ok)
			E.numberlen   = numberlen.u.i;
	} else {
		E.numberlen = 0;
	}
	E.line_indent         = E.numberlen + 2;

	if (message_timeout.ok)
		E.message_timeout = message_timeout.u.i;
	if (tab_stop.ok && tab_stop.u.i != 0)
		E.tab_stop        = tab_stop.u.i;
	if (separator.ok)
		E.separator =  separator.u.s[0];

	free(separator.u.s);
}

void editorOpen(char *filename) {
	free(E.filename);
    strcpy(E.filename, filename);
	// ignore path starting with ./ and ../
	while (*(E.filename + 1) == '/') E.filename += 2;
	while (*(E.filename + 1) == '.') E.filename += 3;

	editorSelectSyntaxHighlight();

	FILE *fp = fopen(filename, "r");
	if (!fp) die("fopen");

	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	while ((linelen = getline(&line, &linecap, fp)) != -1) {
		while (linelen > 0 && (line[linelen - 1] == '\n' ||
					line[linelen - 1] == '\r'))
			linelen--;
		editorInsertRow(E.numrows, line, linelen);
	}
	free(line);
	fclose(fp);
	E.dirty = 0;
}

int editorSave() {
	if (E.filename == NULL) {
		if ((E.filename = editorPrompt("Save as: %s", NULL)) == NULL) {
			editorSetStatusMessage("Save aborted.");
			return -1;
		}
		editorSelectSyntaxHighlight();
	}

	int len;
	char *buf = editorRowsToString(&len);

	int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
	if (fd != -1) {
		if (ftruncate(fd, len) != -1) {
			if (write(fd, buf, len) == len) {
				close(fd);
				free(buf);
				E.dirty = 0;
				editorSetStatusMessage("\"%s\" %dL, %dB Written", E.filename, E.screencols, len);
				return 0;
			}
		}
		close(fd);
	}

	free(buf);
	editorSetStatusMessage("failed to save %s: %s", E.filename, strerror(errno));
	return -1;
}


void editorFindCallback(char *query, int key) {
	static int last_match = -1;
	static int direction = 1;

	static int saved_hl_line;
	static char *saved_hl = NULL;

	if (saved_hl) {
		memcpy(E.row[saved_hl_line].hl, saved_hl, E.row[saved_hl_line].rsize);
		free(saved_hl);
		saved_hl = NULL;
	}
	if (key == '\r' || key == '\x1b') {
		last_match = -1;
		direction = 1;
		return;
	} else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
		direction = 1;
	} else if (key == ARROW_LEFT || key == ARROW_UP) {
		direction = -1;
	} else {
		last_match = -1;
		direction = 1;
	}

	if (last_match == -1) direction = 1;
	int current = last_match;
	int i;
	for (i = 0; i < E.numrows; i++) {
		current += direction;
		if (current == -1) current = E.numrows - 1;
		else if (current == E.numrows) current = 0;

		erow *row = &E.row[current];
		char *match = strstr(row->render, query);
		if (match) {
			last_match = current;
			E.cy = current;
			E.cx = editorRowRxToCx(row, match - row->render);
			E.rowoff = E.numrows;

			saved_hl_line = current;
			saved_hl = malloc(row->rsize);
			memcpy(saved_hl, row->hl, row->rsize);
			memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
			break;
		}
	}
}

void editorFind() {
	int saved_cx = E.cx;
	int saved_cy = E.cy;
	int saved_coloff = E.coloff;
	int saved_rowoff = E.rowoff;
	int saved_mode = E.mode;

	E.mode = SEARCH;
	char *query = editorPrompt("/%s", editorFindCallback);
	if (query) {
		free(query);
	} else {
		E.cx = saved_cx;
		E.cy = saved_cy;
		E.coloff = saved_coloff;
		E.rowoff = saved_rowoff;
	}
	E.mode = saved_mode;
}


struct abuf {
	char *b;
	int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
	char *new = realloc(ab->b, ab->len + len);

	if (new == NULL) return;
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

void abFree(struct abuf *ab) {
	free(ab->b);
}

void editorScroll() {
	E.rx = E.line_indent;
	if (E.cy < E.numrows) {
		E.rx = editorRowCxToRx(&E.row[E.cy], E.cx) + E.line_indent;
	}
	if (E.cy < E.rowoff) {
		E.rowoff = E.cy;
	}
	if (E.cy >= E.rowoff + E.screenrows) {
		E.rowoff = E.cy - E.screenrows + 1;
	}
	if (E.rx < E.coloff) {
		E.coloff = E.rx;
	}
	if (E.rx >= E.coloff + E.screencols) {
		E.coloff = E.rx - E.screencols + 1;
	}
}


void editorDrawRows(struct abuf *ab) {
	int y;
	for (y = 0; y < E.screenrows; y++) {
		int filerow = y + E.rowoff;
		if (filerow >= E.numrows) {
			if (E.numrows == 0 && y == E.screenrows / 3) {
				char welcome[80];
				int welcomelen = snprintf(welcome, sizeof(welcome),
				"epie editor -- version %s", EPIE_VERSION);
				if (welcomelen > E.screencols) welcomelen = E.screencols;
				int padding = (E.screencols - welcomelen) / 2;
				if (padding) {
					abAppend(ab, "\x1b[90m", 5);
					for (int pad=0; pad<E.line_indent-2; pad++) abAppend(ab, " ", 1);
					abAppend(ab, "~", 1);
					abAppend(ab, " ", 1);
					abAppend(ab, "\x1b[m", 3);
					padding--;
				}
				while (padding--) abAppend(ab, " ", 1);
				abAppend(ab, welcome, welcomelen);
			} else {
				abAppend(ab, "\x1b[90m", 5);
				for (int pad=0; pad<E.line_indent-2; pad++) abAppend(ab, " ", 1);
				abAppend(ab, "~", 1);
				abAppend(ab, " ", 1);
				abAppend(ab, "\x1b[m", 3);
			}
		} else {
			if (E.number) {
				char rcol[80];
				E.line_indent = snprintf(rcol, sizeof(rcol), " %*d ",
						E.numberlen, filerow + 1);
				abAppend(ab, "\x1b[90m", 5);
				abAppend(ab, rcol, E.line_indent);
				abAppend(ab, "\x1b[m", 3);
			} else {
				abAppend(ab, "  ", 2);
			}
			int len = E.row[filerow].rsize - E.coloff;
			if (len < 0) len = 0;
			if (len > E.screencols) len = E.screencols;
			char *c = &E.row[filerow].render[E.coloff];
			unsigned char *hl = &E.row[filerow].hl[E.coloff];
			int current_color = -1;
			int j;
			for (j = 0; j < len; j++) {
				if (iscntrl(c[j])){
					char sym = (c[j] <= 26) ? '@' + c[j] : '?';
					abAppend(ab, "\x1b[7m", 4);
					abAppend(ab, &sym, 1);
					abAppend(ab, "\x1b[m", 3);
					if (current_color != -1) {
						char buf[16];
						int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
						abAppend(ab, buf, clen);
					}
				} else if (hl[j] == HL_COMMENT) {
					abAppend(ab, "\x1b[2m", 4);
					abAppend(ab, &c[j], 1);
					abAppend(ab, "\x1b[0m", 4);
				} else if (hl[j] == HL_NORMAL) {
					if (current_color != -1) {
						abAppend(ab, "\x1b[39m", 5);
						abAppend(ab, "\x1b[49m", 5);
						current_color = -1;
					}
					abAppend(ab, &c[j], 1);
				} else {
					int color = editorSyntaxToColor(hl[j]);
					if (color != current_color) {
						current_color = color;
						char buf[16];
						int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
						abAppend(ab, buf, clen);
					}
					abAppend(ab, &c[j], 1);
				}
			}
			abAppend(ab, "\x1b[39m", 5);
			abAppend(ab, "\x1b[49m", 5);
		}

		abAppend(ab, "\x1b[K", 3);
		abAppend(ab, "\r\n", 2);
	}

}

void editorDrawStatusBar(struct abuf *ab) {
	abAppend(ab, "\x1b[40m", 5);

	char status[80], rstatus[80];
	char *e_mode;
	if (E.mode == NORMAL)	    e_mode = "NORMAL";
	else if (E.mode == INPUT)   e_mode = "INSERT";
	else if (E.mode == COMMAND) e_mode = "COMMAND";
	else if (E.mode == SEARCH)  e_mode = "SEARCH";
	int len = snprintf(status, sizeof(status), " %s %c %.20s%s- %d",
			e_mode , E.separator,
			E.filename ? E.filename : "[No Name]",
			E.dirty ? " [+] " : " ",
			E.numrows);
	int rlen = snprintf(rstatus, sizeof(rstatus), "%c %s %c %d/%d ",
			last_input_char,
			E.syntax ? E.syntax->filetype : "no ft", E.separator,
			E.cy + 1, E.numrows);
	if (len > E.screencols) len = E.screencols;
	abAppend(ab, status, len);
	while (len < E.screencols) {
		if (E.screencols - len == rlen) {
			abAppend(ab, rstatus, rlen);
			break;
		} else {
			abAppend(ab, " ", 1);
			len++;
		}
	}
	abAppend(ab, "\x1b[m", 3);
	abAppend(ab, "\r\n", 2);
}

void editorDrawMessageBar(struct abuf *ab) {
	abAppend(ab, "\x1b[K", 3);
	int msglen = strlen(E.statusmsg);
	if (msglen > E.screencols) msglen = E.screencols;
	if (msglen && time(NULL) - E.statusmsg_time < E.message_timeout)
		abAppend(ab, E.statusmsg, msglen);
}


void editorRefreshScreen() {
	editorScroll();

	struct abuf ab = ABUF_INIT;

	abAppend(&ab, "\x1b[?25l", 6);
	abAppend(&ab, "\x1b[H", 3);

	editorDrawRows(&ab);
	editorDrawStatusBar(&ab);
	editorDrawMessageBar(&ab);

	/* return cursor to init position */
	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH",
			(E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);
	abAppend(&ab, buf, strlen(buf));

	abAppend(&ab, "\x1b[?25h", 6);

	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
	va_end(ap);
	E.statusmsg_time = time(NULL);
}


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
		}else if (c == '\x1b'){
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



int main(int argc, char *argv[]) {
	enableRawMode();
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

	quit();
}
