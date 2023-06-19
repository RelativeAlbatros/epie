CC := gcc
CFLAGS := -Wall -Wextra -pedantic -std=c99

all: options kilo

options:
	@echo dwm build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

kilo: kilo.c
	$(CC) $? -o $@ ${FLAGS}

clean:
	rm kilo

install: all
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp kilo ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/kilo
	@echo "installed kilo in ${DESTDIR}${PREFIX}/bin"

uninstall:
	rm -f ${PREFIX}/bin/kilo
