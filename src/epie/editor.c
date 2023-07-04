#include "editor.h"

struct editorConfig E;

int last_input_char = 0;

void die(const char *msg) {
	perror(msg);
	exit(1);
}
void initEditor(void) {
	E.number          = 1;
	E.numberlen       = 4;
	E.line_indent     = E.numberlen + 2;
	E.message_timeout = 5;
	E.tab_stop        = 4;
	E.separator       = '|';
	E.statusmsg[0]    = '\0';
	E.statusmsg_time  = 0;
	E.row             = NULL;
	E.syntax          = NULL;

	E.config_path     = "/.config/epie";
	E.mode            = NORMAL;
	E.cx              = 0;
	E.cy              = 0;
	E.rx              = E.line_indent;
	E.rowoff          = 0;
	E.coloff          = 0;
	E.numrows         = 0;
	E.dirty           = 0;
	E.filename        = NULL;

	E.screenrows -= 2;
}

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

