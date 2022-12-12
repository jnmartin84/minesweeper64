ROOTDIR			=$(N64_INST)


GCCN64PREFIX = $(ROOTDIR)/bin/mips64-elf-
CHKSUM64PATH = $(ROOTDIR)/bin/chksum64
MKDFSPATH = $(ROOTDIR)/bin/mkdfs
MKSPRITEPATH = $(ROOTDIR)/bin/mksprite
HEADERPATH = $(ROOTDIR)/mips64-elf/lib
N64TOOL = $(ROOTDIR)/bin/n64tool
HEADERNAME = header


LIBS = -ldragon -lc -lm -ldragonsys -lc
LINK_FLAGS = -G4 -L$(ROOTDIR)/lib -L$(ROOTDIR)/mips64-elf/lib $(LIBS) -Tn64.ld
PROG_NAME = minesweeper
CFLAGS = -DTRUECOLOR -std=gnu99 -march=vr4300 -mtune=vr4300 -fno-strict-aliasing -Wall -G4 -O4 -I$(ROOTDIR)/include -I$(ROOTDIR)/mips64-elf/include
CFLAGS_ASM = -DTRUECOLOR -std=gnu99 -march=vr4300 -mtune=vr4300 -fno-strict-aliasing -Wall -G4 -O4 -I$(ROOTDIR)/include -I$(ROOTDIR)/mips64-elf/include
ASFLAGS = -mtune=vr4300 -march=vr4300


CC = $(GCCN64PREFIX)gcc
AS = $(GCCN64PREFIX)as
LD = $(GCCN64PREFIX)ld
OBJCOPY = $(GCCN64PREFIX)objcopy
OBJDUMP = $(GCCN64PREFIX)objdump


O=obj


OBJS=						\
		$(O)/mine.o


$(PROG_NAME).z64: $(PROG_NAME).elf 
	$(OBJCOPY) $(PROG_NAME).elf $(PROG_NAME).bin -O binary
	rm -f $(PROG_NAME).z64
	$(N64TOOL) -l 8M -t "MINESWEE" -h $(HEADERPATH)/$(HEADERNAME) -o $(PROG_NAME).z64 $(PROG_NAME).bin
	$(CHKSUM64PATH) $(PROG_NAME).z64


$(PROG_NAME).v64: $(PROG_NAME).elf 
	$(OBJCOPY) $(PROG_NAME).elf $(PROG_NAME).bin -O binary
	rm -f $(PROG_NAME).v64
	$(N64TOOL) -b -l 8M -t "MINESWEE" -h $(HEADERPATH)/$(HEADERNAME) -o $(PROG_NAME).v64 $(PROG_NAME).bin
	$(CHKSUM64PATH) $(PROG_NAME).v64


$(PROG_NAME).elf : $(OBJS) $(ASM)
	$(LD) -o $(PROG_NAME).elf $(OBJS) $(LINK_FLAGS)
	$(OBJDUMP) -t $(PROG_NAME).elf > $(PROG_NAME)_symbols.txt
	cat $(PROG_NAME)_symbols.txt | grep 'F .text' > $(PROG_NAME)_functions.txt

copy: $(PROG_NAME).z64
	cp $(PROG_NAME).z64 ~/


all: $(PROG_NAME).z64


clean:
	rm -f *.z64 *.v64 *.elf *.bin *.dfs $(PROG_NAME)_symbols.txt $(PROG_NAME)_functions.txt
	rm -f $(O)/*

$(O)/%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

#############################################################
#
#############################################################
