// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IDesktopPlatform.h"

class FDesktopPlatformBase : public IDesktopPlatform
{
public:
	// IDesktopPlatform Implementation
	virtual FString GetCurrentEngineIdentifier() OVERRIDE;
	virtual bool GetEngineRootDirFromIdentifier(const FString &Identifier, FString &OutRootDir) OVERRIDE;
	virtual bool GetEngineIdentifierFromRootDir(const FString &RootDir, FString &OutIdentifier) OVERRIDE;
};
