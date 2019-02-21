CPP  = g++
HOME = .
TARGET = ../../exe/lib/libnvmma.a

CPP_DIR = $(HOME)/common/classes
CPP_FILE = $(shell find $(CPP_DIR) -name "*.cpp") 
OBJ_FILE = $(CPP_FILE:.cpp=.o) 

CPPFLAGS := \
	-std=c++11 -O2 \
	-I$(HOME)/include \
	-I$(HOME)/include/libjpeg-8b \

all: $(TARGET)

$(TARGET): $(OBJ_FILE)
	ar rcv $@ $^ 

.cpp.o:
	$(CPP) $(CPPFLAGS) -o $@ -c $<

.PHONY:clean
clean:
	rm -rf $(OBJ_FILE)
