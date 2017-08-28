export TOP := $(shell pwd)
export _DEBUG_ := y
export CC := gcc
export STRIP := strip

TARGET := holyhttp

OBJS := main.o
SUBDIRS := utils server cgi

CFLAGS-${_DEBUG_} += -g -ggdb
CFLAGS-y += -I${TOP}
CFLAGS-y += -D_GNU_SOURCE -D__USE_XOPEN
CFLAGS-y += -Wall -Wno-missing-braces
CFLAGS-y += -Werror
CFLAGS-${_DEBUG_} += -DDEBUG_ON=1

########## DO NOT MODIFY THE BELOW ##########
export CFLAGS := ${CFLAGS-y}

include ${TOP}/common.mk

${TARGET}: ${OBJS} ${SUBOBJS}
	${CC} -o $@ $^
	${STRIP} $@

test: subs test.o ${SUBOBJS}
	${CC} -o $@ test.o ${SUBOBJS}
	@echo

