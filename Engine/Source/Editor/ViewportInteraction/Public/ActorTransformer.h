// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ViewportTransformer.h"
#include "ActorTransformer.generated.h"


UCLASS()
class VIEWPORTINTERACTION_API UActorTransformer : public UViewportTransformer
{
	GENERATED_BODY()

public:

	UFUNCTION()
	virtual void Init( class UViewportWorldInteraction* InitViewportWorldInteraction ) override;

	UFUNCTION()
	virtual void Shutdown() override;


protected:

	/** Called when selection changes in the level */
	void OnActorSelectionChanged( UObject* ChangedObject );

};

