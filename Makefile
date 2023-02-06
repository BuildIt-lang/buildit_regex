BASE_DIR=$(shell pwd)
SRC_DIR=$(BASE_DIR)/src
BUILD_DIR?=$(BASE_DIR)/build
BUILDIT_DIR?=$(BASE_DIR)/buildit
INCLUDE_DIR=$(BASE_DIR)/include
GENERATED_CODE=$(BASE_DIR)/generated_code
TEST_DIR=$(BASE_DIR)/test
SAMPLES_DIR=$(BASE_DIR)/samples

SRCS=$(wildcard $(SRC_DIR)/*.cpp)
SAMPLES_SRCS=$(wildcard $(SAMPLES_DIR)/*.cpp)
OBJS=$(subst $(SRC_DIR),$(BUILD_DIR),$(SRCS:.cpp=.o))
SAMPLES=$(subst $(SAMPLES_DIR),$(BUILD_DIR),$(SAMPLES_SRCS:.cpp=))
TEST_SRCS=$(wildcard $(TEST_DIR)/*.cpp)
TESTS=$(subst $(TEST_DIR),$(BUILD_DIR),$(TEST_SRCS:.cpp=))

$(shell mkdir -p $(BUILD_DIR))
$(shell mkdir -p $(BUILD_DIR)/samples)
$(shell mkdir -p $(BUILD_DIR)/test)
$(shell mkdir -p $(GENERATED_CODE))

BUILDIT_LIBRARY_NAME=buildit
BUILDIT_LIBRARY_PATH=$(BUILDIT_DIR)/build

LIBRARY_NAME=buildit_regex
DEBUG ?= 0
ifeq ($(DEBUG),1)
CFLAGS=-g -std=c++11 -O0
LINKER_FLAGS=-rdynamic  -g -L$(BUILDIT_LIBRARY_PATH) -L$(BUILD_DIR) -l$(LIBRARY_NAME) -l$(BUILDIT_LIBRARY_NAME) -ldl -lomp -lpcrecpp
else
CFLAGS=-std=c++11 -O3
LINKER_FLAGS=-rdynamic  -L$(BUILDIT_LIBRARY_PATH) -L$(BUILD_DIR) -l$(LIBRARY_NAME) -l$(BUILDIT_LIBRARY_NAME) -ldl -lomp -lpcrecpp
endif

LIBRARY=$(BUILD_DIR)/lib$(LIBRARY_NAME).a

CFLAGS+=-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wmissing-declarations -Woverloaded-virtual -pedantic-errors -Wno-deprecated -Wdelete-non-virtual-dtor -Werror -fopenmp

all: executables 

.PHONY: subsystem
subsystem:
	make -C $(BUILDIT_DIR)

.PRECIOUS: $(BUILD_DIR)/samples/%.o
.PRECIOUS: $(BUILD_DIR)/test/%.o

INCLUDES=$(wildcard $(INCLUDE_DIR)/*.h) $(wildcard $(BUILDIT_DIR)/include/*.h) $(wildcard $(BUILDIT_DIR)/include/*/*.h) 
INCLUDE_FLAG=-I$(INCLUDE_DIR) -I$(BUILDIT_DIR)/include -I$(BUILDIT_DIR)/build/gen_headers

$(BUILD_DIR)/samples/%.o: $(SAMPLES_DIR)/%.cpp $(INCLUDES)
	$(CXX) $(CFLAGS) $< -o $@ $(INCLUDE_FLAG) -c 

$(BUILD_DIR)/test/%.o: $(TEST_DIR)/%.cpp $(INCLUDES)
	$(CXX) $(CFLAGS) $< -o $@ $(INCLUDE_FLAG) -c 

.PHONY: $(BUILDIT_LIBRARY_PATH)/lib$(BUILDIT_LIBRARY_NAME).a
$(BUILD_DIR)/sample%: $(BUILD_DIR)/samples/sample%.o $(LIBRARY) $(BUILDIT_LIBRARY_PATH)/lib$(BUILDIT_LIBRARY_NAME).a subsystem
	$(CXX) -o $@ $< $(LINKER_FLAGS)

.PHONY: $(BUILDIT_LIBRARY_PATH)/lib$(BUILDIT_LIBRARY_NAME).a
$(BUILD_DIR)/test%: $(BUILD_DIR)/test/test%.o $(LIBRARY) $(BUILDIT_LIBRARY_PATH)/lib$(BUILDIT_LIBRARY_NAME).a subsystem
	$(CXX) -o $@ $< $(LINKER_FLAGS)

.PHONY: executables
executables: $(SAMPLES) $(TESTS)

$(LIBRARY): $(OBJS)
	ar rv $(LIBRARY) $(OBJS)	

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(INCLUDES)
	$(CXX) $(CFLAGS) $< -o $@ $(INCLUDE_FLAG) -c

test: executables
	./build/test_full
	rm $(BUILDIT_DIR)/scratch/*

clean:
	rm -rf $(BUILD_DIR)
	rm $(BUILDIT_DIR)/scratch/*
