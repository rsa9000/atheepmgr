
TARGET=edump

OBJ=\
	con_pci.o	\
	edump.o		\
	eep_4k.o	\
	eep_9003.o	\
	eep_9287.o	\
	eep_def.o	\
	hw.o		\

CFLAGS+=$(shell pkg-config --cflags pciaccess)
LDFLAGS+=$(shell pkg-config --libs pciaccess)

CC?=gcc

CFLAGS+=-Wall

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET)
	rm -rf $(OBJ)
