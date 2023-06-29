#ifndef INPUT_H
#define INPUT_H

#define EPIE_QUIT_TIMES 3

#define CTRL_KEY(k) ((k) & 0x1f)

char *editorPrompt(char *prompt, void (*callback)(char *, int));
void editorMoveCursor(int key);
void editorProcessKeypress();

#endif
