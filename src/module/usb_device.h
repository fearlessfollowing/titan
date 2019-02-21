
#ifndef _USB_DEVICE_H_
#define _USB_DEVICE_H_

#include "libusb-1.0/libusb.h"
#include <thread>
#include <mutex>
#include <map>
#include <memory>
#include <condition_variable>
#include "common.h"

class dev_contex;

class usb_device
{
public:
    bool is_open(int pid);   
    bool is_all_open();
    bool is_all_close(); 
    int write_cmd(int pid, char* data, int size, unsigned int timeout);
    int read_cmd(int pid, char* data, int size, unsigned int timeout);
    int read_data(int pid, unsigned char* data, int size, unsigned int timeout);
    int write_data(int pid, unsigned char* data, int size, unsigned int timeout); //通过ep1发送
    int write_data2(int pid, unsigned char* data, int size, unsigned int timeout); //通过ep4发送
    void cancle_transfer(int32_t pid);

    static usb_device* create()
    {
        if (instance_ != nullptr) return instance_;
        instance_ = new usb_device;
        if (INS_OK != instance_->init())  
        {
            delete instance_;
            instance_ = nullptr; 
        }
        return instance_;
    };

    static void destroy()
    {
        if (instance_) delete instance_;
        instance_ = nullptr;
    };

    static usb_device* get()
    {
        //INS_ASSERT(instance_ != nullptr);
        return instance_;
    };

    static int hot_plug_cb_fn(libusb_context *ctx, libusb_device *device, libusb_hotplug_event event, void *user_data);

private:
    int init();
    void deinit();
    usb_device() {};
    ~usb_device() { deinit(); };

    void event_handle_task();
    //int swich_usb_err(int usb_err, int ins_err);
    int usb_transfer(int pid, int ep, unsigned char* data, int size, unsigned timeout);
    int32_t usb_transfer_async(int pid, int ep, unsigned char* data, int size, unsigned timeout);

    bool quit_ = false;
    std::thread th_;
    std::mutex mtx_;
    std::map<int32_t, libusb_device_handle*> pid_;
    std::map<int32_t, std::shared_ptr<dev_contex>> dev_ctx_;
    //libusb_context* hot_plug_ctx_ = nullptr;
    libusb_hotplug_callback_handle hot_plug_cb_handle_ = 0;

    static usb_device* instance_;
};

#endif