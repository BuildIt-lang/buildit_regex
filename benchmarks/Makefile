BASE_DIR=$(shell pwd)
BUILD_DIR?=$(BASE_DIR)/build
BUILDIT_DIR=$(BASE_DIR)/../buildit
BUILDIT_REGEX_DIR=$(BASE_DIR)/..
RE2_DIR=$(BASE_DIR)/re2
HYPERSCAN_DIR=$(BASE_DIR)/hyperscan

SRCS=$(wildcard $(BASE_DIR)/*.cpp)
OBJS=$(subst $(BASE_DIR),$(BUILD_DIR),$(SRCS:.cpp=.o))
$(shell export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$RE2_DIR/obj/so)
$(shell mkdir -p $(BUILD_DIR))

LIBRARY_NAME=perf
DEBUG ?= 0
ifeq ($(DEBUG),1)
CFLAGS=-g -std=c++11 -O0 -fopenmp
LINKER_FLAGS=-rdynamic  -g -L$(BUILDIT_DIR)/build -L$(BUILDIT_REGEX_DIR)/build -L$(BUILD_DIR) -L$(RE2_DIR)/obj/so -L$(HYPERSCAN_DIR)/build/lib -l$(LIBRARY_NAME) -lbuildit -lbuildit_regex -lre2 -lhs -ldl -lomp
else
CFLAGS=-std=c++11 -O3 -fopenmp
LINKER_FLAGS=-rdynamic -L$(BUILDIT_DIR)/build -L$(BUILDIT_REGEX_DIR)/build -L$(BUILD_DIR) -L$(RE2_DIR)/obj/so -L$(HYPERSCAN_DIR)/build/lib -l$(LIBRARY_NAME) -lbuildit -lbuildit_regex -lre2 -lhs -ldl -lomp
endif

LIBRARY=$(BUILD_DIR)/lib$(LIBRARY_NAME).a

#CFLAGS+=-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wmissing-declarations -Woverloaded-virtual -pedantic-errors -Wno-deprecated -Wdelete-non-virtual-dtor -Werror

INCLUDES=$(wildcard $(BASE_DIR)/*.h) $(wildcard $(RE2_DIR)/*/*.h) $(wildcard $(BUILDIT_DIR)/include/*.h) $(wildcard $(BUILDIT_DIR)/include/*/*.h) $(BUILDIT_REGEX_DIR)/include/*.h 
INCLUDE_FLAG=-I$(BASE_DIR) -I$(HYPERSCAN_DIR) -I$(RE2_DIR) -I$(BUILDIT_DIR)/include -I$(BUILDIT_DIR)/build/gen_headers -I$(BUILDIT_REGEX_DIR)/include

all: $(BUILD_DIR)/performance.o $(BUILD_DIR)/performance

$(BUILD_DIR)/performance.o: $(BASE_DIR)/performance.cpp $(INCLUDES)
	$(CXX) $(CFLAGS) $< -o $@ $(INCLUDE_FLAG) -c 

$(BUILD_DIR)/performance: $(BUILD_DIR)/performance.o $(LIBRARY) $(BUILDIT_DIR)/build/libbuildit.a $(BUILDIT_REGEX_DIR)/build/libbuildit_regex.a $(RE2_DIR)/obj/so/libre2.so.10 $(HYPERSCAN_DIR)/build/lib/libhs.a
	$(CXX) -o $@ $< $(LINKER_FLAGS)


$(LIBRARY): $(OBJS)
	ar rv $(LIBRARY) $(OBJS)	

run: $(BUILD_DIR)/performance
	./build/performance
#	rm $(BUILDIT_DIR)/scratch/*

clean:
	rm -rf $(BUILD_DIR)
	rm $(BUILDIT_DIR)/scratch/*
