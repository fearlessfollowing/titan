CPP  = g++
TARGET = msgclient
SRC_PATH = ../..
OBJ_PATH = ./obj
EXE = ../../../exe

CPP_FILE := $(SRC_PATH)/tool/msg_client/main.cpp \
			$(SRC_PATH)/util/inslog.cpp \
			$(SRC_PATH)/common/json_obj.cpp \
			$(SRC_PATH)/access/fifo_read.cpp \
			$(SRC_PATH)/access/fifo_write.cpp \
			$(SRC_PATH)/util/ins_util.cpp \

# OBJ_FILE = $(CPP_FILE:.cpp=.o) 
# DEPS = $(CPP_FILE:.cpp=.d) 
OBJ_FILE_TMP = $(patsubst $(SRC_PATH)%.cpp,%.o,$(CPP_FILE)) 
OBJ_FILE = $(addprefix $(OBJ_PATH),$(OBJ_FILE_TMP))
DEPS := $(OBJ_FILE:.o=.d)

CPPFLAGS := \
	-std=c++11 \
	-I$(SRC_PATH)/common \
	-I$(SRC_PATH)/util \
	-I$(SRC_PATH)/access \
	-I$(EXE)/include \

LDFLAGS :=  \
	-lev \
	-ljson-c \
	-lpthread \
	-L$(EXE)/lib \
	-Wl,-rpath=/usr/local/lib

all: $(TARGET)

-include $(DEPS)

$(TARGET): $(OBJ_FILE)
	$(CPP) -o $@ $^ $(LDFLAGS)

# .cpp.o:
# 	$(CPP) $(CPPFLAGS) -o $@ -c $< -MD -MF $*.d -MP

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
