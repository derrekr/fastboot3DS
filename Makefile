#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

export TARGET := fastboot3DS
ENTRY9        := 0x080000D4
ENTRY11       := 0x1FF89040
SECTION0_ADR  := 0x080000C0
SECTION0_TYPE := 0
SECTION0_FILE := arm9/$(TARGET)9.bin
SECTION1_ADR  := 0x07FFFE8C
SECTION1_TYPE := 0
SECTION1_FILE := superhax/superhax.bin
SECTION2_ADR  := 0x1FF89000
SECTION2_TYPE := 1
SECTION2_FILE := arm11/$(TARGET)11.bin


export VERS_STRING := $(shell git describe --tags --match v[0-9]* --abbrev=8 | sed 's/-[0-9]*-g/-/i')
export VERS_MAJOR  := $(shell echo "$(VERS_STRING)" | sed 's/v\([0-9]*\)\..*/\1/i')
export VERS_MINOR  := $(shell echo "$(VERS_STRING)" | sed 's/.*\.\([0-9]*\).*/\1/')


.PHONY: checkarm9 checksuperhax checkarm11 clean release

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: checkarm9 checksuperhax checkarm11 $(TARGET).firm

#---------------------------------------------------------------------------------
checkarm9:
	@$(MAKE) -j4 --no-print-directory -C arm9

#---------------------------------------------------------------------------------
checksuperhax:
	@$(MAKE) --no-print-directory -C superhax

#---------------------------------------------------------------------------------
checkarm11:
	@$(MAKE) -j4 --no-print-directory -C arm11

#---------------------------------------------------------------------------------
$(TARGET).firm: arm9/$(TARGET)9.bin superhax/superhax.bin arm11/$(TARGET)11.bin
	firm_builder $(TARGET).firm $(ENTRY9) $(ENTRY11) $(SECTION0_ADR) $(SECTION0_TYPE) \
		$(SECTION0_FILE) $(SECTION1_ADR) $(SECTION1_TYPE) $(SECTION1_FILE) $(SECTION2_ADR) \
		$(SECTION2_TYPE) $(SECTION2_FILE)
	@bash patchSuperhaxSection.sh

#---------------------------------------------------------------------------------
arm9/$(TARGET)9.bin:
	@$(MAKE) -j4 --no-print-directory -C arm9

#---------------------------------------------------------------------------------
superhax/superhax.bin:
	@$(MAKE) --no-print-directory -C superhax

#---------------------------------------------------------------------------------
arm11/$(TARGET)11.bin:
	@$(MAKE) -j4 --no-print-directory -C arm11

#---------------------------------------------------------------------------------
clean:
	@$(MAKE) --no-print-directory -C arm9 clean
	@$(MAKE) --no-print-directory -C superhax clean
	@$(MAKE) --no-print-directory -C arm11 clean
	rm -f $(TARGET).firm *.7z

release: clean
	@$(MAKE) -j4 --no-print-directory -C arm9 NO_DEBUG=1
	@$(MAKE) --no-print-directory -C superhax NO_DEBUG=1
	@$(MAKE) -j4 --no-print-directory -C arm11 NO_DEBUG=1
	firm_builder $(TARGET).firm $(ENTRY9) $(ENTRY11) $(SECTION0_ADR) $(SECTION0_TYPE) \
		$(SECTION0_FILE) $(SECTION1_ADR) $(SECTION1_TYPE) $(SECTION1_FILE) $(SECTION2_ADR) \
		$(SECTION2_TYPE) $(SECTION2_FILE)
	@bash patchSuperhaxSection.sh
	@bash signFirm.sh
	@7z a -mx -m0=ARM -m1=LZMA $(TARGET)$(VERS_STRING).7z $(TARGET).firm
	@7z u -mx -m0=PPMD $(TARGET)$(VERS_STRING).7z LICENSE.txt README.md
