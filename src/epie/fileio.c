#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include "../toml/toml.h"
#include "editor.h"
#include "fileio.h"
#include "highlight.h"
#include "input.h"
#include "output.h"

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
	E.filename = malloc(strlen(filename) + 1);
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

int editorSave(void) {
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

