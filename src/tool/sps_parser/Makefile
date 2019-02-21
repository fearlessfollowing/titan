CPP  = g++
TARGET = sps_parser
SRC_PATH = ../..
OBJ_PATH = ./obj
EXE = ../../../exe

CPP_FILE := $(SRC_PATH)/tool/sps_parser/main.cpp \
			$(SRC_PATH)/media/sps_parser.cpp 

OBJ_FILE_TMP = $(patsubst $(SRC_PATH)%.cpp,%.o,$(CPP_FILE)) 
OBJ_FILE = $(addprefix $(OBJ_PATH),$(OBJ_FILE_TMP))
DEPS := $(OBJ_FILE:.o=.d)

CPPFLAGS := \
	-std=c++14 \
	-I$(SRC_PATH)/media \

LDFLAGS :=  \
	-L$(EXE)/lib \
	-Wl,-rpath=/usr/local/lib

all: $(TARGET)

-include $(DEPS)

$(TARGET): $(OBJ_FILE)
	$(CPP) -o $@ $^ $(LDFLAGS)

$(OBJ_FILE): $(OBJ_PATH)/%.o : $(SRC_PATH)/%.cpp
	@mkdir -p $(shell dirname $@)
	$(CPP) $(CPPFLAGS) -c $< -o $@ -MD -MF $(OBJ_PATH)/$*.d -MP

.PHONY:clean
clean:
	rm -rf $(OBJ_FILE) $(DEPS)

.PHONY:install
install:
	@echo
	cp $(TARGET) /usr/local/bin
	cp $(TARGET) $(EXE)/bin
