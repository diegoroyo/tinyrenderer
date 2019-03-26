BINDIR = bin

PNGIMAGE = pngimage
PNGCHUNK = pngchunk
MAIN = main

NOMBREEXE = main

CPPFLAGS = -ggdb -O0 -std=c++11
CC = x86_64-w64-mingw32-g++

.PHONY: all
all: ${BINDIR}/${NOMBREEXE}

.PHONY: clean
clean:
	rm -rf ${BINDIR}/*

${BINDIR}/${NOMBREEXE}: ${BINDIR}/${PNGIMAGE}.o ${BINDIR}/${PNGCHUNK}.o ${MAIN}.cpp
	${CC} $^ ${CPPFLAGS} -o $@

${BINDIR}/${PNGIMAGE}.o: ${PNGIMAGE}.cpp ${PNGIMAGE}.h
	${CC} -c ${CPPFLAGS} $< -o $@

${BINDIR}/${PNGCHUNK}.o: ${PNGCHUNK}.cpp ${PNGCHUNK}.h
	${CC} -c ${CPPFLAGS} $< -o $@