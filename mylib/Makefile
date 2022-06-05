# Makefile to build all named modules in library form libmypi.a

LIBPI_MODULE_SOURCES = gpio.c timer.c strings.c printf.c backtrace.c malloc.c \
                       keyboard.c shell.c fb.c gl.c console.c ps2_assign7.c

all: libmypi.a

CFLAGS	= -I$(CS107E)/include -Og -g -std=c99 $$warn $$freestanding
CFLAGS += -mapcs-frame -fno-omit-frame-pointer -mpoke-function-name

SOURCES = cstart.c $(LIBPI_MODULE_SOURCES)
OBJECTS = $(addsuffix .o, $(basename $(SOURCES)))

libmypi.a: $(OBJECTS)
	rm -f $@
	arm-none-eabi-ar cDr $@ $(filter %.o,$^)

# Compile C source to object file
%.o: %.c
	arm-none-eabi-gcc $(CFLAGS) -c $< -o $@

# Assemble asm source to object file
%.o: %.s
	arm-none-eabi-as $< -o $@

# Disassemble object file to asm listing
%.list: %.o
	arm-none-eabi-objdump --no-show-raw-insn -d $< > $@

# Remove all build projects
clean:
	rm -f *.a *.o *.bin *.elf *.list

# this rule will provide better error message when
# a source file cannot be found (missing, misnamed)
$(SOURCES):
	$(error cannot find source file `$@` needed for build)

.PHONY: all clean
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
