// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

DEFINE_LOG_CATEGORY_STATIC(LogVolume, Log, All);

ADefaultPhysicsVolume::ADefaultPhysicsVolume(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

#if WITH_EDITORONLY_DATA
	// Not allowed to be selected or edited within Unreal Editor
	bEditable = false;
#endif // WITH_EDITORONLY_DATA
}

void ADefaultPhysicsVolume::Destroyed()
{
	UE_LOG(LogVolume, Log, TEXT("%s destroyed!"), *GetName());
}
