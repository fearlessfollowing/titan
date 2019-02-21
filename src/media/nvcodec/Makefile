CPP  = g++
TARGET = stitch_test

HOME = ../..
CPP_DIR = $(HOME)/nvmma/common/classes

CPP_FILE = $(shell find $(CPP_DIR) -name "*.cpp") \
	   stitch_test.cpp \
	   nv_video_dec.cpp \
	   nv_video_enc.cpp \
	   video_stitcher.cpp \
	   $(HOME)/media/insdemux.cpp \
	   $(HOME)/media/insmux.cpp \
	   $(HOME)/media/ffutil.cpp \
	   $(HOME)/media/spherical_metadata.cpp \
	   $(HOME)/util/inslog.cpp \
	   $(HOME)/procimg/ins_blender.cpp  \

OBJ_FILE = $(CPP_FILE:.cpp=.o) 

CPPFLAGS := \
	-std=c++11 \
	-DSIMPLE_TEST \
	-I$(HOME)/nvmma \
	-I$(HOME)/nvmma/include \
	-I$(HOME)/nvmma/include/libjpeg-8b \
	-I$(HOME)/util \
	-I$(HOME)/media \
	-I$(HOME)/procimg \
	-I$(HOME)/AutoStitcherLib2 \
	-I/usr/local/cuda/include \
	-I/usr/local/ffmpeg/include

LDFLAGS :=  \
	-lpthread \
	-lv4l2 -lEGL -lGLESv2 \
	-lnvbuf_utils -lnvjpeg \
	-lcuda -lcudart \
	-lavformat -lavcodec -lavutil -lswscale -lx264 \
	-L/usr/lib/aarch64-linux-gnu/tegra \
	-L/usr/local/ffmpeg/lib \
	-L/usr/local/cuda/lib64 \
	-Wl,-rpath=/usr/local/ffmpeg/lib \
	-Wl,-rpath=$(HOME)/stitch \
	-Wl,-rpath=/usr/local/opencv/lib

LDFLAGS += \
	-L$(HOME)/.libs \
	-lautostitch \
	-lopencv_ml \
	-lopencv_highgui \
	-lopencv_videoio  \
	-lopencv_core \
	-lopencv_imgproc \
	-lopencv_calib3d \
	-lopencv_features2d \
	-lopencv_xfeatures2d \
	-lopencv_stitching \
	-lopencv_flann \
	-lopencv_imgcodecs \
	-lopencv_objdetect  \
	-lopencv_video \
	-lopencv_cudawarping \
	-lopencv_cudaarithm \
	-lopencv_cudaimgproc \
	-lopencv_cudafilters \
	-lopencv_photo \
	-lopencv_cudev \
	-lcufft \
	-lcudart \
	-lz \
	-lceres \
	-L/usr/local/opencv/lib \
	-Wl,-rpath=/usr/local/opencv/lib \

all: $(TARGET)

$(TARGET): $(OBJ_FILE)
	$(CPP) -o $@ $^ $(LDFLAGS)

.cpp.o:
	$(CPP) $(CPPFLAGS) -o $@ -c $<

.PHONY:clean
clean:
	rm -rf $(OBJ_FILE)
