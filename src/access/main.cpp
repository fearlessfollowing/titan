
#include <memory>
#include <unistd.h>
#include "access_msg_center.h"
#include "ins_signal.h"
#include "inslog.h"
#include "common.h"
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <system_properties.h>

int32_t main(int32_t argc, char *argv[])
{	
	int32_t option;
	bool b_deamon = false;
	while ((option = getopt(argc, argv, "d:n:")) != -1) 
	{
		switch (option) 
		{
			case 'd':
				b_deamon = true;
				break;
			case 'n':
				break;
			default: 
				break;
		}
	}
  
	if (b_deamon) daemon(1, 1);

	ins_log::init(INS_LOG_PATH, "log");

	if (singleton(PID_LOCK_FILE) != INS_OK)
	{
		LOGERR("singleton run fail"); 
		return -1;
	}

    LOGINFO("----------hello, I'm comming");

    // setpriority(PRIO_PROCESS, 0, -18);
    nice(1);

	//x11(HDMI) env
	if (setenv("DISPLAY", ":0", 1))
	{
		LOGINFO("set env DISPLAY fail");
	}
	if (setenv("XAUTHORITY", "/var/run/lightdm/root/:0", 1))
	{
		LOGINFO("set env XAUTHORITY fail");
	}

	//debug, commnet when release
	system("timedatectl set-ntp false");
	//system("rm -rf /home/nvidia/core.*");
	system("ulimit -c unlimited");
	system("echo /home/nvidia/core.%e > /proc/sys/kernel/core_pattern");
	//system("echo /home/nvidia/core.%e.%t.%p > /proc/sys/kernel/core_pattern");

    struct rlimit limite;
    limite.rlim_cur = limite.rlim_max = RLIM_INFINITY;
    if (0 != setrlimit(RLIMIT_CORE, &limite))
    {
    	LOGERR("setrlimit fail:%d %s", strerror(errno));
    }

	system_properties_init();

	set_sigal_handle(SIGPIPE);

	auto center = std::make_shared<access_msg_center>();

	if (center->setup()) return -1;

	wait_signal_terminate();

    center = nullptr;

    LOGINFO("----------bye bye, you'll miss me");

	return 0;
}