
#include "usb_device.h"
#include "inslog.h"
#include <unistd.h>

struct dev_contex
{
    std::condition_variable cv;
    std::mutex mtx;
    bool stop = false;
    libusb_transfer* transfer = nullptr;
};

#define ENDPOINT_SEND_CMD  (0x01)
#define ENDPOINT_SEND_DATA (0x04)
#define ENDPOINT_RECV_CMD  (0x83)
#define ENDPOINT_RECV_DATA (0x82)
#define PRO_USB_VID        (0x4255)
//#define PRO_USB_VID        (0x2E1A)

usb_device* usb_device::instance_ = nullptr;

// Must return 0  or will cause this callback to be deregistered
int usb_device::hot_plug_cb_fn(libusb_context *ctx, libusb_device *device, libusb_hotplug_event event, void *user_data)
{
	int ret = 0;
	struct libusb_device_descriptor desc;

    usb_device* instance = (usb_device*)user_data;
    if (instance == nullptr)
    {
        LOGERR("callback user_data is nullptr");
        return 0;
    }

	if(event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT)
	{
		ret = libusb_get_device_descriptor(device, &desc);
		if(ret < 0)
		{
			LOGERR("libusb_get_device_descriptor fail:%s",libusb_error_name(ret));
			return 0;
		}
		
		if(desc.idVendor != PRO_USB_VID)
		{
            LOGERR("recv dev left event, vid:0x%x != 0x%x", desc.idVendor, PRO_USB_VID);
            return 0;
        }

        LOGINFO("usb device left pid:0x%x", desc.idProduct);

        instance->mtx_.lock();
        auto it = instance->pid_.find(desc.idProduct);
        if (it == instance->pid_.end())
        {
            LOGERR("pid:%x correct??????", desc.idProduct);
        }
        else if (it->second != nullptr)
        {
            libusb_release_interface(it->second, 0);
            libusb_close(it->second);
            it->second = nullptr;
        }
        instance->mtx_.unlock();
	}
	else if(event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED)
	{
		// wait 100ms and get device descriptor, sometime device descriptor is not ready
		usleep(100*1000);		
		ret = libusb_get_device_descriptor(device, &desc);
		if(ret < 0)
		{
			LOGERR("libusb_get_device_descriptor fail:%s",libusb_error_name(ret));
			return 0;
		}
		
		if(desc.idVendor != PRO_USB_VID)
		{
            LOGERR("recv dev arrive event vid:0x%x != 0x%x", desc.idVendor, PRO_USB_VID);
            return 0;
        }

        LOGINFO("usb device arrive pid:0x%x", desc.idProduct);
        auto handle = libusb_open_device_with_vid_pid(nullptr, desc.idVendor, desc.idProduct);
        if (handle == nullptr)
        {
            LOGERR("libusb_open_device_with_vid_pid fail pid:0x%x", desc.idProduct);
            return 0;
        }

        ret = libusb_claim_interface(handle, 0);
        if (ret < 0)
        {
            LOGERR("libusb_claim_interface pid:0x%x fail:%s", desc.idProduct, libusb_error_name(ret));
            libusb_release_interface(handle, 0);
            libusb_close(handle);
            return 0;
        }

        instance->mtx_.lock();
        auto it = instance->pid_.find(desc.idProduct);
        if (it == instance->pid_.end() || it->second)
        {
            LOGERR("pid:%x correct??????", desc.idProduct);
        }
        else
        {
            it->second = handle;
        }
        instance->mtx_.unlock();
	}
	return 0;
} 


int usb_device::init()
{
	int ret = 0;
	ret = libusb_init(nullptr);
	if(ret < 0)
	{
		LOGERR("libusb_init fail:%s", libusb_error_name(ret));
		return INS_ERR;
	}

    //libusb_set_debug(nullptr, LIBUSB_LOG_LEVEL_INFO);

    for (int i = 0; i < INS_CAM_NUM; i++)
    {
        pid_.insert(std::make_pair(i+1, nullptr));
        auto ctx  = std::make_shared<dev_contex>();
        dev_ctx_.insert(std::make_pair(i+1, ctx));
    }

	ret = libusb_hotplug_register_callback(nullptr, 
                (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), 
				LIBUSB_HOTPLUG_ENUMERATE, 
				PRO_USB_VID, 
                LIBUSB_HOTPLUG_MATCH_ANY, 
                LIBUSB_HOTPLUG_MATCH_ANY, 
				hot_plug_cb_fn, this, &hot_plug_cb_handle_);

	if(ret != LIBUSB_SUCCESS) 
	{
		LOGERR("libusb_hotplug_register_callback fail");
		libusb_exit(nullptr);
		return INS_ERR;
	}

    th_ = std::thread(&usb_device::event_handle_task, this);
	
    LOGINFO("libusb init success");

	return INS_OK;
}

void usb_device::deinit()
{
    quit_ = true;
    INS_THREAD_JOIN(th_);

    for (auto it = pid_.begin(); it != pid_.end(); it++)
	{
        if (it->second == nullptr) continue;
        libusb_release_interface(it->second, 0);
        libusb_close(it->second);
    }
    pid_.clear();

	libusb_hotplug_deregister_callback(nullptr, hot_plug_cb_handle_);
    hot_plug_cb_handle_ = 0;

	libusb_exit(nullptr);

	LOGINFO("libusb exit");
}

void usb_device::event_handle_task()
{
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 30*1000;
    while(!quit_)
	{
		libusb_handle_events_timeout(nullptr, &timeout);
	}
}

bool usb_device::is_open(int pid)
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = pid_.find(pid);
    if (it != pid_.end() && it->second != nullptr)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool usb_device::is_all_open()
{
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto it = pid_.begin(); it != pid_.end(); it++)
    {
        if (it->second == nullptr)
        {
            return false;
        }
    }

    return true;
}

bool usb_device::is_all_close()
{
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto it = pid_.begin(); it != pid_.end(); it++)
    {
        if (it->second != nullptr)
        {
            return false;
        }
    }

    return true;
}

int usb_device::write_data(int pid, unsigned char* data, int size, unsigned int timeout)
{
    return usb_transfer(pid, ENDPOINT_SEND_CMD, (unsigned char*)data, size, timeout);
}

int usb_device::write_data2(int pid, unsigned char* data, int size, unsigned int timeout)
{
    return usb_transfer(pid, ENDPOINT_SEND_DATA, (unsigned char*)data, size, timeout);
}

int usb_device::write_cmd(int pid, char* data, int size, unsigned int timeout)
{
    return usb_transfer(pid, ENDPOINT_SEND_CMD, (unsigned char*)data, size, timeout);
}

int usb_device::read_cmd(int pid, char* data, int size, unsigned int timeout)
{
    return usb_transfer_async(pid, ENDPOINT_RECV_CMD, (unsigned char*)data, size, timeout);
    //return usb_transfer(pid, ENDPOINT_RECV_CMD, (unsigned char*)data, size, timeout, false);
}

int usb_device::read_data(int pid, unsigned char* data, int size, unsigned int timeout)
{
    return usb_transfer(pid, ENDPOINT_RECV_DATA, (unsigned char*)data, size, timeout);
}

int usb_device::usb_transfer(int pid, int ep, unsigned char* data, int size, unsigned timeout)
{
    auto it = pid_.find(pid);
    if (it == pid_.end() || it->second == nullptr)
    {
        LOGERR("usb_transfer not find pid:%d", pid);
        return INS_ERR;
    }

    int actual_size = 0;
    int ret = libusb_bulk_transfer(it->second, ep, data, size, &actual_size, timeout);
    if (ret < 0)
    {
        //if (ret != LIBUSB_ERROR_TIMEOUT) //timeout太多不打印
        {
            LOGERR("pid:%d ep:0x%x bulk transfer size:%d actual_size:%d fail:%d %s", pid, ep, size, actual_size, ret, libusb_error_name(ret));
        }
        return ret;
    } 
    if (!actual_size) 
    {
        LOGERR("pid:%d ep:0x%x bulk transfer size 0", pid, ep);
        return LIBUSB_ERROR_NULL_PACKET;
    }

    if (actual_size == size)
    {
        return LIBUSB_SUCCESS;
    }
    else
    {
        LOGERR("pid:%d ep:0x%x bulk transfer size:%d != actual size:%d", pid, ep, size, actual_size);
        return LIBUSB_ERROR_NOT_COMPLETE;
    }
}

void usb_device::cancle_transfer(int32_t pid)
{
    auto it = dev_ctx_.find(pid);
    if (it == dev_ctx_.end()) return;

    std::unique_lock<std::mutex> lock(it->second->mtx);
    if (it->second->transfer) libusb_cancel_transfer(it->second->transfer);
    it->second->transfer = nullptr;
    it->second->stop = true;
}

static void LIBUSB_CALL async_transfer_cb(struct libusb_transfer *transfer)
{
    dev_contex* ctx = (dev_contex*)transfer->user_data;
    if (ctx != nullptr) ctx->cv.notify_all();
}
 
int32_t usb_device::usb_transfer_async(int pid, int ep, unsigned char* data, int size, unsigned timeout)
{
    auto it = pid_.find(pid);
    if (it == pid_.end() || it->second == nullptr) 
    {
        LOGERR("usb_transfer_async not find pid:%d", pid);
        return INS_ERR;
    }

    auto ctx = dev_ctx_.find(pid);
    if (ctx == dev_ctx_.end() || ctx->second == nullptr) 
    {
        LOGERR("not find pid:%d ctx", pid);
        return INS_ERR;
    }

    std::unique_lock<std::mutex> lock(ctx->second->mtx);

    if (ctx->second->stop) return LIBUSB_ERROR_CANCLE;

    auto transfer = libusb_alloc_transfer(0);
	if (!transfer) 
    {
        LOGERR("pid:%d ep:%x alloc transfer fail", pid, ep);
        return LIBUSB_ERROR_NO_MEM;
    }
        
	libusb_fill_bulk_transfer(transfer, it->second, ep, data, size, async_transfer_cb, dev_ctx_[pid].get(), 0);
	transfer->type = LIBUSB_TRANSFER_TYPE_BULK;

	auto ret = libusb_submit_transfer(transfer);
	if (ret < 0) 
    {
        LOGERR("pid:%d ep:%x submit transfer fail:%d %s", pid, ep, ret, libusb_error_name(ret));
		libusb_free_transfer(transfer);
		return ret;
	}
    
    ctx->second->transfer = transfer;

    ctx->second->cv.wait(lock);

    ctx->second->transfer = nullptr;

	switch (transfer->status) {
	case LIBUSB_TRANSFER_COMPLETED:
		ret = 0;
		break;
	case LIBUSB_TRANSFER_TIMED_OUT:
		ret = LIBUSB_ERROR_TIMEOUT;
		break;
	case LIBUSB_TRANSFER_STALL:
		ret = LIBUSB_ERROR_PIPE;
		break;
	case LIBUSB_TRANSFER_OVERFLOW:
		ret = LIBUSB_ERROR_OVERFLOW;
		break;
	case LIBUSB_TRANSFER_NO_DEVICE:
		ret = LIBUSB_ERROR_NO_DEVICE;
		break;
	case LIBUSB_TRANSFER_ERROR:
	case LIBUSB_TRANSFER_CANCELLED:
		ret = LIBUSB_ERROR_IO;
		break;
	default:
		ret = LIBUSB_ERROR_OTHER;
	}

    libusb_free_transfer(transfer);

    if (ret < 0)
    {
        if (ctx->second->stop)
        {
            return LIBUSB_ERROR_CANCLE; //正常停止不打印错误日志
        }
        else
        {
            LOGERR("pid:%d ep:0x%x bulk transfer size:%d fail:%d %s", pid, ep, size, ret, libusb_error_name(ret));
        }
    } 

    return ret;
}

// int usb_device::swich_usb_err(int usb_err, int ins_err)
// {
//     if (usb_err == LIBUSB_ERROR_NO_DEVICE)
//     {
//         return INS_ERR_CAMERA_OFFLINE;
//     }
//     else if (usb_err == LIBUSB_ERROR_TIMEOUT)
//     {
//         return INS_ERR_TIME_OUT;
//     }
//     else
//     {
//         return ins_err;
//     }
// }