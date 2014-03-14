// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Collision.cpp: AActor collision implementation
=============================================================================*/

#include "EnginePrivate.h"


AActor* FHitResult::GetActor() const
{
	return Actor.Get();
}

AActor* FOverlapResult::GetActor() const
{
	return Actor.Get();
}