// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Core.h"

IPlatformFile& IPlatformFile::GetPlatformPhysical()
{
	static FApplePlatformFile MacPlatformSingleton;
	return MacPlatformSingleton;
}
