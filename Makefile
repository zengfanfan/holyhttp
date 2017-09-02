export TOP := $(shell pwd)
export _DEBUG_ := y
export CC := gcc
export STRIP := strip

TARGET := libholyhttp.so
OBJS := holyhttp.o
SUBDIRS := utils server

CFLAGS-${_DEBUG_} += -g -ggdb
CFLAGS-y += -I${TOP}
CFLAGS-y += -D_GNU_SOURCE -D__USE_XOPEN
CFLAGS-y += -Wall -Wno-missing-braces
CFLAGS-y += -Werror
CFLAGS-${_DEBUG_} += -DDEBUG_ON=1

########## DO NOT MODIFY THE BELOW ##########
export CFLAGS := ${CFLAGS-y} -fPIC

include ${TOP}/common.mk

all: subs ${TARGET}
${TARGET}: ${OBJS} ${SUBOBJS}
	${CC} -shared -o $@ $^
	${STRIP} $@

