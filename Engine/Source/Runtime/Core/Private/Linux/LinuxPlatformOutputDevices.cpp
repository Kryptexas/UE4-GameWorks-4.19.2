// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LinuxOutputDevices.cpp: Linux implementations of OutputDevices functions
=============================================================================*/


#include "CorePrivate.h"
#include "LinuxPlatformOutputDevices.h"
#include "FeedbackContextAnsi.h"
#include <syslog.h>

class FFeedbackContextLinux : public FFeedbackContextAnsi
{
public:

	void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category)
	{
		if (Verbosity == ELogVerbosity::Error || Verbosity == ELogVerbosity::Warning)
		{
			FString Format = FOutputDevice::FormatLogLine(Verbosity, Category, V);

			int BasePriority = (Verbosity == ELogVerbosity::Error) ? LOG_ERR : LOG_WARNING;
			syslog(BasePriority | LOG_USER, "%s", StringCast< char >(*Format).Get());
		}

		FFeedbackContextAnsi::Serialize(V, Verbosity, Category);
	}
};

class FFeedbackContext*				FLinuxOutputDevices::GetWarn()
{
	static FFeedbackContextLinux Singleton;
	return &Singleton;
}
