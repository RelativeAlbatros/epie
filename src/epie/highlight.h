#ifndef EPIE_HIGHLIGHT_H
#define EPIE_HIGHLIGHT_H

#include "erow.h"

#define HL_HIGHLIGHT_NUMBERS (1<<0)
#define HL_HIGHLIGHT_STRINGS (1<<1)
#define HL_HIGHLIGHT_FUNCTIONS (1<<2)

enum editorHighlight {
	HL_NORMAL = 0,
	HL_COMMENT,
	HL_MLCOMMENT,
	HL_FUNCTION,
	HL_KEYWORD1,
	HL_KEYWORD2,
	HL_STRING,
	HL_NUMBER,
	HL_MATCH
};

struct editorSyntax {
	char *filetype;
	char **filematch;
	char **keywords;
	char *singleline_comment_start;
	char *multiline_comment_start;
	char *multiline_comment_end;
	int flags;
};

static char *C_HL_extensions[] = { ".c", ".h", ".cpp", NULL };
static char *C_HL_keywords[] = {
	"switch", "if", "while", "for", "break", "continue", "return", "else", "default",
	"union", "case", "#include", "#ifndef", "#define", "#endif", "#pragma once", "namespace",
	"struct|", "typedef|", "enum|", "class|", "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
	"void|", "static|", "using|", "std", NULL
};
static struct editorSyntax HLDB[] = {
	{
		"c",
		C_HL_extensions,
		C_HL_keywords,
		"//", "/*", "*/",
		HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_FUNCTIONS
	},
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

int is_separator(int c);
void editorUpdateSyntax(erow *row);
int editorSyntaxToColor(int hl);
void editorSelectSyntaxHighlight(void);

#endif
