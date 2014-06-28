// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperSpriteActor.h"

//////////////////////////////////////////////////////////////////////////
// APaperSpriteActor

APaperSpriteActor::APaperSpriteActor(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	RenderComponent = PCIP.CreateDefaultSubobject<UPaperSpriteComponent>(this, TEXT("RenderComponent"));
	RenderComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RenderComponent->Mobility = EComponentMobility::Static;

	RootComponent = RenderComponent;
}

#if WITH_EDITOR
bool APaperSpriteActor::GetReferencedContentObjects(TArray<UObject*>& Objects) const
{
	if (UPaperSprite* SourceSprite = RenderComponent->GetSprite())
	{
		Objects.Add(SourceSprite);
	}
	return true;
}
#endif
