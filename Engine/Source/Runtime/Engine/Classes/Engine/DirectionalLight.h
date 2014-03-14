// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DirectionalLight.generated.h"

UCLASS(HeaderGroup=Light, ClassGroup=(Lights, DirectionalLights), MinimalAPI, meta=(ChildCanTick))
class ADirectionalLight : public ALight
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITORONLY_DATA
	// Reference to editor visualization arrow
	UPROPERTY()
	TSubobjectPtr<UArrowComponent> ArrowComponent;
#endif

public:

	// Begin UObject Interface
	virtual void PostLoad() OVERRIDE;
#if WITH_EDITOR
	virtual void LoadedFromAnotherClass(const FName& OldClassName) OVERRIDE;
#endif
	// End UObject Interface
};



