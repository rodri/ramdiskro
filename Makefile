# c 2018,2019,2020

CFLAGS=-Wall -Wextra -O2 #-m32 #-Werror
CXXFLAGS=$(CFLAGS)
LDFLAGS=$(CFLAGS)
CC=$(CXX)

BIN=ramdiskro_build ramdiskro_read ramdiskro_readall

all: $(BIN)

clean:
	rm -f *.o $(BIN)

depend:
	makedepend -Y. *.cpp *.c 2> /dev/null

ramdiskro_build: ramdiskro_build.o ramdiskro_builder.o ramdiskro.o
ramdiskro_read: ramdiskro_read.o ramdiskro.o
ramdiskro_readall: ramdiskro_readall.o ramdiskro.o

# DO NOT DELETE

ramdiskro_build.o: ramdiskro_builder.hpp ramdiskro.h
ramdiskro_builder.o: ramdiskro_builder.hpp ramdiskro.h
ramdiskro_read.o: ramdiskro.h
ramdiskro_readall.o: ramdiskro.h
ramdiskro.o: ramdiskro.h
