// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


/**
 * Flips selected objects.
 */

#pragma once
#include "GeomModifier_Flip.generated.h"

UCLASS()
class UGeomModifier_Flip : public UGeomModifier_Edit
{
	GENERATED_BODY()
public:
	UGeomModifier_Flip(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UGeomModifier Interface
	virtual bool Supports() override;
protected:
	virtual bool OnApply() override;
	// End UGeomModifier Interface
};



