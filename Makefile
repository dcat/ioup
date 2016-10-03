CFLAGS  += -I/usr/local/include -std=c99 -Wall
LDFLAGS += -L/usr/local/lib -lcurl
PREFIX   = /usr/local
VERSION  = 2.0
RM       = /bin/rm

.SUFFIXES: .c .o

all: ioup

.c.o:
	@${CC} ${CFLAGS} -DVERSION=\"${VERSION}\" -c $<

ioup: ioup.o
	@${CC} ioup.o ${CFLAGS} ${LDFLAGS} -o $@

clean:
	${RM} *.o
	${RM} ioup

install:
	install -m755 ioup   ${PREFIX}/bin/ioup
	install -m644 ioup.1 ${PREFIX}/man/man1/ioup.1

uninstall:
	${RM} ${PREFIX}/bin/ioup
	${RM} ${PREFIX}/man/man1/ioup.1
