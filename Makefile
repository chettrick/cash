PROG=		ssi

SRCS=		sh.c

CFLAGS+=	-g
CFLAGS+=	-O2 -pipe
CFLAGS+=	-fPIC
CFLAGS+=	-Wall -Werror -Wextra -Wcast-qual -Wformat=2
CFLAGS+=	-Wmissing-declarations -pedantic -Wstrict-prototypes
CFLAGS+=	-Wpointer-arith -Wuninitialized -Wmissing-prototypes
CFLAGS+=	-Wsign-compare -Wshadow -Wdeclaration-after-statement
CFLAGS+=	-Wfloat-equal -Wcast-align -Wundef -Wstrict-aliasing=2

LDFLAGS+=	-lreadline -ltermcap

all: ${PROG}

${PROG}: ${SRCS}
	${CC} ${SRCS} ${CFLAGS} ${CPPFLAGS} ${LDFLAGS} -o $@

clean:
	rm -f a.out [Ee]rrs mklog *.core *.o ${PROG}

.PHONY: all clean
