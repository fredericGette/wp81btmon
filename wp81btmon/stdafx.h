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


#define VERSION "wp81-5.78" 
#define	EINVAL 22	/* Invalid argument */
