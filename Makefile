BINDIR = bin
PIDIR = pngimage
MAIN = main

PIOBJ = $(patsubst ${PIDIR}/%.cpp,${BINDIR}/%.o,$(wildcard ${PIDIR}/*.cpp))
PROJOBJ = $(patsubst ./%.cpp, ${BINDIR}/%.o, $(wildcard ./*.cpp))

NOMBREEXE = main

CPPFLAGS = -O3 -std=c++11
CC = g++

#CPPFLAGS = -ggdb -O0 -std=c++11
#CC = x86_64-w64-mingw32-g++

.PHONY: all
all: ${BINDIR}/${NOMBREEXE}

.PHONY: clean
clean:
	rm -rf ${BINDIR}/*

${BINDIR}/${NOMBREEXE}: $(PIOBJ) $(PROJOBJ)
	${CC} $^ ${CPPFLAGS} -o $@

${BINDIR}/%.o: ${PIDIR}/%.cpp
	${CC} -c ${CPPFLAGS} $< -o $@

${BINDIR}/%.o: %.cpp
	${CC} -c ${CPPFLAGS} $< -o $@
