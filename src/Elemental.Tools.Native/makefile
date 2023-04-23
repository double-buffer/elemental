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

# Set common compiler parameters
# TODO: Debug defines per platform
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

# Set compiler and flags based on the operating system
ifeq ($(OS), Windows)
    CC = cl
    CFLAGS = /nologo /MD /Wall /std:c++latest /EHsc /DUNICODE /D_UNICODE /D_WINDOWS /D_USRDLL /D_WINDLL $(INCLUDES) $(DBG_OPT_FLAGS)
    LDFLAGS = /DLL $(LIBS)
	# TODO:
	PCHFLAGS = -include-pch obj/PreCompiledHeader.pch
	LIB_EXTENSION = dll
else
	CC = clang++
	CFLAGS = -Wall -std=c++20 -fms-extensions -fno-objc-arc $(INCLUDES) $(DBG_OPT_FLAGS)
	PCHFLAGS = -include-pch obj/PreCompiledHeader.pch
	LDFLAGS = -dynamiclib $(LIBS) 
	LIB_EXTENSION = dylib
endif

# Get source files dependency
SRC_FILES = $(filter-out UnityBuild.cpp, $(wildcard *.cpp *.h **/*.cpp **/*.h))
 
# Create obj folder if it doesn't exist
$(shell mkdir -p obj)

# Targets
.PHONY: clean all
all: Elemental.Tools.Native

obj/PreCompiledHeader.pch: PreCompiledHeader.h
	$(CC) -c $(CFLAGS) -x c++-header PreCompiledHeader.h -o obj/PreCompiledHeader.pch

obj/UnityBuild.o: $(SRC_FILES) obj/PreCompiledHeader.pch
	@echo "[93mCompiling Tools Native ($(Configuration) - $(PLATFORM) / $(OS))...[0m"
	$(CC) -c $(CFLAGS) $(PCHFLAGS) UnityBuild.cpp -o $@

../Elemental.Tools/$(OutputDirectory)Elemental.Tools.Native.$(LIB_EXTENSION): obj/UnityBuild.o obj/PreCompiledHeader.pch
	@echo "[93mLinking Tools Native ($(Configuration) - $(PLATFORM) / $(OS))...[0m"
	$(CC) $(CFLAGS) $(LDFLAGS) -o ../Elemental.Tools/$(OutputDirectory)Elemental.Tools.Native.$(LIB_EXTENSION) $<
	@echo "[92mTools Native Compiled Successfully![0m"

Elemental.Tools.Native: ../Elemental.Tools/$(OutputDirectory)Elemental.Tools.Native.$(LIB_EXTENSION)

clean:
	rm -f obj/*\
		./Elemental.Tools/$(OutputDirectory)Elemental.Tools.Native.$(LIB_EXTENSION)