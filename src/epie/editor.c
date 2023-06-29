#include "terminal.h"

#include "editor.h"

struct editorConfig E;

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

	if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
	E.screenrows -= 2;
}

