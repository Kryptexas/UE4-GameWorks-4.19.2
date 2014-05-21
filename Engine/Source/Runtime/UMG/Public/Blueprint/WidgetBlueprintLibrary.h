// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "WidgetBlueprintLibrary.generated.h"

UCLASS(MinimalAPI)
class UWidgetBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	/** Creates a widget */
	UFUNCTION(BlueprintCallable, meta=( HidePin="WorldContextObject", DefaultToSelf="WorldContextObject", UnsafeDuringActorConstruction = "true", FriendlyName = "Create Widget" ), Category="User Interface|Widget")
	static class AUserWidget* Create(UObject* WorldContextObject, TSubclassOf<class AUserWidget> WidgetType);
};