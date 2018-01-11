PROG=		ssi

SRCS=		sh.c

CLFAGS+=	-g
CFLAGS+=	-O2 -pipe
CFLAGS+=	-Wall -Werror -Wextra -ansi -Wcast-qual -Wformat=2
CFLAGS+=	-Wmissing-declarations -pedantic -Wstrict-prototypes
CFLAGS+=	-Wpointer-arith -Wuninitialized -Wmissing-prototypes
CFLAGS+=	-Wsign-compare -Wshadow -Wdeclaration-after-statement
CFLAGS+=	-Wfloat-equal -Wcast-align -Wundef -Wstrict-aliasing=2

#PREFIX=	/usr/local
#CPPFLAGS+=	-I${PREFIX}/include
#LDFLAGS+=	-L${PREFIX}/lib
#LDFLAGS+=	-lereadline -ehistory -ltermcap

all: ${PROG}

${PROG}: ${SRCS}
	${CC} ${CFLAGS} ${CPPFLAGS} ${LDFLAGS} -o $@ ${SRCS}

clean:
	rm -f a.out [Ee]rrs mklog *.core *.o ${PROG}

.PHONY: all clean
