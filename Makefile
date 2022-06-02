# Programs built by this makefile
RUN_PROGRAM   = graphic.bin

MY_MODULE_SOURCES = fb.c gl.c console.c

# MY_MODULE_SOURCES is a list of those library modules (such as gpio.c)
# for which you intend to use your own code. The reference implementation
# from our libraries will be used for any module you do not name in this list.
# Editing this list allows you to control whether the program being
# built is using your code or the reference implementation for each module
# on a per-module basis. Great for testing!
#
# NOTE: when you name a module in this list, it must provide definitions
# for all of the symbols in the entire module. For example, if you list
# gpio.c as one of your modules, your gpio.c must define gpio_set_function,
# gpio_get_function, ... and so on for all functions declared in the gpio.h
# header file. If your module forgets to implement any of the needed
# functions, the linker will bring in gpio.o from reference libpi to
# resolve the missing definition. But you can't have both gpio modules!
# The linker will report multiple definition errors for every function
# that occurs in both your gpio.c and the reference gpio.o. No bueno!
#
# You shouldn't need to modify anything below this line.
########################################################

PROGRAMS      = $(RUN_PROGRAM) $(TEST_PROGRAM)

all: $(PROGRAMS)

# Flags for compile and link
CFLAGS	= -I$(CS107E)/include -Og -g -std=c99 $$warn $$freestanding
CFLAGS += -mapcs-frame -fno-omit-frame-pointer -mpoke-function-name
LDFLAGS	= -nostdlib -T memmap -L$(CS107E)/lib
LDLIBS 	= -lpi -lgcc

# Common objects for the programs built by this makefile
SOURCES = start.s cstart.c $(MY_MODULE_SOURCES)
OBJECTS = $(addsuffix .o, $(basename $(SOURCES)))

# Rules and recipes for all build steps

# Extract raw binary from elf executable
%.bin: %.elf
	arm-none-eabi-objcopy $< -O binary $@

# Link program executable from program.o and all common objects
%.elf: $(OBJECTS) %.o
	@echo arm-none-eabi-gcc $(LDFLAGS) $^ $(LDLIBS) -o $@
	@$(CS107E)/bin/link-filter arm-none-eabi-gcc $(LDFLAGS) $^ $(LDLIBS) -o $@

# Compile C source to object file
%.o: %.c
	arm-none-eabi-gcc $(CFLAGS) -c $< -o $@

# Assemble asm source to object file
%.o: %.s
	arm-none-eabi-as $< -o $@

# Disassemble object file to asm listing
%.list: %.o
	arm-none-eabi-objdump --no-show-raw-insn -d $< > $@

# Build and run the application binary
run: $(RUN_PROGRAM)
	rpi-run.py -p $<

# Build and run the test binary
test: $(TEST_PROGRAM)
	rpi-run.py -p $<

# Remove all build projects
clean:
	rm -f *.o *.bin *.elf *.list

# this rule will provide better error message when
# a source file cannot be found (missing, misnamed)
$(SOURCES) $(PROGRAMS:.bin=.c):
	$(error cannot find source file `$@` needed for build)

# Access .c and .s source files within shared mylib directory using vpath
# https://www.cmcrossroads.com/article/basics-vpath-and-vpath
vpath %.c ../mylib
vpath %.s ../mylib

.PHONY: all clean run test
.PRECIOUS: %.elf %.o

# disable built-in rules (they are not used)
.SUFFIXES:

export warn = -Wall -Wpointer-arith -Wwrite-strings -Werror \
              -Wno-error=unused-function -Wno-error=unused-variable \
              -fno-diagnostics-show-option
export freestanding = -ffreestanding -nostdinc \
                      -isystem $(shell arm-none-eabi-gcc -print-file-name=include)

define CS107E_ERROR_MESSAGE
ERROR - CS107E environment variable is not set.

Review instructions for properly configuring your shell.
https://cs107e.github.io/guides/install/userconfig#env

endef

ifndef CS107E
$(error $(CS107E_ERROR_MESSAGE))
endif
