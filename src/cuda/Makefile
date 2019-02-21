CPP  = g++
NVCC := nvcc -ccbin $(CPP)
TARGET = ../../exe/lib/libprocuda.a
HOME = .

CU_FILE :=  $(HOME)/csp_transform_cu.cu \

OBJ_FILE := $(CU_FILE:.cu=.o) 

CPPFLAGS := -std=c++11 \
			-I$(HOME) \
			-I/usr/local/cuda/include \

CUDAFLAGS := -Xcompiler -fPIC -gencode arch=compute_53,code=sm_53 -gencode arch=compute_62,code=sm_62

all: $(TARGET)

$(TARGET): $(OBJ_FILE)
	ar rcv $@ $^ 

%o:%cu
	$(NVCC) $(CUDAFLAGS) $(CPPFLAGS) -o $@ -c $<

.PHONY:clean
clean:
	rm -r $(OBJ_FILE)
