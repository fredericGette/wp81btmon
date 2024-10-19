// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

// TODO: reference additional headers your program requires here

#include <windows.h>
#include <stdlib.h>
#include <stdint.h>
#include <atomic>
#include <winsock2.h>
#include "tty.h"
#include "mainloop.h"
#include "packet.h"
#include "display.h"
#include "lmp.h"
#include "keys.h"
#include "analyze.h"
#include "ellisys.h"
#include "control.h"
#include "btsnoop.h"
#include "hci.h"
#include "portable_endian.h"
#include "Win32Api.h"

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

#define FILE_ANY_ACCESS                 0
#define FILE_SPECIAL_ACCESS    (FILE_ANY_ACCESS)
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe

//
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#define VERSION "wp81-5.78" 
#define ENOMEM 12   /* Not enough core */
#define	EINVAL 22	/* Invalid argument */
