PROG=		ssi

SRCS=		sh.c
SRCS+=		linenoise.c

CFLAGS+=	-g
CFLAGS+=	-O2 -pipe
CFLAGS+=	-Wall -Werror -Wextra -ansi -Wcast-qual -Wformat=2
CFLAGS+=	-Wmissing-declarations -pedantic -Wstrict-prototypes
CFLAGS+=	-Wpointer-arith -Wuninitialized -Wmissing-prototypes
CFLAGS+=	-Wsign-compare -Wshadow -Wdeclaration-after-statement
CFLAGS+=	-Wfloat-equal -Wcast-align -Wundef -Wstrict-aliasing=2

CPPFLAGS+=	-I.

all: ${PROG}

${PROG}: ${SRCS}
	${CC} ${CFLAGS} ${CPPFLAGS} ${LDFLAGS} -o $@ ${SRCS}

clean:
	rm -f a.out [Ee]rrs mklog *.core *.o ${PROG}

.PHONY: all clean
