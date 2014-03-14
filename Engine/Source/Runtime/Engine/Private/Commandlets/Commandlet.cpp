// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UCommandlet::UCommandlet(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	IsServer = true;
	IsClient = true;
	IsEditor = true;
	ShowErrorCount = true;
}
