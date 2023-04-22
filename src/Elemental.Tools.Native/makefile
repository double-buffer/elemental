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

ifeq ($(Configuration), Debug)
DBG_OPT_FLAGS=-g
else
DBG_OPT_FLAGS=-O2
endif

INCLUDES=-I../Platforms/Common/\
		 -I./Interop/\
		 -I../../external/DirectX-Headers/include/directx/\
		 -I../../external/shader-compilers/dxc/include/dxc/\
		 -I../../external/shader-compilers/spirv-cross/include/

LIBS=-liconv\
	 -lspirv-cross-core\
	 -lspirv-cross-cpp\
	 -lspirv-cross-msl\
	 -lspirv-cross-glsl\
	 -lspirv-cross-reflect\
	 -lspirv-cross-util\
	 -L../../external/shader-compilers/spirv-cross/lib/

CC=clang++
CFLAGS=-Wall -std=c++20 -fms-extensions -fno-objc-arc $(INCLUDES) $(DBG_OPT_FLAGS)
LDFLAGS=$(LIBS) -dynamiclib

SRC_FILES = $(filter-out UnityBuild.cpp, $(wildcard *.cpp))
 
all: Elemental.Tools.Native

obj/UnityBuild.o: $(SRC_FILES)
	@echo "[93mCompiling Tools Native ($(Configuration) - $(PLATFORM) / $(OS))...[0m"
	$(CC) -c $(CFLAGS) UnityBuild.cpp -o $@

../Elemental.Tools/$(OutputDirectory)Elemental.Tools.Native.dylib: obj/UnityBuild.o
	@echo "[93mLinking Tools Native ($(Configuration) - $(PLATFORM) / $(OS))...[0m"
	$(CC) $(CFLAGS) $(LDFLAGS) -o ../Elemental.Tools/$(OutputDirectory)Elemental.Tools.Native.dylib $<
	@echo "[92mTools Native Compiled Successfully![0m"

Elemental.Tools.Native: ../Elemental.Tools/$(OutputDirectory)Elemental.Tools.Native.dylib