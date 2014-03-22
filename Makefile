CFLAGS  += -I/usr/local/include -std=c99 -Wall
LDFLAGS += -L/usr/local/lib -lcurl
PREFIX   = /usr/local
VERSION  = 1.7
RM       = /bin/rm

ioup:
	@${CC} -o $@ -DVERSION=\"${VERSION}\" ioup.c ${CFLAGS} ${LDFLAGS}

clean:
	${RM} ioup
	${RM} *.o

install:
	install -m755 ioup   ${PREFIX}/bin/ioup
	install -m644 ioup.1 ${PREFIX}/man/man1/ioup.1

uninstall:
	${RM} ${PREFIX}/bin/ioup
	${RM} ${PREFIX}/man/man1/ioup.1
