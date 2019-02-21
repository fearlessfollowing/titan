
#include "ins_signal.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "inslog.h"
#include "common.h"

void wait_signal_terminate()
{
	sigset_t sset;
	sigemptyset(&sset);
	
	sigaddset(&sset, SIGINT);
	sigaddset(&sset, SIGQUIT);
	sigaddset(&sset, SIGTERM);
	sigprocmask(SIG_BLOCK, &sset, NULL);
	
	int32_t sig;
	sigwait(&sset, &sig);

	LOGINFO("----recv sig:%d", sig);
}

int32_t singleton(const char* lock_file)
{
	int32_t fd = open(lock_file, O_WRONLY | O_CREAT, 0644);
	if (fd < 0) 
	{  
		LOGERR("file:%s open fail", lock_file);
		return INS_ERR;
	}  
	
	struct flock lock;  
	bzero(&lock, sizeof(lock));  
	
	if (fcntl(fd, F_GETLK, &lock) < 0) 
	{  
		LOGERR("F_GETLK fail");
		return INS_ERR;
	}  
	
	lock.l_type = F_WRLCK;  
	lock.l_whence = SEEK_SET;  
	if (fcntl(fd, F_SETLK, &lock) < 0) 
	{  
		LOGERR("F_SETLK fail");
		return INS_ERR;
	}

	char buf[32]; 
	int32_t len = snprintf(buf, 32, "%d\n", getpid());  
	write(fd, buf, len);

	return INS_OK;
}

void sig_handle(int32_t sig)
{
	LOGINFO("------recv sig:%d", sig);
}

void set_sigal_handle(int32_t sig)
{
	struct sigaction sig_action;
    sig_action.sa_handler = sig_handle;
    sigaction(sig, &sig_action, nullptr);
}