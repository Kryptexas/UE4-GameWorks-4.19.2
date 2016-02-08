// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Camera/CineCameraComponent.h"
#include "Camera/CineCameraActor.h"

#define LOCTEXT_NAMESPACE "CineCameraActor"

//////////////////////////////////////////////////////////////////////////
// ACameraActor

ACineCameraActor::ACineCameraActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer
			.SetDefaultSubobjectClass<UCineCameraComponent>(TEXT("CameraComponent"))
	)
{
}

#if WITH_EDITOR
void ACineCameraActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

#undef LOCTEXT_NAMESPACE
