// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	GenericPlatformNamedPipe.cpp: Implements the FGenericPlatformNamedPipe class
==============================================================================================*/

#include "CorePrivate.h"


#if PLATFORM_SUPPORTS_NAMED_PIPES

FGenericPlatformNamedPipe::FGenericPlatformNamedPipe()
{
	NamePtr = new FString();
}

FGenericPlatformNamedPipe::~FGenericPlatformNamedPipe()
{
	delete NamePtr;
	NamePtr = NULL;
}

#endif	// PLATFORM_SUPPORTS_NAMED_PIPES
