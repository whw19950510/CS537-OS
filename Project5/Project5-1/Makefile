# Warnings
WFLAGS	:= -Wno-unused-variable -D_GNU_SOURCE

# Optimization and architecture
OPT		:= -g
ARCH   	:= 

# Language standard
CCSTD	:= -std=gnu99 

# Linker options
LDOPT 	:= $(OPT)
LDFLAGS := -lrt -lpthread 

.DEFAULT_GOAL := all
EXEC := xv6_fsck
OBJS := xv6_fsck.o 
.PHONY: debug
debug : OPT  := -O0 -g -fno-omit-frame-pointer -fsanitize=address
debug : LDFLAGS := -fsanitize=address
debug : ARCH :=
debug : $(EXEC)

all : Makefile $(EXEC)

%.o : %.c Makefile
	@ echo Compiling $<...
	$(CC) $(CCSTD) $(WFLAGS) $(OPT) $(ARCH) $(CFLAGS) -c $< -o $@

# TODO: add targets for building executables
xv6_fsck: xv6_fsck.o
	@ echo Building $@...
	@ $(CXX) -o $@ $< $(LDFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	@ rm -f $(EXEC) $(OBJS)