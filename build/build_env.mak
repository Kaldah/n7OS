# On my MacOS, x86 compiler (installed by macports) is prefixed by i386-elf- 
OS=$(shell uname -s)
ifeq ($(OS), Darwin)
	PREFIX=i386-elf-
else 
	PREFIX=
endif

DEBUG=$(PREFIX)gdb
CC= $(PREFIX)gcc
CFLAGS= -m32 -g -ggdb -std=c99 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -nostartfiles -nodefaultlibs -Wall -Wextra # -Werror

LD= $(PREFIX)ld.bfd
LDFLAGS = -melf_i386

AS= $(PREFIX)gcc
ASFLAGS= -m32 -DASSEMBLER -g -ggdb

OBJCOPY= $(PREFIX)objcopy