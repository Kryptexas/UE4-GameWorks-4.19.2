#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum
{
	EDSExtSuccess					=	0,
	EDSExtUnknownError				=	-1,
	EDSExtNoSDLWindow				=	-2,
	EDSExtCannotRetrieveWindowInfo	=	-3,
	EDSExtPointerOnAnotherScreen	=	-4,
	EDSExtInvalidX11ConnectionCache =	-5		// cached X11 display is not the same as one used by SDL window or NULL
};

/**
 * Caches necessary information about connection to X11
 *
 * @param SDLWindow SDL window known to be opened
 *
 * @return EDSExtSuccess if succeded, more specific error otherwise
 */
int DSEXT_CacheX11Info(SDL_Window *SDLWindow);

/**
 * Attempts to grab/ungrab pointer for the SDL window (caveat: seems to be broken for some (multi-monitor?) setups)
 *
 * @param window SDL window to grab cursor for
 * @param grabEnabled true if grab, false if let go
 *
 * @return EDSExtSuccess if succeded, more specific error otherwise
 */
int DSEXT_SetMouseGrab(SDL_Window *SDLWindow, SDL_bool bGrab);

/**
 * Attempts to get current (absolute) mouse position
 *
 * @param PtrX pointer to absolute x (cannot be NULL)
 * @param PtrY pointer to absolute y (cannot be NULL)
 *
 * @return EDSExtSuccess if succeeded (do not use x,y if returned anything else!)
 */
int DSEXT_GetAbsoluteMousePosition(int *PtrX, int *PtrY);

#ifdef __cplusplus
}
#endif

