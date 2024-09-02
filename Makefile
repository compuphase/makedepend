OBJS=cppsetup.o ifparser.o include.o main.o parse.o pr.o
ifeq ($(MAKE_HOST),Windows)
    EXTENSION=.exe
else
    EXTENSION=
endif
BIN=makedepend$(EXTENSION)

all: $(BIN)

clean:
	rm -f $(BIN) *.o

depend:
	$(BIN) $(OBJS:.o=.c)

$(BIN): $(OBJS)
	gcc -Wall -s -O3 $(OBJS) -o $@

%.o: %.c
	gcc -Wall -Wno-format-truncation -c -O3 $<

# GENERATED DEPENDENCIES. DO NOT DELETE.

cppsetup.o : def.h ifparser.h
ifparser.o : ifparser.h
include.o : def.h
main.o : def.h imakemdep.h
parse.o : def.h
pr.o : def.h
