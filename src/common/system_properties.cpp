
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <errno.h>
#include <poll.h>

#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#include <sys/syscall.h>
//#include <_futex.h>

#include <system_properties.h>
//#include <sys_config.h>

#define PROP_MSG_SETPROP 1
#define PROP_NAME_MAX 32
#define PROP_VALUE_MAX 92
#define PA_SIZE 32768
#define PROP_AREA_MAGIC   0x504f5250
#define PROP_AREA_VERSION 0x45434f76
#define TOC_NAME_LEN(toc)       ((toc) >> 24)
#define TOC_TO_INFO(area, toc)  ((prop_info*) (((char*) area) + ((toc) & 0xFFFFFF)))
#define SERIAL_VALUE_LEN(serial) ((serial) >> 24)
#define SERIAL_DIRTY(serial) ((serial) & 1)

struct prop_area 
{
    unsigned volatile count;
    unsigned volatile serial;
    unsigned magic;
    unsigned version;
    unsigned reserved[4];
    unsigned toc[1];
};

struct prop_info 
{
    char name[PROP_NAME_MAX];
    unsigned volatile serial;
    char value[PROP_VALUE_MAX];
};

struct prop_msg 
{
    unsigned cmd;
    char name[PROP_NAME_MAX];
    char value[PROP_VALUE_MAX];
};

int send_prop_msg(prop_msg *msg);
const prop_info *system_property_find(std::string key);

static const char property_service_socket[] = "/dev/socket/property_service";

prop_area *__system_property_area__ = nullptr;

int32_t system_properties_init()
{
    prop_area *pa;
    int fd;
    unsigned sz;

    if (__system_property_area__ != nullptr) 
    {
        return 0;
    }
	
    fd = open("/run/.__properties__", O_RDONLY);	/* 以只读的方式打开,这样进程就没有直接修改属性的权限了 */
    if (fd < 0) 
    {
        printf("__properties__ open fail\n");
        return -1;
    }

	sz = PA_SIZE;	/* 属性区域的大小固定 */

    pa = (prop_area*)mmap(0, sz, PROT_READ, MAP_SHARED, fd, 0);
    if (pa == MAP_FAILED) 
    {
        printf("mmap fail\n");
        return -1;
    }

    if ((pa->magic != PROP_AREA_MAGIC) || (pa->version != PROP_AREA_VERSION)) 
    {
        munmap(pa, sz);
        return -1;
    }

	printf("prop_area.magic = 0x%x, prop_area.version = 0x%x\n", PROP_AREA_MAGIC, PROP_AREA_VERSION);
	
    __system_property_area__ = pa;

    return 0;
}

std::string property_get(std::string key)
{
    const prop_info *pi = system_property_find(key);

    if (pi != 0) 
    {
    	//printf("prop: %s exist\n", key.c_str());
        return pi->value;
    } 
    else 
    {
    	printf("prop: %s not exist\n", key.c_str());
        return "";
    }
}

int32_t property_set(std::string key, std::string value)
{
    if (key.length() >= PROP_NAME_MAX) return -1;
    if (value.length() >= PROP_VALUE_MAX) return -1;

    prop_msg msg;
    memset(&msg, 0, sizeof msg);
    msg.cmd = PROP_MSG_SETPROP;
    strncpy(msg.name, key.c_str(), key.length());
    strncpy(msg.value, value.c_str(), value.length());

	//printf("property set: key[%s], value [%s] \n", key.c_str(), value.c_str());

    auto ret = send_prop_msg(&msg);
    if (ret < 0) 
    {
        printf("send_prop_msg fail\n");
        return ret;
    }

    return 0;
}

const prop_info *system_property_find(std::string key)
{
    prop_area *pa = __system_property_area__;
    unsigned count = pa->count;
    unsigned *toc = pa->toc;
    prop_info *pi;

    while (count--) 
    {
        unsigned entry = *toc++;
        
        pi = TOC_TO_INFO(pa, entry);

        if (TOC_NAME_LEN(entry) != key.length()) 
        {
            continue;
        }

        if (memcmp(key.c_str(), pi->name, key.length())) 
        {
            continue;
        }

        return pi;
    }

    return 0;
}

int32_t send_prop_msg(prop_msg *msg)
{
    struct pollfd pollfds[1];
    struct sockaddr_un addr;
    socklen_t alen;
    size_t namelen;
    int s;
    int r;
    int ret = -1;

    s = socket(AF_LOCAL, SOCK_STREAM, 0);
    if(s < 0) return -1;

    memset(&addr, 0, sizeof(addr));
    namelen = strlen(property_service_socket);
    strncpy(addr.sun_path, property_service_socket, sizeof addr.sun_path);
    addr.sun_family = AF_LOCAL;
    alen = namelen + offsetof(struct sockaddr_un, sun_path) + 1;

    if (TEMP_FAILURE_RETRY(connect(s, (struct sockaddr *) &addr, alen)) < 0) 
    {
        close(s);
        return -1;
    }

    r = TEMP_FAILURE_RETRY(send(s, msg, sizeof(prop_msg), 0));

    if (r == sizeof(prop_msg)) 
    {
        pollfds[0].fd = s;
        pollfds[0].events = 0;
        r = TEMP_FAILURE_RETRY(poll(pollfds, 1, 250 /* ms */));
        if (r == 1 && (pollfds[0].revents & POLLHUP) != 0) 
        {
            ret = 0;
        } 
        else 
        {
            ret = 0;
        }
    }

    close(s);

    return ret;
}

// const prop_info* system_property_find_nth(unsigned n)
// {
//     prop_area *pa = __system_property_area__;

//     if(n >= pa->count) 
//     {
//         return nullptr;
//     } 
//     else 
//     {
//         return TOC_TO_INFO(pa, pa->toc[n]);
//     }
// }

// int system_property_read(const prop_info *pi, char *name, char *value)
// {
//     unsigned serial, len;
    
//     for(;;) {
//         serial = pi->serial;
//         while(SERIAL_DIRTY(serial)) {
//             __futex_wait((volatile void *)&pi->serial, serial, 0);
//             serial = pi->serial;
//         }
//         len = SERIAL_VALUE_LEN(serial);
//         memcpy(value, pi->value, len + 1);
//         if (serial == pi->serial) {
//             if(name != 0) {
//                 strcpy(name, pi->name);
//             }
//             return len;
//         }
//     }
// }

// int system_property_wait(const prop_info *pi)
// {
//     unsigned n;
//     if(pi == 0) {
//         prop_area *pa = __system_property_area__;
//         n = pa->serial;
//         do {
//             __futex_wait(&pa->serial, n, 0);
//         } while(n == pa->serial);
//     } else {
//         n = pi->serial;
//         do {
//             __futex_wait((volatile void *)&pi->serial, n, 0);
//         } while(n == pi->serial);
//     }
//     return 0;
// }
