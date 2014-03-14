// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	HideWindowsPLatformTypes.h: Defines for hiding windows type names.
=============================================================================*/

#ifdef WINDOWS_PLATFORM_TYPES_GUARD
	#undef WINDOWS_PLATFORM_TYPES_GUARD
#else
	#error Mismatched HideWindowsPLatformTypes.h detected.
#endif

#undef INT
#undef UINT
#undef DWORD
#undef FLOAT

#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif