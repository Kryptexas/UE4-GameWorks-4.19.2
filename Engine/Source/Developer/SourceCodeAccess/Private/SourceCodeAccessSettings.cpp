// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SourceCodeAccessPrivatePCH.h"
#include "SourceCodeAccessSettings.h"

USourceCodeAccessSettings::USourceCodeAccessSettings(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
#if PLATFORM_WINDOWS
	PreferredAccessor = TEXT("VisualStudioSourceCodeAccessor");
#elif PLATFORM_MAC
	PreferredAccessor = TEXT("XCodeSourceCodeAccessor");
#endif
}