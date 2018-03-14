// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
#include "UObject/NameTypes.h"
#include "Misc/Guid.h"
#include "Serialization/CustomVersion.h"

class CORE_API FDevVersionRegistration :  public FCustomVersionRegistration
{
public:
	FDevVersionRegistration(FGuid InKey, int32 Version, FName InFriendlyName);

	/** Dumps all registered versions to log */
	static void DumpVersionsToLog();
};
