#ifndef __SPLASHZ_H__
#define __SPLASHZ_H__

#if !defined(NO_SPLASH)

#ifdef SPLASH_QNX
#include "splashz_qnx.h"
#else
#ifdef SPLASH_LINUX
#include "splashz_linux.h"
#else
#error You need to specify a splash header.
#endif 
#endif

#endif

#endif
