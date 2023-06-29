#ifndef OUTPUT_H
#define OUTPUT_H

#include "ab.h"

void editorDrawRows(struct abuf *ab);
void editorDrawStatusBar(struct abuf *ab);
void editorDrawMessageBar(struct abuf *ab);
void editorScroll();
void editorRefreshScreen();
void editorSetStatusMessage(const char* fmt, ...);

#endif
