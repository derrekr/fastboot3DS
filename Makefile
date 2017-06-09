#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

export TARGET := firm
SECTION0_TYPE := 0
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
	firm_builder $(TARGET).bin \
		$(shell arm-none-eabi-readelf -h $(TARGET)9.elf | grep 'Entry' | sed 's/.*\(0x[0-f]*\)/\1/i') \
		$(shell arm-none-eabi-readelf -h $(TARGET)11.elf | grep 'Entry' | sed 's/.*\(0x[0-f]*\)/\1/i') \
		0x$(shell arm-none-eabi-readelf --sections $(TARGET)9.elf | grep '.text' | sed 's/.*ITS\ *\([0-f]*\).*/\1/i') \
		$(SECTION0_TYPE) $(TARGET)9.bin \
		0x$(shell arm-none-eabi-readelf --sections $(TARGET)11.elf | grep '.text' | sed 's/.*ITS\ *\([0-f]*\).*/\1/i') \
		$(SECTION1_TYPE) $(TARGET)11.bin

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
	firm_builder $(TARGET).bin \
		$(shell arm-none-eabi-readelf -h $(TARGET)9.elf | grep 'Entry' | sed 's/.*\(0x[0-f]*\)/\1/i') \
		$(shell arm-none-eabi-readelf -h $(TARGET)11.elf | grep 'Entry' | sed 's/.*\(0x[0-f]*\)/\1/i') \
		0x$(shell arm-none-eabi-readelf --sections $(TARGET)9.elf | grep '.text' | sed 's/.*ITS\ *\([0-f]*\).*/\1/i') \
		$(SECTION0_TYPE) $(TARGET)9.bin \
		0x$(shell arm-none-eabi-readelf --sections $(TARGET)11.elf | grep '.text' | sed 's/.*ITS\ *\([0-f]*\).*/\1/i') \
		$(SECTION1_TYPE) $(TARGET)11.bin
	@7z a -mx -m0=ARM -m1=LZMA2 fastboot3DS$(VERS_STRING).7z $(TARGET).bin
#	@7z u -mx -m0=LZMA2 fastboot3DS$(VERS_STRING).7z README.md
