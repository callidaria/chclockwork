CC = g++

SDST_CORE = core/
SDST_SCRIPT = script/
LDST = lib/
TARGET = chcw

INCLUDE = -I/usr/include/freetype2 -I/usr/include/libpng16
LINKER = -lGL -lGLEW -lSDL2 -lpthread -lfreetype

DEBUG_SUFFIX = -pg -g -O0 -DDEBUG
RELEASE_SUFFIX = -O3 -fno-gcse

SRCS_CORE = $(wildcard $(SDST_CORE)*.cpp)
SRCS_SCRIPT = $(wildcard $(SDST_SCRIPT)*.cpp)
SRCS = $(SRCS_CORE) $(SRCS_SCRIPT)
OBJS = $(SRCS:%.cpp=$(LDST)%.o)
MAIN = main.cpp

PROJECT_SUFFIX = -DPROJECT_PONG

CXXFLAGS =


all: $(TARGET)

debug: CXXFLAGS = $(DEBUG_SUFFIX) $(PROJECT_SUFFIX)
debug: $(TARGET)

release: CXXFLAGS = $(RELEASE_SUFFIX) $(PROJECT_SUFFIX)
release: $(TARGET)

$(TARGET): $(OBJS) $(MAIN)
	@mkdir -p $(dir $@)
	$(CC) $(MAIN) $(OBJS) -o $@ $(INCLUDE) $(LINKER) $(CXXFLAGS)

$(LDST)%.o: $(SDST)%.cpp $(SDST)%.h
	@mkdir -p $(dir $@)
	$(CC) $< -o $@ -c $(INCLUDE) $(CXXFLAGS)
