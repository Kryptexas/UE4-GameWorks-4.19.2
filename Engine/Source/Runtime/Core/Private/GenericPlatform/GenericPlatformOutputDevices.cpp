// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GenericPlatformOutputDevices.cpp: Generic implementations of OutputDevices platform functions
=============================================================================*/

#include "CorePrivate.h"
#include "FeedbackContextAnsi.h"

FString								FGenericPlatformOutputDevices::GetAbsoluteLogFilename()
{
	static TCHAR		Filename[1024] = { 0 };

	if (!Filename[0])
	{
		// The Editor requires a fully qualified directory to not end up putting the log in various directories.
		FCString::Strcpy(Filename, *FPaths::GameLogDir());

		if(	!FParse::Value(FCommandLine::Get(), TEXT("LOG="), Filename+FCString::Strlen(Filename), ARRAY_COUNT(Filename)-FCString::Strlen(Filename) )
			&&	!FParse::Value(FCommandLine::Get(), TEXT("ABSLOG="), Filename, ARRAY_COUNT(Filename) ) )
		{
			if (FCString::Strlen(GGameName) != 0)
			{
				FCString::Strcat( Filename, GGameName );
			}
			else
			{
				FCString::Strcat( Filename, TEXT("UE4") );
			}
			FCString::Strcat( Filename, TEXT(".log") );
		}
	}

	return Filename;
}

class FOutputDevice*				FGenericPlatformOutputDevices::GetLog()
{
	static FOutputDeviceFile Singleton;
	return &Singleton;
}
class FOutputDeviceError*			FGenericPlatformOutputDevices::GetError()
{
	static FOutputDeviceAnsiError Singleton;
	return &Singleton;
}
class FFeedbackContext*				FGenericPlatformOutputDevices::GetWarn()
{
	static FFeedbackContextAnsi Singleton;
	return &Singleton;
}
