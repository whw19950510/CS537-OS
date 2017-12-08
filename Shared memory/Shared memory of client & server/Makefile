# Warnings
WFLAGS	:= -Wall -Werror -Wno-unused-parameter -D_GNU_SOURCE

# Optimization and architecture
OPT		:= -g
ARCH   	:= 

# Language standard
CCSTD	:= -std=gnu99 

# Linker options
LDOPT 	:= $(OPT)
LDFLAGS := -lrt -lpthread 

.DEFAULT_GOAL := all
EXEC := shm_server shm_client
OBJS := shm_server.o shm_client.o
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
shm_server: shm_server.o
	@ echo Building $@...
	@ $(CXX) -o $@ $< $(LDFLAGS) $(LDFLAGS)
shm_client: shm_client.o
	@ echo Building $@...
	@ $(CXX) -o $@ $< $(LDFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	@ rm -f $(EXEC) $(OBJS)