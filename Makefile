OBJS=cppsetup.o ifparser.o include.o main.o parse.o pr.o
ifeq ($(MAKE_HOST),Windows)
    EXTENSION=.exe
else
    EXTENSION=
endif
BIN=makedepend$(EXTENSION)

CFLAGS = -Wall -Wno-format-truncation
LFLAGS = -Wall
ifdef NDEBUG
    CFLAGS += -O3
    LFLAGS += -s
else
    CFLAGS += -g
    LFLAGS += -g
endif

all: $(BIN)

clean:
	rm -f $(BIN) *.o

depend:
	$(BIN) $(OBJS:.o=.c)

$(BIN): $(OBJS)
	gcc $(LFLAGS) -o $@ $(OBJS)

%.o: %.c
	gcc $(CFLAGS) -c $<

# GENERATED DEPENDENCIES. DO NOT DELETE.

cppsetup.o : def.h ifparser.h
ifparser.o : ifparser.h
include.o : def.h
main.o : def.h imakemdep.h
parse.o : def.h
pr.o : def.h
