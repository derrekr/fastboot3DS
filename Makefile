.SUFFIXES:

TARGET        := firm.bin
ENTRY9        := 0x08000054
ENTRY11       := 0x1FF85040
SECTION0_ADR  := 0x08000040
SECTION0_TYPE := 0
SECTION0_FILE := code9.bin
SECTION1_ADR  := 0x1FF85000
SECTION1_TYPE := 1
SECTION1_FILE := code11.bin


.PHONY: clean release

$(TARGET):
	@$(MAKE) --no-print-directory -f Makefile.arm9
	@$(MAKE) --no-print-directory -f Makefile.arm11
	@firm_builder $(TARGET) $(ENTRY9) $(ENTRY11) $(SECTION0_ADR) $(SECTION0_TYPE) \
		$(SECTION0_FILE) $(SECTION1_ADR) $(SECTION1_TYPE) $(SECTION1_FILE)
	@echo built ... $(TARGET)

clean:
	@$(MAKE) --no-print-directory -f Makefile.arm9 clean
	@$(MAKE) --no-print-directory -f Makefile.arm11 clean
	@rm -rf $(TARGET)

release:
	@$(MAKE) --no-print-directory -f Makefile.arm9 release
	@$(MAKE) --no-print-directory -f Makefile.arm11 release
	@firm_builder $(TARGET) $(ENTRY9) $(ENTRY11) $(SECTION0_ADR) $(SECTION0_TYPE) \
		$(SECTION0_FILE) $(SECTION1_ADR) $(SECTION1_TYPE) $(SECTION1_FILE)
	@echo built ... $(TARGET)
	@7z a -mx -m0=ARM -m1=lzma2 fastboot3ds.7z $(TARGET)
