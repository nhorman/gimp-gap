#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <memory.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/ioctl.h>
#include <assert.h>
#ifdef __CYGWIN__
#include <sys/soundcard.h>
#else
#include <linux/soundcard.h>
#endif
#include <wavplay.h>
#include <wavfile.h>

#include <msg.c>
