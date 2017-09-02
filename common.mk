include ${TOP}/alias.mk

TARGET ?= ../$(notdir ${MYPATH}).o
MYPATH := $(shell pwd)
SUBOBJS := $(patsubst %,%.o,${SUBDIRS})

all: subs ${TARGET}

${TARGET}: ${OBJS} ${SUBOBJS}
	${LD} -r -o $@ $^

subs:
	@for i in ${SUBDIRS}; \
		#${ENTER} ${MYPATH}/$$i;\
		do make -C $$i PARENT=${MYPATH} || exit $$?;\
		#${LEAVE} ${MYPATH}/$$i;\
	done

clean:
	${RM} ${TARGET} ${OBJS} ${SUBOBJS} *.o *.tmp core.* *.so
	@for i in ${SUBDIRS}; \
		do make --no-print-directory -C $$i $@; \
	done

