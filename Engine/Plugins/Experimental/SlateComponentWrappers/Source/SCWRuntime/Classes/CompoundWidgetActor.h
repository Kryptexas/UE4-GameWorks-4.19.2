// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CompoundWidgetActor.generated.h"

UCLASS(Abstract, hideCategories=(Object, Actor))
class SCWRUNTIME_API ACompoundWidgetActor : public AActor
{
	GENERATED_UCLASS_BODY()

	// AActor interface
	virtual void PostActorCreated() OVERRIDE;
	virtual void Destroyed() OVERRIDE;
	virtual void RerunConstructionScripts() OVERRIDE;
	// End of AActor interface

	TSharedRef<SWidget> GetWidget();

private:
	TSharedPtr<SOverlay> MyWrapperWidget;

private:
	void EnsureWidgetExists();
	void RebuildWrapperWidget();
};
