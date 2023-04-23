# Determine the operating system
ifeq ($(OS),Windows_NT)
    OS = Windows
    ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
        PLATFORM = x64
    endif
    ifeq ($(PROCESSOR_ARCHITECTURE),ARM64)
        PLATFORM = arm64
    endif
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        OS = LINUX
    endif
    ifeq ($(UNAME_S),Darwin)
        OS = OSX
    endif
    UNAME_P := $(shell uname -p)
    PLATFORM = x64
    ifneq ($(filter arm%,$(UNAME_P)),)
        PLATFORM = arm64
    endif
endif

# TODO: Check OS

# Set common compiler parameters
# TODO: Debug defines per platform
ifeq ($(Configuration), Debug)
DBG_OPT_FLAGS=-g
else
DBG_OPT_FLAGS=-O2
endif

INCLUDES=-I../Common/\

LIBS=

# Set compiler and flags based on the operating system
CC = clang++
CFLAGS = -Wall -std=c++20 -fms-extensions -fno-objc-arc $(INCLUDES) $(DBG_OPT_FLAGS)
PCHFLAGS = -include-pch obj/PreCompiledHeader.pch
LDFLAGS = -dynamiclib $(LIBS) 
LIB_EXTENSION = dylib

# Get source files dependency
SRC_FILES = $(filter-out UnityBuild.cpp, $(wildcard *.cpp *.h **/*.cpp **/*.h))
 
# Create obj folder if it doesn't exist
$(shell mkdir -p obj)

# Targets
.PHONY: clean all
all: Elemental.Native

obj/PreCompiledHeader.pch: PreCompiledHeader.h
	$(CC) -c $(CFLAGS) -x c++-header PreCompiledHeader.h -o obj/PreCompiledHeader.pch

obj/UnityBuild.o: $(SRC_FILES) obj/PreCompiledHeader.pch
	@echo "[93mCompiling Elemental Native ($(Configuration) - $(PLATFORM) / $(OS))...[0m"
	$(CC) -c $(CFLAGS) $(PCHFLAGS) UnityBuild.cpp -o $@

../Elemental/$(OutputDirectory)Elemental.Native.$(LIB_EXTENSION): obj/UnityBuild.o obj/PreCompiledHeader.pch
	@echo "[93mLinking Tools Native ($(Configuration) - $(PLATFORM) / $(OS))...[0m"
	$(CC) $(CFLAGS) $(LDFLAGS) -o ../Elemental/$(OutputDirectory)Elemental.Native.$(LIB_EXTENSION) $<
	@echo "[92mElemental Native Compiled Successfully![0m"

Elemental.Native: ../Elemental/$(OutputDirectory)Elemental.Native.$(LIB_EXTENSION)

clean:
	rm -f obj/*\
		./Elemental/$(OutputDirectory)Elemental.Native.$(LIB_EXTENSION)