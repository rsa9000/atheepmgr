
TARGET=atheepmgr

OBJ=\
	atheepmgr.o	\
	con_file.o	\
	con_stub.o	\
	eep_5211.o	\
	eep_5416.o	\
	eep_6174.o	\
	eep_9285.o	\
	eep_9287.o	\
	eep_9300.o	\
	eep_9880.o	\
	eep_9888.o	\
	eep_common.o	\
	hw.o		\
	utils.o		\

DEP=$(OBJ:%.o=%.d)

DEFS=

OS?=$(shell uname -s)
HAVE_LIBPCIACCESS=$(shell pkg-config pciaccess && echo y || echo n)

CONFIG_CON_DRIVER?=$(if $(filter Linux,$(OS)),y)
CONFIG_CON_PCI?=$(HAVE_LIBPCIACCESS)
CONFIG_CON_MEM?=y
CONFIG_I_KNOW_WHAT_I_AM_DOING?=n

ifeq ($(CONFIG_CON_DRIVER),y)
  ifeq ($(OS),Linux)
    DEFS+=-DCONFIG_CON_DRIVER
    OBJ+=con_driver_linux.o
  else
    $(error Driver connector building was requested, but there are no driver access support for OS $(OS))
  endif
endif
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

FORCE:

$(TARGET): config.h $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(DEPFLAGS) $(CFLAGS) -include config.h -c $< -o $@

.__config: FORCE
	@echo "$(DEFS)" | tr ' ' '\n' | grep -v '^$$' > $@.tmp
	@diff $@ $@.tmp > /dev/null 2>&1 && rm -f $@.tmp || mv $@.tmp $@

config.h: .__config
	@echo "Generate config.h"
	@echo '/* Automatically generated. DO NOT EDIT. */' > $@.tmp
	@echo '#ifndef _CONFIG_H_' >> $@.tmp
	@echo '#define _CONFIG_H_' >> $@.tmp
	@sed -re 's/-D([A-Z0-9_]+)/#define \1/' $^ >> $@.tmp
	@echo '#endif' >> $@.tmp
	@mv $@.tmp $@

clean:
	rm -rf $(TARGET)
	rm -rf .__config config.h
	rm -rf $(OBJ)
	rm -rf $(DEP)

-include $(DEP)
