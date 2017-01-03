// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ViewportTransformer.generated.h"


UCLASS( abstract )
class VIEWPORTINTERACTION_API UViewportTransformer : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION()
	virtual void Init( class UViewportWorldInteraction* InitViewportWorldInteraction );

	UFUNCTION()
	virtual void Shutdown();


protected:

	/** The viewport world interaction object we're registered with */
	UPROPERTY()
	class UViewportWorldInteraction* ViewportWorldInteraction;

};

