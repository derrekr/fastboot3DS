#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

export TARGET := firm
ENTRY9        := 0x08000054
ENTRY11       := 0x1FF85040
SECTION0_ADR  := 0x08000040
SECTION0_TYPE := 0
SECTION1_ADR  := 0x1FF85000
SECTION1_TYPE := 1


export VERS_STRING := $(shell git describe --tags --match v[0-9]* --abbrev=8 | sed 's/-[0-9]*-g/-/i')

export VERS_MAJOR  := $(shell echo "$(VERS_STRING)" | sed 's/v\([0-9]*\)\..*/\1/i')
export VERS_MINOR  := $(shell echo "$(VERS_STRING)" | sed 's/.*\.\([0-9]*\).*/\1/')


.PHONY: checkarm9 checkarm11 clean release

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: checkarm9 checkarm11 $(TARGET).bin

#---------------------------------------------------------------------------------
checkarm9:
	@$(MAKE) -j4 --no-print-directory -f Makefile.arm9
	
#---------------------------------------------------------------------------------
checkarm11:
	@$(MAKE) -j4 --no-print-directory -f Makefile.arm11

#---------------------------------------------------------------------------------
$(TARGET).bin: $(TARGET)9.bin $(TARGET)11.bin
	firm_builder $(TARGET).bin $(ENTRY9) $(ENTRY11) $(SECTION0_ADR) $(SECTION0_TYPE) \
		$(TARGET)9.bin $(SECTION1_ADR) $(SECTION1_TYPE) $(TARGET)11.bin

#---------------------------------------------------------------------------------
$(TARGET)9.bin:
	@$(MAKE) -j4 --no-print-directory -f Makefile.arm9
	
#---------------------------------------------------------------------------------
$(TARGET)11.bin:
	@$(MAKE) -j4 --no-print-directory -f Makefile.arm11

#---------------------------------------------------------------------------------
clean:
	@$(MAKE) --no-print-directory -f Makefile.arm9 clean
	@$(MAKE) --no-print-directory -f Makefile.arm11 clean
	rm -f $(TARGET).bin

release: clean
	@$(MAKE) -j4 --no-print-directory -f Makefile.arm9 NO_DEBUG=1
	@$(MAKE) -j4 --no-print-directory -f Makefile.arm11 NO_DEBUG=1
	firm_builder $(TARGET).bin $(ENTRY9) $(ENTRY11) $(SECTION0_ADR) $(SECTION0_TYPE) \
		$(TARGET)9.bin $(SECTION1_ADR) $(SECTION1_TYPE) $(TARGET)11.bin
	@7z a -mx -m0=ARM -m1=LZMA2 fastboot3DS$(VERS_STRING).7z $(TARGET).bin
#	@7z u -mx -m0=LZMA2 fastboot3DS$(VERS_STRING).7z README.md
