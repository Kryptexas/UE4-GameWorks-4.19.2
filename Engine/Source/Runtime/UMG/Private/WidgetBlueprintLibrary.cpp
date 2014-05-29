// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "WidgetBlueprintLibrary.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UWidgetBlueprintLibrary

UWidgetBlueprintLibrary::UWidgetBlueprintLibrary(const FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
}

UUserWidget* UWidgetBlueprintLibrary::Create(UObject* WorldContextObject, TSubclassOf<UUserWidget> WidgetType)
{
	if ( WidgetType->HasAnyClassFlags(CLASS_Abstract) )
	{
		// TODO Error?
		return NULL;
	}

	//TODO ConstructObject after UUserWidget is no longer an actor.

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	UUserWidget* NewWidget = ConstructObject<UUserWidget>(WidgetType, World->GetCurrentLevel());
	//UUserWidget* NewWidget = World->SpawnActor<UUserWidget>(WidgetType);
		
	return NewWidget;
}

#undef LOCTEXT_NAMESPACE