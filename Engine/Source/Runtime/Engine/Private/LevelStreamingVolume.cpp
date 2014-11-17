// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Engine/LevelStreamingVolume.h"

ALevelStreamingVolume::ALevelStreamingVolume(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BrushComponent->BodyInstance.bEnableCollision_DEPRECATED = false;

	BrushComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	BrushComponent->bAlwaysCreatePhysicsState = true;

	bColored = true;
	BrushColor.R = 255;
	BrushColor.G = 165;
	BrushColor.B = 0;
	BrushColor.A = 255;

	StreamingUsage = SVB_LoadingAndVisibility;
}

void ALevelStreamingVolume::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	if (GIsEditor)
	{
		UpdateStreamingLevelsRefs();
	}
#endif//WITH_EDITOR
}

#if WITH_EDITOR

void ALevelStreamingVolume::UpdateStreamingLevelsRefs()
{
	StreamingLevelNames.Reset();
	
	UWorld* OwningWorld = GetWorld();
	if (OwningWorld)
	{
		for (ULevelStreaming* LevelStreaming : OwningWorld->StreamingLevels)
		{
			if (LevelStreaming && LevelStreaming->EditorStreamingVolumes.Find(this) != INDEX_NONE)
			{
				StreamingLevelNames.Add(LevelStreaming->PackageName);
			}
		}
	}
}

#endif// WITH_EDITOR