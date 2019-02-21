CPP  = g++
TARGET = atest

CPP_FILE := main.cpp ../../util/inslog.cpp ../../core/audio_fmt_conv.cpp

OBJ_FILE := $(CPP_FILE:.cpp=.o)

CPPFLAGS := -std=c++14 -I../../util -I../../core -I../../common -I/home/nvidia/work/speexdsp-1.2rc3/include

LDFLAGS := -lasound -lpthread -lspeexdsp -L. -Wl,-rpath=/usr/local/lib

all: $(TARGET) 

$(TARGET): $(OBJ_FILE)
	$(CPP) $^ -o $@ $(LDFLAGS)

.cpp.o:
	$(CPP) $(CPPFLAGS) -c $< -o $@

.PHONY:clean
clean:
	rm -r $(OBJ_FILE) 
