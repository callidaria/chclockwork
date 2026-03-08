CC = g++

SDST = */
LDST = lib/

LINKER = -lpthread
TARGET = chcctool

RELEASE_SUFFIX = -O3 -fno-gcse

SRCS = $(wildcard $(SDST)*.c)
OBJS = $(SRCS:%.cpp=$(LDST)%.o)
MAIN = tools.cpp

all: $(TARGET)

tool: CXXFLAGS = $(RELEASE_SUFFIX)
tool: $(TARGET)

$(TARGET): $(OBJS) $(MAIN)
	@mkdir -p $(dir $@)
	$(CC) $(MAIN) $(OBJS) -o $@ $(LINKER) $(CXXFLAGS)

$(LDST)%.o: $(SDST)%.cpp $(SDST)%.h
	@mkdir -p $(dir $@)
	$(CC) $< -o $@ -c $(CXXFLAGS)
