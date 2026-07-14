EE_BIN = PS2Grid.elf
EE_OBJS = main.o
EE_LIBS = -L$(GSKIT)/lib -lgskit -ldmakit -lpad -ldebug -lm
EE_INCS = -I$(GSKIT)/include -I$(GSKIT)/ee/dma/include -I$(GSKIT)/ee/gs/include

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
