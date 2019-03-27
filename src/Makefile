CC = g++
RM = rm -f
CPPFLAG = -g -Wall -O2 -std=c++11 -fno-strict-aliasing

BINPY = /export/home/u15/wli/metadata/src/binarization.py
SRCS = main.cpp HandwrittenImage.cpp ConvexHullComponent.cpp \
	   Point.cpp GroupTree.cpp ConfigParser.cpp MsgPrint.cpp
OBJS = $(subst .cpp,.o,$(SRCS))

config = __NONE__
image = __NONE__
outdir = __NONE__
prefix = __NONE__
dumpall = 0

bin_image = $(addprefix $(outdir),$(addprefix /,$(addsuffix _bin.bmp,$(prefix))))

.PHONY: setup preprocess build run clean

run: build preprocess
	./engine $(config) $(bin_image) $(outdir) $(prefix) $(dumpall)

preprocess: setup $(BINPY)
	$(BINPY) $(image) $(bin_image)

setup:
ifeq ($(image), __NONE__)
	$(error [ERR] Please specify input image (image=<image path>))
endif
ifeq ($(outdir), __NONE__)
	$(error [ERR] Please specify output directory (outdir=<output path>))
endif
ifeq ($(prefix), __NONE__)
	$(error [ERR] Please specify output prefix (prefix=<name>))
endif

ifeq ("$(wildcard $(image))","")
	$(error [ERR] Cannot find input image $(image))
endif
	$(shell test -d $(outdir) || mkdir -p $(outdir))


build: engine


engine: $(OBJS)
	$(CC) -o engine $(OBJS)

main.o: main.cpp HandwrittenImage.h
	$(CC) $(CPPFLAG) -c main.cpp

HandwrittenImage.o: HandwrittenImage.cpp HandwrittenImage.h ConvexHullComponent.h GroupTree.h MsgPrint.h
	$(CC) $(CPPFLAG) -c HandwrittenImage.cpp

ConvexHullComponent.o: ConvexHullComponent.cpp ConvexHullComponent.h Point.h
	$(CC) $(CPPFLAG) -c ConvexHullComponent.cpp

Point.o: Point.cpp Point.h HandwrittenImage.h
	$(CC) $(CPPFLAG) -c Point.cpp

GroupTree.o: GroupTree.cpp GroupTree.h
	$(CC) $(CPPFLAG) -c GroupTree.cpp

ConfigParser.o: ConfigParser.cpp ConfigParser.h MsgPrint.h
	$(CC) $(CPPFLAG) -c ConfigParser.cpp

MsgPrint.o: MsgPrint.cpp MsgPrint.h
	$(CC) $(CPPFLAG) -c MsgPrint.cpp

clean:
	$(RM) $(OBJS) engine
