CPP  = g++
TARGET = atest

CPP_FILE := main.cpp ../../util/inslog.cpp

OBJ_FILE := $(CPP_FILE:.cpp=.o)

CPPFLAGS := -std=c++14 -I../../util

LDFLAGS := -lasound -lpthread

all: $(TARGET) 

$(TARGET): $(OBJ_FILE)
	$(CPP) $^ -o $@ $(LDFLAGS)

.cpp.o:
	$(CPP) $(CPPFLAGS) -c $< -o $@

.PHONY:clean
clean:
	rm -r $(OBJ_FILE) 
