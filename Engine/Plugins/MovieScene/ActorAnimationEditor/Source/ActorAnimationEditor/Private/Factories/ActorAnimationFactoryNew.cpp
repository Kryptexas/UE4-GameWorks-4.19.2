// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ActorAnimationEditorPrivatePCH.h"
#include "ActorAnimation.h"
#include "ActorAnimationFactoryNew.h"


#define LOCTEXT_NAMESPACE "MovieSceneFactory"


/* UActorAnimationFactory structors
 *****************************************************************************/

UActorAnimationFactoryNew::UActorAnimationFactoryNew(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// @todo sequencer: Only allow users to create new MovieScenes if that feature is turned on globally.
	if (FParse::Param(FCommandLine::Get(), TEXT("Sequencer")))
	{
		bCreateNew = true;
	}

	bEditAfterNew = true;
	SupportedClass = UActorAnimation::StaticClass();
}


/* UFactory interface
 *****************************************************************************/

UObject* UActorAnimationFactoryNew::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	auto NewActorAnimation = NewObject<UActorAnimation>(InParent, Name, Flags|RF_Transactional);
	NewActorAnimation->Initialize();

	return NewActorAnimation;
}


bool UActorAnimationFactoryNew::ShouldShowInNewMenu() const
{
	// @todo sequencer: Only allow users to create new MovieScenes if that feature is turned on globally.
	if (!FParse::Param(FCommandLine::Get(), TEXT("Sequencer")))
	{
		return false;
	}

	return true;
}


#undef LOCTEXT_NAMESPACE