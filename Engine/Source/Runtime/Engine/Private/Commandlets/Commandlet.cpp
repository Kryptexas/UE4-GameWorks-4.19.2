// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Commandlets/Commandlet.h"

UCommandlet::UCommandlet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	IsServer = true;
	IsClient = true;
	IsEditor = true;
	ShowErrorCount = true;
}
