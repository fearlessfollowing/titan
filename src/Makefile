CPP  = g++
NVCC := nvcc -ccbin $(CPP)
TARGET = camerad
SRC_PATH = .
OBJ_PATH := ./obj
EXE = ../exe

COMPILE_TIME = \"$(shell date '+%m%d%H')\" 

CPP_FILE := \
			$(SRC_PATH)/media/insmux.cpp \
			$(SRC_PATH)/media/insdemux.cpp \
			$(SRC_PATH)/media/ffaacdec.cpp \
			$(SRC_PATH)/media/ffaacenc.cpp \
			$(SRC_PATH)/media/ffh264dec.cpp \
			$(SRC_PATH)/media/ffh264enc.cpp \
			$(SRC_PATH)/media/ffpngdec.cpp \
			$(SRC_PATH)/media/ff_enc_wrap.cpp \
			$(SRC_PATH)/media/insmedia.cpp \
			$(SRC_PATH)/media/tjpegdec.cpp \
			$(SRC_PATH)/media/tjpegenc.cpp \
			$(SRC_PATH)/media/stream_sink.cpp \
			$(SRC_PATH)/media/file_sink.cpp \
			$(SRC_PATH)/media/pic_seq_sink.cpp \
			$(SRC_PATH)/media/jpeg_muxer.cpp \
			$(SRC_PATH)/media/spherical_metadata.cpp \
			$(SRC_PATH)/media/ffutil.cpp \
			$(SRC_PATH)/media/camm_util.cpp \
			$(SRC_PATH)/media/exif_parser.cpp \
			$(SRC_PATH)/media/tiff_demux.cpp \
			$(SRC_PATH)/media/tiff_mux.cpp \
			$(SRC_PATH)/media/nvcodec/nv_video_dec.cpp \
	   		$(SRC_PATH)/media/nvcodec/nv_video_enc.cpp \
			$(SRC_PATH)/media/nvcodec/nv_jpeg_dec.cpp \
			$(SRC_PATH)/media/nvcodec/nv_jpeg_enc.cpp \
			$(SRC_PATH)/media/sps_parser.cpp \
			$(SRC_PATH)/render/render.cpp \
			$(SRC_PATH)/render/render_screen.cpp \
			$(SRC_PATH)/render/render_base.cpp \
			$(SRC_PATH)/render/insegl.cpp \
			$(SRC_PATH)/render/insx11.cpp \
			$(SRC_PATH)/render/shader.cpp \
			$(SRC_PATH)/render/singlen_render.cpp \
			$(SRC_PATH)/render/singlen_shader.cpp \
			$(SRC_PATH)/cuda/csp_transform.cpp \
			$(SRC_PATH)/procimg/ins_hdr_merger.cpp \
			$(SRC_PATH)/procimg/ins_len_circle.cpp \
			$(SRC_PATH)/procimg/ins_len_offset.cpp \
			$(SRC_PATH)/procimg/ins_blender.cpp \
			$(SRC_PATH)/procimg/all_lens_hdr.cpp \
			$(SRC_PATH)/procimg/hdr.cpp \
			$(SRC_PATH)/procimg/ins_offset_conv.cpp \
			$(SRC_PATH)/core/gps_mgr.cpp \
			$(SRC_PATH)/core/storage_speed_test.cpp \
			$(SRC_PATH)/core/temp_monitor.cpp \
			$(SRC_PATH)/core/storage_test.cpp \
			$(SRC_PATH)/core/video_mgr.cpp \
			$(SRC_PATH)/core/video_composer.cpp \
			$(SRC_PATH)/core/image_mgr.cpp \
			$(SRC_PATH)/core/timelapse_mgr.cpp \
			$(SRC_PATH)/core/gps_file.cpp \
			$(SRC_PATH)/core/gps_dev.cpp \
			$(SRC_PATH)/core/singlen_focus.cpp \
			$(SRC_PATH)/core/singlen_mgr.cpp \
			$(SRC_PATH)/core/qr_scanner.cpp \
			$(SRC_PATH)/core/camera_info.cpp \
			$(SRC_PATH)/core/audio_dev.cpp \
			$(SRC_PATH)/core/audio_dev_monitor.cpp \
			$(SRC_PATH)/core/audio_reader.cpp \
			$(SRC_PATH)/core/audio_fmt_conv.cpp \
			$(SRC_PATH)/core/audio_mgr.cpp \
			$(SRC_PATH)/core/audio_spatial.cpp \
			$(SRC_PATH)/core/audio_play.cpp \
			$(SRC_PATH)/core/audio_ace.cpp \
			$(SRC_PATH)/core/img_seq_composer.cpp \
			$(SRC_PATH)/core/audio_record.cpp \
			$(SRC_PATH)/core/usb_sink.cpp \
			$(SRC_PATH)/core/gyro_file.cpp \
			$(SRC_PATH)/core/exposure_file.cpp \
			$(SRC_PATH)/core/stabilization.cpp \
			$(SRC_PATH)/core/hdmi_monitor.cpp \
			$(SRC_PATH)/core/jpeg_preview_sink.cpp \
			$(SRC_PATH)/module/usb_device.cpp \
			$(SRC_PATH)/module/usb_camera.cpp  \
			$(SRC_PATH)/module/cam_manager.cpp \
			$(SRC_PATH)/module/all_cam_video_buff.cpp \
			$(SRC_PATH)/module/one_cam_video_buff.cpp \
			$(SRC_PATH)/module/all_cam_origin_stream.cpp \
			$(SRC_PATH)/module/all_cam_video_queue.cpp \
			$(SRC_PATH)/module/cam_img_repo.cpp \
			$(SRC_PATH)/common/prj_file_mgr.cpp \
			$(SRC_PATH)/common/xml_config.cpp \
			$(SRC_PATH)/common/json_obj.cpp \
			$(SRC_PATH)/common/prj_file_parser.cpp \
			$(SRC_PATH)/common/gyro_data_parser.cpp \
			$(SRC_PATH)/common/rotation_mat_calculator.cpp \
			$(SRC_PATH)/common/offset_wrap.cpp \
			$(SRC_PATH)/common/hdr_wrap.cpp \
			$(SRC_PATH)/common/img_enc_wrap.cpp \
			$(SRC_PATH)/common/ins_i2c.cpp \
			$(SRC_PATH)/common/ins_battery.cpp \
			$(SRC_PATH)/common/system_properties.cpp\
			$(SRC_PATH)/util/inslog.cpp \
			$(SRC_PATH)/util/tinyxml2.cpp \
			$(SRC_PATH)/util/inserr.cpp \
			$(SRC_PATH)/util/ins_timer.cpp \
			$(SRC_PATH)/util/ins_signal.cpp \
			$(SRC_PATH)/util/ins_md5.cpp \
			$(SRC_PATH)/util/ins_base64.cpp \
			$(SRC_PATH)/util/ins_uuid.cpp \
			$(SRC_PATH)/util/ins_util.cpp \
			$(SRC_PATH)/util/hw_util.cpp \
			$(SRC_PATH)/access/access_msg_parser.cpp \
			$(SRC_PATH)/access/access_msg_receiver.cpp \
			$(SRC_PATH)/access/access_msg_sender.cpp \
			$(SRC_PATH)/access/upload_cal_file.cpp \
			$(SRC_PATH)/access/access_state.cpp \
			$(SRC_PATH)/access/fifo_write.cpp \
			$(SRC_PATH)/access/fifo_read.cpp \
			$(SRC_PATH)/access/main.cpp \
			$(SRC_PATH)/access/access_msg_center.cpp \

#$(SRC_PATH)/media/nvcodec/enc_test.cpp 
#OBJ_FILE := $(CPP_FILE:.cpp=.o)
OBJ_FILE_TMP = $(patsubst $(SRC_PATH)%.cpp,%.o,$(CPP_FILE)) 
OBJ_FILE = $(addprefix $(OBJ_PATH),$(OBJ_FILE_TMP))
DEPS := $(OBJ_FILE:.o=.d) 

CPPFLAGS := \
	-std=c++14 -O2\
	-DCOMPILE_TIME=$(COMPILE_TIME) \
	-Wno-unused-result \
	-Wno-int-to-pointer-cast \
	-Wno-deprecated-declarations \
	-I$(SRC_PATH)/media \
	-I$(SRC_PATH)/module \
	-I$(SRC_PATH)/AutoStitcherLib2 \
	-I$(SRC_PATH)/GyrostabLib/MagneticCalibration \
	-I$(SRC_PATH)/GyrostabLib \
	-I$(SRC_PATH)/eigen-3.3.1 \
	-I$(SRC_PATH)/util \
	-I$(SRC_PATH)/core \
	-I$(SRC_PATH)/cuda \
	-I$(SRC_PATH)/render \
	-I$(SRC_PATH)/box \
	-I$(SRC_PATH)/common \
	-I$(SRC_PATH)/access \
	-I$(SRC_PATH)/procimg \
	-I$(SRC_PATH)/media/nvcodec \
	-I$(SRC_PATH)/nvmma/include \
	-I/usr/local/include \
	-I/usr/local/cuda/include \
	-I$(EXE)/include \

LDFLAGS :=  \
	-lev \
	-ljson-c \
	-ltiff \
	-lturbojpeg \
	-lusb-1.0 \
	-lzbar \
	-lleveldb \
	-lexpat \
	-lexiv2 \
	-luuid \
	-lublox \
	-lb64 \
	-lfdk-aac \
 	-ljpeg \
	-lspeexdsp \
	-lTwirlingCapture \
	-lasound \
	-lpthread \
	-lEGL -lGLESv2 -lX11\
	-lcuda -lcudart \
	-lavformat -lavcodec -lavutil -lswscale -lx264 \
	-L/usr/local/cuda/lib64 \
	-L/usr/lib/aarch64-linux-gnu/tegra \
	-L$(EXE)/lib \
	-Wl,-rpath=/usr/local/lib \

#auto stitch lib
LDFLAGS += \
	-lprocuda \
	-lautostitch \
	-lopencv_ml \
	-lopencv_highgui \
	-lopencv_videoio  \
	-lopencv_core \
	-lopencv_imgproc \
	-lopencv_calib3d \
	-lopencv_features2d \
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
	-lopencv_xfeatures2d \
 	-lopencv_cudafeatures2d \
 	-lopencv_cudalegacy \
	-lcufft \
	-lcudart \
	-lz \
	-lceres \

#tegra mutimedia lib
LDFLAGS += -lnvmma -lnvbuf_utils -lnvjpeg -lv4l2 \

#gyro stablizer lib
LDFLAGS += -lstab 

all: $(TARGET) 

-include $(DEPS)

$(TARGET): $(OBJ_FILE)
	$(CPP) $^ -o $@ $(LDFLAGS)

$(OBJ_FILE): $(OBJ_PATH)/%.o : $(SRC_PATH)/%.cpp
	@mkdir -p $(shell dirname $@)
	$(CPP) $(CPPFLAGS) -c $< -o $@ -MD -MF $(OBJ_PATH)/$*.d -MP

.PHONY:clean
clean:
	rm -r $(OBJ_FILE) $(DEPS)

.PHONY:install
install:
	@echo
	cp $(TARGET) /usr/local/bin
	cp $(TARGET) $(EXE)/bin

.PHONY:remote
remote:
	scp $(TARGET) nvidia@192.168.2.44:/home/nvidia
