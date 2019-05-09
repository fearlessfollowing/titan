/*
 * Copyright (c) 2016, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * <b>NVIDIA Multimedia API: V4L2 Element Plane</b>
 *
 * @b Description: This file declares a helper class for operations
 * performed on a V4L2 Element plane.
 */

/**
 * @defgroup l4t_mm_nvv4lelementplane_group NvV4l2ElementPlane Class
 * @ingroup l4t_mm_nvelement_group
 *
 * Helper class for operations performed on a V4L2 element plane.
 * This includes getting/setting plane formats, plane buffers,
 * and cropping.
 * @{
 */

#ifndef __NV_V4L2_ELELMENT_PLANE_H__
#define __NV_V4L2_ELELMENT_PLANE_H__

#include <pthread.h>
#include "NvElement.h"
#include "NvLogging.h"
#include "NvBuffer.h"

/**
 * Prints a plane-specific message of level LOG_LEVEL_DEBUG.
 * Must not be used by applications.
 */
#define PLANE_DEBUG_MSG(str) COMP_DEBUG_MSG(plane_name << ":" << str);
/**
 * Prints a plane-specific message of level LOG_LEVEL_INFO.
 * Must not be used by applications.
 */
#define PLANE_INFO_MSG(str) COMP_INFO_MSG(plane_name << ":" << str);
/**
 * Prints a plane-specific message of level LOG_LEVEL_WARN.
 * Must not be used by applications.
 */
#define PLANE_WARN_MSG(str) COMP_WARN_MSG(plane_name << ":" << str);
/**
 * Prints a plane-specific message of level LOG_LEVEL_ERROR.
 * Must not be used by applications.
 */
#define PLANE_ERROR_MSG(str) COMP_ERROR_MSG(plane_name << ":" << str);
/**
 * Prints a plane-specific system error message of level LOG_LEVEL_ERROR.
 * Must not be used by applications.
 */
#define PLANE_SYS_ERROR_MSG(str) COMP_SYS_ERROR_MSG(plane_name << ":" << str);


/**
 * @brief Defines a helper class for operations performed on a V4L2 Element plane.
 *
 * This derived class is modeled on the planes of a V4L2 Element. It provides
 * convenient wrapper methods around V4L2 IOCTLs associated with plane
 * operations such as <code>VIDIOC_G_FMT/VIDIOC_S_FMT</code>, \c VIDIOC_REQBUFS,
 * <code>VIDIOC_STREAMON/VIDIOC_STREAMOFF</code>, etc.
 *
 * The plane buffer type can be either \c V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE (for
 * the output plane) or \c V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE (for the capture
 * plane).
 *
 * The plane has an array of NvBuffer object pointers that is allocated and
 * initialized during reqbuf call. These \c NvBuffer objects are similar to the
 * \c v4l2_buffer structures that are queued/dequeued.
 *
 * This class provides another feature useful for multi-threading. On calling
 * #startDQThread, it internally spawns a thread that runs infinitely until
 * signaled to stop. This thread keeps trying to dequeue a buffer from the
 * plane and calls a #dqThreadCallback method specified by the user on
 * successful dequeue.
 *
 */
class NvV4l2ElementPlane {

public:
    
    /**
     * 获取Plane的格式
     * 
     * 内部调用VIDIOC_G_FMT
     * 
     * @param[in,out] format A reference to the \c v4l2_format structure to be filled.
     * @return 0 for success, -1 otherwise.
     */
    int getFormat(struct v4l2_format & format);
    

    /**
     * 获取Plane的格式
     * 
     * 内部调用VIDIOC_S_FMT
     *
     * @param[in] format A reference to the \c v4l2_format structure to be set on the plane.
     * @return 0 for success, -1 otherwise.
     */
    int setFormat(struct v4l2_format & format);

    
    /**
     * Maps the NvMMBuffer to NvBuffer for V4L2_MEMORY_DMABUF.
     *
     * @param[in] v4l2_buf Address of the NvBuffer to which the NvMMBuffer is mapped.
     * @param[in] dmabuff_fd Index to the field that holds NvMMBuffer attributes.
     * @return 0 for success, -1 otherwise.
     */
    int mapOutputBuffers(struct v4l2_buffer &v4l2_buf, int dmabuff_fd);

    /**
     * Unmaps the NvMMBuffer for V4L2_MEMORY_DMABUF.
     *
     * @param[in] index for the current buffer index.
     * @param[in] dmabuff_fd Index to the field that holds NvMMBuffer attributes.
     * @return 0 for success, -1 otherwise.
     */

    int unmapOutputBuffers(int index, int dmabuff_fd);

    /**
     * 获取Plane的分辨率
     * 
     * 内部调用VIDIOC_G_CROP
     * 
     * @param[in] crop A reference to the \c v4l2_crop structure to be filled.
     * @return 0 for success, -1 otherwise.
     */
    int getCrop(struct v4l2_crop & crop);


    /**
     * Sets the selection rectangle for the plane.
     *
     * Calls @b VIDIOC_S_SELECTION IOCTL internally.
     *
     * @param[in] target Specifies the rectangle selection type.
     * @param[in] flags Specifies the flags to control selection adjustments.
     * @param[in] rect A reference to the selection rectangle.
     * @return 0 for success, -1 otherwise.
     */
    int setSelection(uint32_t target, uint32_t flags, struct v4l2_rect & rect);


    /**
     * 在Plane中请求若干个Buffers
     *
     * 内部调用VIDIOC_REQBUFS IOCTL, 创建指定长度的NvBuffer数组
     *
     * @param[in] mem_type Specifies the type of V4L2 memory to be requested.
     * @param[in] num Specifies the number of buffers to request on the plane.
     * @return 0 for success, -1 otherwise.
     */
    int reqbufs(enum v4l2_memory mem_type, uint32_t num);
	

    /**
     * 查询指定index的Buffers的状态
     *
     * @warning This method works only for \c V4L2_MEMORY_MMAP memory.
     *
     * Calls \c VIDIOC_QUERYBUF IOCTL internally. Populates the \a length and \a
     * mem_offset members of all the NvBuffer::NvBufferPlane members of the
     * \c %NvBuffer object at index \a buf_index.
     *
     * @param[in] buf_index Specifies the index of the buffer to query.
     * @return 0 for success, -1 otherwise.
     */
    int queryBuffer(uint32_t buf_index);
	

    /**
     * Exports the buffer as DMABUF FD.
     *
     * @warning This method works only for \c V4L2_MEMORY_MMAP memory.
     *
     * Calls \c VIDIOC_EXPBUF IOCTL internally. Populates the \a fd member of all
     * the \c NvBuffer::NvBufferPlane members of \c NvBuffer object at index \a buf_in.
     *
     * @param[in] buf_index Specifies the index of the buffer to export.
     * @return 0 for success, -1 otherwise.
     */
    int exportBuffer(uint32_t buf_index);


    /**
     * 启动/停止该Plane上的流
     *
     * Calls \c VIDIOC_STREAMON/VIDIOC_STREAMOFF IOCTLs internally.
     *
     * @param[in] status Must be TRUE to start the stream, FALSE to stop the stream.
     * @return 0 for success, -1 otherwise.
     */
    int setStreamStatus(bool status);


    /**
     * 获取该Plane上的流状态
     *
     * @returns true if the plane is streaming, false otherwise.
     */
    bool getStreamStatus();


    /**
     * 设置流参数
     *
     * 内部调用VIDIOC_S_PARM
     *
     * @param[in] parm A reference to the \c v4l2_streamparm structure to be set on the
     *                 plane.
     * @return 0 for success, -1 otherwise.
     */
    int setStreamParms(struct v4l2_streamparm & parm);


    /**
     * Helper method that encapsulates all the method calls required to
     * set up the plane for streaming.
     *
     * Calls reqbuf internally. Then, for each of the buffers, calls #queryBuffer,
     * #exportBuffer and maps the buffer/allocates the buffer memory depending
     * on the memory type.
     *
     * @sa deinitPlane
     *
     * @param[in] mem_type V4L2 Memory to use on the buffer.
     * @param[in] num_buffers Number of buffer to request on the plane.
     * @param[in] map boolean value indicating if the buffers should be mapped to
                      memory (Only for V4L2_MEMORY_MMAP).
     * @param[in] allocate boolean valued indicating whether the buffers should be
                           allocated memory (Only for V4L2_MEMORY_USERPTR).
     * @return 0 for success, -1 otherwise.
     */
    int setupPlane(enum v4l2_memory mem_type, uint32_t num_buffers, bool map, bool allocate);


	/**
     * Helper method that encapsulates all the method calls required to
     * deinitialize the plane for streaming.
     *
     * For each of the buffers, unmaps/deallocates memory depending on the
     * memory type. Then, calls reqbuf with count zero.
     *
     * @sa setupPlane
     */
    void deinitPlane();


    /**
     * Gets the streaming/buffer type of this plane.
     *
     * @returns Type of the buffer belonging to enum \c v4l2_buf_type.
     */
    inline enum v4l2_buf_type getBufType()
    {
        return buf_type;
    }


    /**
     * Gets the \c NvBuffer object at index n.
     *
     * @returns \c %NvBuffer object at index n, NULL if n >= number of buffers.
     */
    NvBuffer *getNthBuffer(uint32_t n);


    /**
     * Dequeues a buffer from the plane.
     *
     * This is a blocking call. This call returns when a buffer is successfully
     * dequeued or timeout is reached. If \a buffer is not NULL, returns the
     * \c NvBuffer object at the index returned by the \c VIDIOC_DQBUF IOCTL. If this
     * plane shares a buffer with other elements and \a shared_buffer is not
     * NULL, returns the shared \c %NvBuffer object in \a shared_buffer.
     *
     * @param[in] v4l2_buf A reference to the \c v4l2_buffer structure to use for dequeueing.
     * @param[out] buffer Returns a pointer to a pointer to the \c %NvBuffer object associated with the dequeued
     *                    buffer. Can be NULL.
     * @param[out] shared_buffer Returns a pointer to a pointer to the shared \c %NvBuffer object if the queued
                   buffer is shared with other elements. Can be NULL.
     * @param[in] num_retries Number of times to try dequeuing a buffer before
     *                        a failure is returned. In case of non-blocking
     *                        mode, this is equivalent to the number of
     *                        milliseconds to try to dequeue a buffer.
     * @return 0 for success, -1 otherwise.
     */
    int dqBuffer(struct v4l2_buffer &v4l2_buf, 
                    NvBuffer ** buffer,
                    NvBuffer ** shared_buffer, 
                    uint32_t num_retries);


	/**
     * Queues a buffer on the plane.
     *
     * This method calls \c VIDIOC_QBUF internally. If this plane is sharing a
     * buffer with other elements, the application can pass the pointer to the
     * shared NvBuffer object in \a shared_buffer.
     *
     * @param[in] v4l2_buf A reference to the \c v4l2_buffer structure to use for queueing.
     * @param[in] shared_buffer A pointer to the shared \c %NvBuffer object.
     * @return 0 for success, -1 otherwise.
     */
    int qBuffer(struct v4l2_buffer &v4l2_buf, NvBuffer* shared_buffer);


    /**
     * Gets the number of buffers allocated/requested on the plane.
     *
     * @returns Number of buffers.
     */
    inline uint32_t getNumBuffers() {
        return num_buffers;
    }


    /**
     * Gets the number of planes buffers on this plane for the currently
     * set format.
     *
     * @returns Number of planes.
     */
    inline uint32_t getNumPlanes() {
        return n_planes;
    }


    /**
     * Sets the format of the planes of the buffer that is used with this
     * plane.
     *
     * The buffer plane format must be set before calling reqbuf since
     * these are needed by the NvBuffer constructor.
     *
     * @sa reqbufs
     *
     * @param[in] n_planes Number of planes in the buffer.
     * @param[in] planefmts A pointer to the array of \c NvBufferPlaneFormat that describes the
     *            format of each of the plane. The array length must be at
     *            least @a n_planes.
     */
    void setBufferPlaneFormat(int n_planes, NvBuffer::NvBufferPlaneFormat * planefmts);


    /**
     * 该Plane中当前队列中buffers数目
     *
     * @returns Number of buffers currently queued on the plane.
     */
    inline uint32_t getNumQueuedBuffers() {
        return num_queued_buffers;
    }


    /**
     * 该Plane中总共出队列的Buffers次数
     *
     * @returns Total number of buffers dequeued from the plane.
     */
    inline uint32_t getTotalDequeuedBuffers() {
        return total_dequeued_buffers;
    }


    /**
     * 该Plane中总共入队列的buffers数目
     *
     * @returns Total number of buffers queued on the plane.
     */
    inline uint32_t getTotalQueuedBuffers() {
        return total_queued_buffers;
    }


    /**
     * 等待直到所有的buffers入该Plane的队列
     *
     * 该方法为阻塞调用, 直到所有的buffers入队列或超时

     * @param[in] max_wait_ms - 最大的等待时间(单位为ms)
     * @return 0 for success, -1 otherwise.
     */
    int waitAllBuffersQueued(uint32_t max_wait_ms);

	
    /**
     * 等待Plane中所有的buffers出队列
     *
     * 阻塞调用,直到所有的buffers出队列或超时

     * @param[in] max_wait_ms - 等待的最长时间(单位为ms)
     * @return 0 for success, -1 otherwise.
     */
    int waitAllBuffersDequeued(uint32_t max_wait_ms);


    /**
     * 回调函数: 当DQ线程成功的从Plane中DQ一个buffer时被调用,应用程序必须实现该回调并通过
     * #setDQThreadCallback设置
     *
     * Setting the stream to off automatically stops this thread.
     *
     * @sa setDQThreadCallback, #startDQThread
     *
     * @param v4l2_buf A pointer to the \c v4l2_buffer structure that is used for dequeueing.
     * @param buffer A pointer to the NvBuffer object at the \c index contained in \c v4l2_buf.
     * @param shared_buffer A pointer to the NvBuffer object if the plane shares a buffer with other elements,
     *         else NULL.
     * @param data A pointer to application specific data that is set with
     *             #startDQThread.
     * @returns If the application implementing this call returns FALSE,
     *          the DQThread is stopped; else, the DQ Thread continues running.
     */
    typedef bool(*dqThreadCallback) (struct v4l2_buffer * v4l2_buf,
                                        NvBuffer * buffer, 
                                        NvBuffer * shared_buffer,
                                        void *data);


    /**
     * 设置DQ线程的回调函数
     *
     * 该回调函数在DQ线程成功DQ一个buffer时被调用
     * 
     * @param[in] callback Method to be called upon succesful dequeue.
     * @returns TRUE for success, FALSE for failure.
     */
    bool setDQThreadCallback(dqThreadCallback callback);

	
    /**
     * 启动DQ线程
     *
     * 该方法将在内部启动一个线程,一旦成功成Plane中DQ一个buffer时会调用回调
     *
     * 设置流OFF时会自动的停止DQ线程
     *
     * @sa stopDQThread, waitForDQThread
     *
     * @param[in] data A pointer to the application data. This is provided as an
     *                 argument in the \c dqThreadCallback method.
     * @return 0 for success, -1 otherwise.
     */
    int startDQThread(void *data);

	
    /**
     * 强制停止DQ线程
     *
     * 设备以阻塞模式打开时将无效
     *
     * @sa startDQThread, waitForDQThread
     *
     * @return 0 for success, -1 otherwise.
     */
    int stopDQThread();

	
    /**
     * 等待DQ线程停止
     *
     * 等待DQ线程停止或直到超时
     *
     * @sa startDQThread, stopDQThread
     *
     * @param[in] max_wait_ms - 最大的等待时间(单位为ms)
     * @return 0 for success, -1 otherwise.
     */
    int waitForDQThread(uint32_t max_wait_ms);


    pthread_mutex_t plane_lock; 	/**< Mutex lock used along with #plane_cond. */
    pthread_cond_t  plane_cond;     /**< Plane condition that application can wait on
                                    		to receive notifications from #qBuffer/#dqBuffer. */

private:
    int &fd;     					/**< A reference to the FD of the V4l2 Element the plane is associated with. */

    const char *plane_name; 		/**< 该NvV4l2ElementPlane的名称 */
    enum v4l2_buf_type buf_type; 	/**< Speciifes the type of the stream. */

    bool blocking;  				/**< Specifies whether the V4l2 element is opened with blocking mode. */

    uint32_t num_buffers;   		/**< 含有所少个Buffer(VIDIOC_REQBUFS_IOCTL的返回值) */
    NvBuffer **buffers;     		/**< NvBuffer指针数组 */

    uint8_t n_planes;       		/**< 一个NvBuffer中含有多少个Plane */

    NvBuffer::NvBufferPlaneFormat planefmts[MAX_PLANES];
                            		/**< Format of the buffer planes. This must be initialized before calling #reqbufs since this is required by the \c %NvBuffer constructor. */

    enum v4l2_memory    memory_type; 	/**< Buffer使用的V4L2内存类型 */

    uint32_t            num_queued_buffers;  	/**< Holds the number of buffers currently queued on the plane. */
    uint32_t            total_queued_buffers;   /**< Holds the total number of buffers queued on the plane. */
    uint32_t            total_dequeued_buffers; /**< Holds the total number of buffers dequeued from the plane. */

    bool                streamon;  			/**< 该Plane的流是否开启标志 */
    bool                dqthread_running;  	/**< DQ线程是否正在运行的标志 */
    bool                stop_dqthread; 		/**< 通知DQ线程停止的信号值 */

    pthread_t           dq_thread; 			/**< DQ线程的线程ID */
    dqThreadCallback    callback; 		    /**< DQ线程的回调函数 */
    void*               dqThread_data;      /**< DQ线程回调函数的参数(由应用程序传递) */

    /**
     * The DQ thread method.
     *
     * This method runs indefinitely until it is signaled to stop
     * by #stopDQThread or the #dqThreadCallback method returns FALSE.
     * It keeps on trying to dequeue a buffer from the plane and calls the
     * #dqThreadCallback method on successful dequeue.
     *
     * @param[in] v4l2_element_plane A pointer to the NvV4l2ElementPlane object
     *                 for which the thread started.
     */
    static void *dqThread(void *v4l2_element_plane);


    NvElementProfiler &v4l2elem_profiler; /**< A reference to the profiler belonging
                                            to the plane's parent element. */

    /**
     * Indicates whether the plane encountered an error during its operation.
     *
     * @return 0 if no error was encountered, a non-zero value if an
     *            error was encountered.
     */
    inline int isInError()
    {
        return is_in_error;
    }


    /**
     * Creates a new V4l2Element plane.
     *
     *
     * @param[in] buf_type Type of the stream.
     * @param[in] device_name A pointer to the name of the element the plane belongs to.
     * @param[in] fd A reference to the FD of the device opened using v4l2_open.
     * @param[in] blocking A flag that indicates whether the device has been opened with blocking mode.
     * @param[in] profiler The profiler.
     */
    NvV4l2ElementPlane(enum v4l2_buf_type buf_type, const char *device_name, int &fd, bool blocking, NvElementProfiler &profiler);

    /**
     * Disallows copy constructor.
     */
    NvV4l2ElementPlane(const NvV4l2ElementPlane& that);
	
    /**
     * Disallows assignment.
     */
    void operator=(NvV4l2ElementPlane const&);

    /**
     * NvV4l2ElementPlane destructor.
     *
     * Calls #deinitPlane internally.
     */
     ~NvV4l2ElementPlane();

    int             is_in_error;        /**< Indicates if an error was encountered during the operation of the element. */
    const char *    comp_name;  /**< Specifies the name of the component, for debugging. */

    friend class    NvV4l2Element;
};

#endif
