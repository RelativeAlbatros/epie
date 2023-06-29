#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include "editor.h"
#include "highlight.h"
#include "ab.h"
#include "output.h"

void editorDrawRows(struct abuf *ab) {
	int y;
	for (y = 0; y < E.screenrows; y++) {
		int filerow = y + E.rowoff;
		if (filerow >= E.numrows) {
			if (E.numrows == 0 && y == E.screenrows / 3) {
				char welcome[80];
				int welcomelen = snprintf(welcome, sizeof(welcome),
				"epie editor");
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
				for (int pad=0; pad < E.line_indent-2; pad++) abAppend(ab, " ", 1);
				abAppend(ab, "~", 1);
				abAppend(ab, " ", 1);
				abAppend(ab, "\x1b[m", 3);
			}
		} else {
			if (E.number) {
				char rcol[80];
				E.line_indent = snprintf(rcol, sizeof(rcol), " %*d ",
						E.numberlen, filerow + 1);
				if (y == E.cy)
					abAppend(ab, "\x1b[1m", 4);
				else
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
	abAppend(ab, "\x1b[7m", 4);

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

