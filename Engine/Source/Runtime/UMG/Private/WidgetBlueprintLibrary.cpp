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

AUserWidget* UWidgetBlueprintLibrary::Create(UObject* WorldContextObject, TSubclassOf<AUserWidget> WidgetType)
{
	if ( WidgetType->HasAnyClassFlags(CLASS_Abstract) )
	{
		// TODO Error?
		return NULL;
	}

	//TODO ConstructObject after AUserWidget is no longer an actor.

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	AUserWidget* NewWidget = World->SpawnActor<AUserWidget>(WidgetType);
	
	return NewWidget;
}

#undef LOCTEXT_NAMESPACE