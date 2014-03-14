// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	WindowsPlatformString.h: Windows platform string classes, mostly implemented with ANSI C++
==============================================================================================*/

#pragma once

#include "MicrosoftPlatformString.h"

/**
* Windows string implementation
**/
struct FWindowsPlatformString : public FMicrosoftPlatformString
{
	// These should be replaced with equivalent Convert and ConvertedLength functions when we properly support variable-length encodings.
/*	static void WideCharToMultiByte(const wchar_t *Source, uint32 LengthWM1, ANSICHAR *Dest, uint32 LengthA)
	{
		::WideCharToMultiByte(CP_ACP,0,Source,LengthWM1+1,Dest,LengthA,NULL,NULL);
	}
	static void MultiByteToWideChar(const ANSICHAR *Source, TCHAR *Dest, uint32 LengthM1)
	{
		::MultiByteToWideChar(CP_ACP,0,Source,LengthM1+1,Dest,LengthM1+1);
	}*/

	static const ANSICHAR* GetEncodingName()
	{
		return "UTF-16LE";
	}
};

typedef FWindowsPlatformString FPlatformString;
