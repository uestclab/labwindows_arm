#ifndef	__COMMON_H__

#define	__COMMON_H__

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
#include <setjmp.h>
#include <signal.h>
#include <paths.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>
#include <sys/wait.h>

//#include "memwatch.h"

#define	USE_MQTT

#define	LONG_COMMON_OPT\
	{ "help",	 0, NULL, 'h' },\
	{ "version", 0, NULL, 'v' },\
	{ "file",    1, NULL, 'f' },\
	{ "print",   1, NULL, 'p' },\
	{ "spec",    0, NULL, 's' },\
	{ "debug",   1, NULL, 'd' },\
	{ "logc",    1, NULL, 'l' }

#define	SHORT_COMMON_OPT	"hvf:l:p:s::d:"


#define	COMMON_HELP_INFO\
	" -h --help\t\thelp info\n"\
	" -v --version\t\tversion info\n"\
	" -f --file\t\tjson(configure) file\n"\
	" -p --print\t\tprint read value\n"\
	" -s --spec\t\tspecified to used the cmdline parameter\n"\
	" -d --debug\t\tdebug info\n"\
	" -l --logc\t\tlog(configure) file\n"


#define	END_OPT\
	{ NULL, 0, NULL, 0}

#define	VER_MAJOR	2
#define	VER_MINOR	0
#define	VER_REV		0

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

struct err_jsonstr_s{
	char *str;
	int free;
};

static inline void* xmalloc(size_t size)
{
	void *ptr = malloc(size);
	return ptr;
}

static inline void* xzalloc(size_t size)
{
	void *ptr = malloc(size);
	memset(ptr, 0, size);
	return ptr;
}

static inline void* xrealloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);
	return ptr;
}

#define	xfree(ptr) do{ \
	if(ptr){ \
		free(ptr); \
		ptr = NULL; \
	} \
}while(0)


static inline char *get_prog_name(char *argv)
{
	int len = strlen(argv);
	int i;
	char *tmp = argv;
	
	for(i=len; i >=0; i--)
	{
		if(tmp[i] == '/'){
			i++;
			break;
		}
	}
	
	if(-1 == i){
		i = 0;
	}

	return argv + i;
}
	
static inline void print_msg(char *buf, int buf_len)
{
	//return ;
	int i;
	for(i=0; i<buf_len; i++){
		printf("%c",buf[i]);
	}
	printf("\n");
}


typedef	struct timeval	time_diff_s;

static inline void get_usec(time_diff_s *time)
{
	//struct timeval time;

	gettimeofday(time, NULL);

	//return time;
}

static inline int time_expire(time_diff_s start, int expire_us)
{
	struct timeval now;
	int sec;
	int diff_usec;
	
	gettimeofday(&now, NULL);

	sec = now.tv_sec - start.tv_sec;
	
	diff_usec = sec * 1000000 + now.tv_usec - start.tv_usec;

	return ((diff_usec - expire_us)>=0)?1:0;
}

	
#endif

