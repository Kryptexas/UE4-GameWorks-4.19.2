// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UserWidget.generated.h"

UCLASS(Abstract, hideCategories=(Object, Actor))
class UMG_API AUserWidget : public AActor
{
	GENERATED_UCLASS_BODY()

	// AActor interface
	virtual void PostActorCreated() OVERRIDE;
	virtual void Destroyed() OVERRIDE;
	virtual void RerunConstructionScripts() OVERRIDE;
	// End of AActor interface

	USlateWrapperComponent* GetWidgetHandle(TSharedRef<SWidget> InWidget);

	TSharedRef<SWidget> GetRootWidget();

private:
	TSharedPtr<SWidget> RootWidget;
	TMap< SWidget*, USlateWrapperComponent* > WidgetToComponent;

private:
	void RebuildWrapperWidget();
};
