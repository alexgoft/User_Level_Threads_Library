CC = g++
STD = -std=gnu++11

UTHREAD_OBJECTSS = uthreads.cpp uthreads.h
SCHEDULE_ROBJECT = Scheduler.cpp Scheduler.h
THREAD_OBJECTS = Thread.cpp Thread.h
ERRORH_ANDLER_OBJECTS =  ErrorHandler.cpp ErrorHandler.h

TAROBJECTS = uthreads.cpp Scheduler.cpp Scheduler.h Thread.cpp \
Thread.h ErrorHandler.cpp ErrorHandler.h Makefile README

CFLAGS = -Wextra -Wvla -Wall -Wno-unused-parameter

all: uthreads

uthreads: $(UTHREAD_OBJECTSS) $(SCHEDULE_ROBJECT) $(THREAD_OBJECTS) \
$(ERRORH_ANDLER_OBJECTS)
	${CC} $(STD) ${CFLAGS} -c uthreads.cpp -o uthreads.o
	${CC} $(STD) ${CFLAGS} -c Thread.cpp -o Thread.o
	${CC} $(STD) ${CFLAGS} -c Scheduler.cpp -o Scheduler.o
	${CC} $(STD) ${CFLAGS} -c ErrorHandler.cpp -o ErrorHandler.o
	ar rcs libuthreads.a uthreads.o Thread.o Scheduler.o ErrorHandler.o

tar:
	tar cvf ex2.tar ${TAROBJECTS}

clean:
	rm -f ex2.tar uthreads.o Scheduler.o Thread.o ErrorHandler.o libuthreads.a

.PHONY: all uthreads tar clean