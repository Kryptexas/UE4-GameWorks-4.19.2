// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "FlexParticleSpriteEmitter.h"

const FName UFlexParticleSpriteEmitter::DefaultEmitterName(TEXT("Flex Particle Emitter"));

UFlexParticleSpriteEmitter::UFlexParticleSpriteEmitter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EmitterName = DefaultEmitterName;

	Mass = 1.0f;
	bLocalSpace = false;
}
