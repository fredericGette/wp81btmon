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
#include <winsock2.h>
#include "shared/tty.h"
#include "mainloop.h"
#include "packet.h"
#include "display.h"
#include "lmp.h"
#include "keys.h"
#include "analyze.h"
#include "ellisys.h"
#include "control.h"

extern "C" {
	WINBASEAPI BOOL	WINAPI SetConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine, BOOL Add);
	char *getenv(const char *varname);
}


#define VERSION "wp81-5.78" 