// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DirectionalLight.generated.h"

UCLASS(ClassGroup=(Lights, DirectionalLights), MinimalAPI, meta=(ChildCanTick))
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
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void LoadedFromAnotherClass(const FName& OldClassName) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	// End UObject Interface
};



