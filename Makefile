.PHONY: clean

all:	code9.bin code11.bin firm.bin

clean:
	make clean -f Makefile.arm9
	make clean -f Makefile.arm11
	rm -fr firm.bin

code9.bin:
	make -f Makefile.arm9
code11.bin:
	make -f Makefile.arm11
firm.bin:
	firm_builder firm.bin 0x8006800 0x1FF80000 0x8006800 0 code9.bin 0x1FF80000 1 code11.bin
	