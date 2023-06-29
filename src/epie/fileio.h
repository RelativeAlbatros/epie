#ifndef FILEIO_H
#define FILEIO_H

char *editorRowsToString(int *buflen);
void editorConfigSource(void);
void editorOpen(char *filename);
int editorSave(void);

#endif
