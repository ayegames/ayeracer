/* --------------------------------------------------------------------
EXTREME TUXRACER

Copyright (C) 1999-2001 Jasmin F. Patry (Tuxracer)
Copyright (C) 2010 Extreme Tuxracer Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
---------------------------------------------------------------------*/

#ifndef BH_H
#define BH_H

// --------------------------------------------------------------------
//		compiler flags
// --------------------------------------------------------------------

#define HAVE_SDL
#define HAVE_SDL_MIXER
#define HAVE_SDL_IMAGE
#define HAVE_SDL_JOYSTICK
#define STDC_HEADERS
#define TIME_WITH_SYS_TIME
#define HAVE_GETCWD
#define HAVE_GETTIMEOFDAY
#define HAVE_STRDUP
#define HAVE_GL_GLEXT_H
#define HAVE_GL_GLX_H
#define HAVE_SYS_TIME_H
#define USE_STENCIL_BUFFER

// --------------------------------------------------------------------
//			includes
// --------------------------------------------------------------------

#include <cstdint>
#include <climits>
#include <cfloat>
#include <cstddef>
#include <string>
#include <sys/stat.h>

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cstring>

#include <GL/gl.h>
#include <GL/glu.h>
#include "SDL/SDL.h"
#include "SDL/SDL_joystick.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_mixer.h"

#ifdef _WIN32 // Windows platform
	#ifdef _MSC_VER // MSVC compiler
		#include <windows.h>
		#include "glext.h"
		#define OS_WIN32_MSC
		#pragma warning (disable:4244)
		#pragma warning (disable:4305)
		#define SEP "\\"
		#undef DrawText
	#else // Assume MinGW compiler
		#include <dirent.h>
		#include <GL/glext.h>
		#define OS_WIN32_MINGW
		#define SEP "/"
	#endif
#else // Assume Unix platform (Linux, Mac OS X, BSD, ...)
	#include <unistd.h>
	#include <sys/types.h>
	#include <pwd.h>
	#include <dirent.h>
	#include <sys/time.h>
	#include <GL/glx.h>
	#define SEP "/"
	#ifdef __APPLE__
		#define OS_MAC
	#elif defined(__linux__)
		#define OS_LINUX
	#endif
#endif

// --------------------------------------------------------------------
//			defines
// --------------------------------------------------------------------

#define PROG_NAME "ETR"
#define PACKAGE "etr"
#define VERSION "0.6"
#define WINDOW_TITLE "Extreme Tux Racer " VERSION

using namespace std;

#include "etr_types.h"
#include "mathlib.h"
#include "common.h"
#include "game_config.h"
#include "winsys.h"
#include "states.h"
#include "physics.h"

extern TGameData g_game;

#endif