// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "StatsViewerPrivatePCH.h"
#include "TextureStats.h"

UTextureStats::UTextureStats(const class FPostConstructInitializeProperties& PCIP) :
	Super(PCIP),
	LastTimeRendered( FLT_MAX )
{
}
