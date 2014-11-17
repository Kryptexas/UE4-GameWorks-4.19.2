#include <SDL.h>
#include <SDL_syswm.h>

#include "ds_extensions.h"

typedef Bool (*PFNXGRABPOINTER)       (Display*, Window, Bool, unsigned int, int, int, Window, Cursor, Time);
typedef Bool (*PFNXUNGRABPOINTER)     (Display*, Time);
typedef Bool (*PFNXQUERYPOINTER)      (Display*, Window, Window*, Window*, int*, int*, int*, int*, unsigned int*);

void *libX11 = NULL;

PFNXGRABPOINTER       DSEXT_XGrabPointer       = NULL;
PFNXUNGRABPOINTER     DSEXT_XUngrabPointer     = NULL;
PFNXQUERYPOINTER      DSEXT_XQueryPointer      = NULL;

#define LOAD_SYMBOL( Name )                                \
	if (DSEXT_##Name == NULL)                              \
	{                                                      \
		if (libX11 == NULL)                                \
		{                                                  \
			libX11 = SDL_LoadObject("libX11.so.6");        \
		}                                                  \
		if(libX11 != NULL)                                 \
		{                                                  \
			DSEXT_##Name = SDL_LoadFunction(libX11, #Name);\
		}                                                  \
	}

#ifdef SDL_VIDEO_DRIVER_X11
/** Cached X11 display connection */
Display * GDisplay = NULL;
#endif // SDL_VIDEO_DRIVER_X11

int DSEXT_CacheX11Info(SDL_Window *SDLWindow)
{
	if (SDLWindow == NULL)
	{
		return EDSExtNoSDLWindow;
	}

	SDL_SysWMinfo Info;
	SDL_VERSION(&Info.version);
	if (SDL_GetWindowWMInfo(SDLWindow, &Info) == SDL_FALSE)
	{
		return EDSExtCannotRetrieveWindowInfo;
	}

	switch(Info.subsystem)
	{
#ifdef SDL_VIDEO_DRIVER_X11
		case SDL_SYSWM_X11:
			GDisplay = Info.info.x11.display;
			return EDSExtSuccess;
#endif // SDL_VIDEO_DRIVER_X11
		default:
			break;
	}

	return EDSExtUnknownError;
}

int DSEXT_SetMouseGrab(SDL_Window *SDLWindow, SDL_bool bGrab)
{
	if (SDLWindow == NULL)
	{
		SDLWindow = SDL_GetMouseFocus(); // Use MouseFocus if SDLWindow not specified
	}
	if (SDLWindow == NULL)
	{
		SDLWindow = SDL_GL_GetCurrentWindow(); // Fallback to current GL SDLWindow if MouseFocus is not set
	}

	if (SDLWindow == NULL)
	{
		return EDSExtNoSDLWindow;
	}

	SDL_SysWMinfo Info;
	SDL_VERSION(&Info.version);
	if (SDL_GetWindowWMInfo(SDLWindow, &Info) == SDL_FALSE)
	{
		return EDSExtCannotRetrieveWindowInfo;
	}

	switch(Info.subsystem)
	{
#ifdef SDL_VIDEO_DRIVER_X11
		case SDL_SYSWM_X11:
		{
			Display *display = Info.info.x11.display;
			if (display != GDisplay)
			{
				// we don't really need GDisplay here but it helps to check the possible errors
				return EDSExtInvalidX11ConnectionCache;
			}

			if (bGrab)
			{
				LOAD_SYMBOL(XGrabPointer)

				if (DSEXT_XGrabPointer)
				{
					return GrabSuccess == DSEXT_XGrabPointer( \
						display,
						DefaultRootWindow(display),
						False,
						ButtonPressMask | ButtonReleaseMask |
						ButtonMotionMask | PointerMotionMask |
						FocusChangeMask |
						EnterWindowMask | LeaveWindowMask,
						GrabModeAsync, GrabModeAsync,
						None, None, CurrentTime) ? EDSExtSuccess : EDSExtUnknownError;
				}
			}
			else
			{
				LOAD_SYMBOL(XUngrabPointer)

				if (DSEXT_XUngrabPointer)
				{
					DSEXT_XUngrabPointer(display, CurrentTime);
					return EDSExtSuccess;
				}
			}
		}
		break;
#endif // SDL_VIDEO_DRIVER_X11

	default:
		break;
	}

	return EDSExtUnknownError;
}

int DSEXT_GetAbsoluteMousePosition(int *PtrX, int *PtrY)
{
	if (PtrX == NULL || PtrY == NULL)
	{
		return EDSExtUnknownError;
	}

#ifdef SDL_VIDEO_DRIVER_X11
	if (GDisplay == NULL)
	{
		return EDSExtInvalidX11ConnectionCache;
	}

	LOAD_SYMBOL(XQueryPointer)

	Window ReturnedRoot, ReturnedChild;
	int WinX, WinY;
	unsigned int Mask;

	return True == DSEXT_XQueryPointer(
		GDisplay, DefaultRootWindow(GDisplay), &ReturnedRoot, &ReturnedChild, PtrX, PtrY,
		&WinX, &WinY, &Mask) ? EDSExtSuccess : EDSExtPointerOnAnotherScreen;

#else
	return EDSExtUnknownError;
#endif // SDL_VIDEO_DRIVER_X11

#if 0
	SDL_Window* SDLWindow = SDL_GetMouseFocus();
	if (SDLWindow == NULL)
	{
		return EDSExtNoSDLWindow;
	}
	SDL_SysWMinfo Info;
	SDL_VERSION(&Info.version);
	if (SDL_GetWindowWMInfo(SDLWindow, &Info) == SDL_FALSE)
	{
		return EDSExtCannotRetrieveWindowInfo;
	}

	switch(Info.subsystem)
	{
#ifdef SDL_VIDEO_DRIVER_X11
	case SDL_SYSWM_X11:
		{
			Display *XDisplay = Info.info.x11.display;
			Window XWindow = Info.info.x11.window;
			Window Root, Child;
			int WinX, WinY;
			unsigned int Mask;

			LOAD_SYMBOL(XQueryPointer)

			return True == DSEXT_XQueryPointer(
				XDisplay, XWindow, &Root, &Child, PtrX, PtrY,
				&WinX, &WinY, &Mask) ? EDSExtSuccess : EDSExtPointerOnAnotherScreen;
		}
#endif // SDL_VIDEO_DRIVER_X11
	default:
		{
			int WndX, WndY, RelX, RelY;
			SDL_GetWindowPosition(SDLWindow, &WndX, &WndY);
			SDL_GetMouseState(&RelX, &RelY);
			*PtrX = WndX + RelX;
			*PtrY = WndY + RelY;
			return EDSExtSuccess;
		}
	}

	// unreachable
	return EDSExtUnknownError;
#endif // 0
}
