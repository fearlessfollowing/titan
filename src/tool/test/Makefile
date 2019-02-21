CPP  = g++
TARGET = mytest
SRC_PATH = .
OBJ_PATH = ./obj

CPP_FILE := $(SRC_PATH)/main.cpp \

OBJ_FILE_TMP = $(patsubst $(SRC_PATH)%.cpp,%.o,$(CPP_FILE)) 
OBJ_FILE = $(addprefix $(OBJ_PATH),$(OBJ_FILE_TMP))
DEPS := $(OBJ_FILE:.o=.d)

CPPFLAGS := -std=c++14 \

LDFLAGS := -lpthread

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
