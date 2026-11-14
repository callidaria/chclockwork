CC = g++

SDST_CORE = core/
SDST_SCRIPT = script/
LDST = lib/

ifeq ($(OS),Windows_NT)
	INCLUDE = -I/mingw64/include -I/mingw64/include/freetype2 -I/mingw64/include/libpng16
	LINKER = -lpthread -lglew32 -lopengl32 -lSDL2main -lSDL2 -lassimp -lfreetype
	TARGET = chcw.exe
else
	INCLUDE = -I/usr/include/freetype2 -I/usr/include/libpng16
	LINKER = -lpthread -lGL -lGLEW -lSDL2 -lassimp -lfreetype -lvulkan
	TARGET = chcw
endif

DEBUG_SUFFIX = -O0 -DDEBUG -pg -g
RELEASE_SUFFIX = -O3 -fno-gcse
GPUAPI_SUFFIX ?= -DVKBUILD

rcwild = $(foreach d,$(wildcard $1*),$(call rcwild,$d/,$2) $(filter $(subst *,%,$2),$d))
SRCS_CORE = $(call rcwild,$(SDST_CORE),*.cpp)
SRCS_SCRIPT = $(call rcwild,$(SDST_SCRIPT),*.cpp)
SRCS = $(SRCS_CORE) $(SRCS_SCRIPT)
OBJS = $(SRCS:%.cpp=$(LDST)%.o)
MAIN = main.cpp

CXXFLAGS =


all: $(TARGET)

debug: CXXFLAGS = $(DEBUG_SUFFIX) $(GPUAPI_SUFFIX)
debug: $(TARGET)

release: CXXFLAGS = $(RELEASE_SUFFIX) $(GPUAPI_SUFFIX)
release: $(TARGET)

$(TARGET): $(OBJS) $(MAIN)
	@mkdir -p $(dir $@)
	$(CC) $(MAIN) $(OBJS) -o $@ $(INCLUDE) $(LINKER) $(CXXFLAGS)

$(LDST)%.o: $(SDST)%.cpp $(SDST)%.h
	@mkdir -p $(dir $@)
	$(CC) $< -o $@ -c $(INCLUDE) $(CXXFLAGS)
