// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.


#include "SlateViewerApp.h"

static FString GSavedCommandLine;

/**
 * WinMain, called when the application is started
 */
int main( int argc, char** argv )
{
	for (int32 Option = 1; Option < argc; Option++)
	{
		GSavedCommandLine += TEXT(" ");
		GSavedCommandLine += UTF8_TO_TCHAR(argv[Option]);	// note: technically it depends on locale
	}

	// do the slate viewer thing
	RunSlateViewer(*GSavedCommandLine);

	return 0;
}
