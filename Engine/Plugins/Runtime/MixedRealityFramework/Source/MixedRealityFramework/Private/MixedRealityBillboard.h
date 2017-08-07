// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/MaterialBillboardComponent.h"
#include "MixedRealityBillboard.generated.h"

class APawn;

UCLASS()
class UMixedRealityBillboard : public UMaterialBillboardComponent
{
	GENERATED_UCLASS_BODY()

public:
	// UActorComponent interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	// End of UActorComponent interface

	// UPrimitiveComponent interface
#if WITH_EDITOR
	virtual uint64 GetHiddenEditorViews() const override
	{
		// we don't want this billboard crowding the editor window, so hide it from all editor
		// views (however, we do want to see it in preview windows, which this doesn't affect)
		return 0xFFFFFFFFFFFFFFFF;
	}
#endif// WITH_EDITOR
	// End of UPrimitiveComponent interface


	/** */
	void SetTargetPawn(const APawn* PlayerPawn);
	/** */
	void SetDepthTarget(USceneComponent* NewDepthTarget);

private:
	/** */
	UPROPERTY()
	USceneComponent* DepthMatchTarget;
};
