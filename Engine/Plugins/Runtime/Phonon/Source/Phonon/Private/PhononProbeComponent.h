//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#pragma once

#include "Components/ActorComponent.h"
#include "Components/BillboardComponent.h"
#include "PhononProbeComponent.generated.h"

UCLASS(ClassGroup = (Audio), HideCategories = (Activation, Collision))
class PHONON_API UPhononProbeComponent : public UBillboardComponent
{
	GENERATED_BODY()

public:
	UPhononProbeComponent();

	// Influence radius defines the volume within which this probe's sample data is used for rendering.
	UPROPERTY(EditAnywhere, Category = ProbeSettings, meta = (ClampMin = "0.0", ClampMax = "1024.0", UIMin = "0.0", UIMax = "1024.0"))
	float InfluenceRadius;
};
