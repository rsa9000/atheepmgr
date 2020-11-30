
TARGET=atheepmgr

OBJ=\
	atheepmgr.o	\
	con_file.o	\
	eep_5211.o	\
	eep_5416.o	\
	eep_9285.o	\
	eep_9287.o	\
	eep_9300.o	\
	eep_common.o	\
	hw.o		\
	utils.o		\

DEP=$(OBJ:%.o=%.d)

DEFS=

HAVE_LIBPCIACCESS=$(shell pkg-config pciaccess && echo y || echo n)

CONFIG_CON_PCI?=$(HAVE_LIBPCIACCESS)
CONFIG_CON_MEM?=y
CONFIG_I_KNOW_WHAT_I_AM_DOING?=n

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
ifeq ($(CONFIG_I_KNOW_WHAT_I_AM_DOING),y)
DEFS+=-DCONFIG_I_KNOW_WHAT_I_AM_DOING
endif

CC?=gcc

CFLAGS+=-Wall

DEPFLAGS=-MMD -MP

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(DEPFLAGS) $(CFLAGS) $(DEFS) -c $< -o $@

clean:
	rm -rf $(TARGET)
	rm -rf $(OBJ)
	rm -rf $(DEP)

-include $(DEP)
