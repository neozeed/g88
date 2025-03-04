/*
 * getpagesize for UTek system V /88k only
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/getpagesize.h,v 1.4 90/03/15 09:04:52 andrew Exp $
 */ 

#ifndef _SC_MEMCTL_UNIT
#include <sys/unistd.h>
#endif

#define getpagesize() sysconf(_SC_MEMCTL_UNIT)
