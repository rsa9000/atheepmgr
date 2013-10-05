
TARGET=edump

OBJ=\
	con_file.o	\
	edump.o		\
	eep_5416.o	\
	eep_9285.o	\
	eep_9287.o	\
	eep_9300.o	\
	eep_common.o	\
	hw.o		\

DEFS=

HAVE_LIBPCIACCESS=$(shell pkg-config pciaccess && echo y || echo n)

CONFIG_CON_PCI?=$(HAVE_LIBPCIACCESS)
CONFIG_CON_MEM?=y

ifeq ($(CONFIG_CON_PCI),y)
DEFS+=-DCONFIG_CON_PCI
OBJ+=con_pci.o
con_pci.o: CFLAGS+=$(shell pkg-config --cflags pciaccess)
LDFLAGS+=$(shell pkg-config --libs pciaccess)
endif
ifeq ($(CONFIG_CON_MEM),y)
DEFS+=-DCONFIG_CON_MEM
OBJ+=con_mem.o
endif

CC?=gcc

CFLAGS+=-Wall

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(DEFS) -c $< -o $@

clean:
	rm -rf $(TARGET)
	rm -rf $(OBJ)
