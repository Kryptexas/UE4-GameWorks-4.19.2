// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MacPlatformFile.h"
#include "Apple/ApplePlatformFile.h"

IPlatformFile& IPlatformFile::GetPlatformPhysical()
{
	static FApplePlatformFile MacPlatformSingleton;
	return MacPlatformSingleton;
}
