#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_rules

#---------------------------------------------------------------------------------
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
# DATA is a list of directories containing binary files
# all directories are relative to this makefile
#---------------------------------------------------------------------------------
BUILD		:= build
SOURCES		:= ../source ../source/arm9 ../thirdparty/fatfs
INCLUDES	:= ../include ../thirdparty
DEFINES		:= -DARM9 -D_3DS -DVERS_STRING=\"$(VERS_STRING)\" \
				-DVERS_MAJOR=$(VERS_MAJOR) -DVERS_MINOR=$(VERS_MINOR)
DATA		:= data
ASSETS		:= ../assets
LDNAME		:= arm9.ld

ifneq ($(strip $(NO_DEBUG)),)
	DEFINES += -DNDEBUG
endif


#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:= -march=armv5te -mtune=arm946e-s -mfloat-abi=soft -marm -mthumb-interwork

CFLAGS	:= $(ARCH) -std=c11 -O2 -g -flto -fstrict-aliasing -mword-relocations -fomit-frame-pointer \
			-ffast-math -ffunction-sections -Wall -Wextra
CFLAGS	+=	$(INCLUDE) $(DEFINES)

CXXFLAGS	:=	$(ARCH) -std=c11 -O2 -g -flto -fno-rtti -fno-exceptions -fstrict-aliasing \
				-mword-relocations -fomit-frame-pointer -ffast-math -ffunction-sections -Wall -Wextra
CXXFLAGS	+=	$(INCLUDE) $(DEFINES)

ASFLAGS	:=	$(ARCH) -g -flto $(INCLUDE) $(DEFINES)

LDFLAGS	:=	$(ARCH) -g -flto -Wl,--use-blx,--gc-sections,-Map,$(notdir $*.map) -nostartfiles -T ../$(LDNAME)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:=

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export DEPSDIR := $(CURDIR)/$(BUILD)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
					$(foreach dir,$(ASSETS),$(CURDIR)/$(dir))

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))
ASSETFILES	:=	$(foreach dir,$(ASSETS),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES_SOURCES 	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES)) $(ASSETFILES:.png=.spla.o)

export INTERMEDIATE_ASSETS	:=	$(ASSETFILES:.png=.spla)

export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)

export HFILES	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).bin


#---------------------------------------------------------------------------------
else

.SECONDARY: $(INTERMEDIATE_ASSETS)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).bin	:	$(OUTPUT).elf
	@$(OBJCOPY) --gap-fill 0xFF -O binary $< $@
	@echo built ... $(notdir $@)

$(OFILES_SOURCES) : $(HFILES)

$(OUTPUT).elf	:	$(OFILES)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data 
#---------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
%.spla	:	%.png
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@echo "$@: $<" > $(DEPSDIR)/$*.d 
	@splashtool -f RGB565 -r $< $@

#---------------------------------------------------------------------------------
%.spla.o %_spla.h	:	%.spla
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@echo "$*.spla.o: $<" > $(DEPSDIR)/$*.spla.d 
	@echo "$*_spla.h: $<" >> $(DEPSDIR)/$*.spla.d 
	@$(bin2o)

#---------------------------------------------------------------------------------
%.elf:
#---------------------------------------------------------------------------------
	@echo linking $(notdir $@)
	@$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
	@$(NM) -CSn $@ > $(notdir $*.lst)

-include $(DEPSDIR)/*.d

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------