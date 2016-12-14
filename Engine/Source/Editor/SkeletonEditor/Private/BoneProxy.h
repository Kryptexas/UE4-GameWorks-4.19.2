// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "TickableEditorObject.h"
#include "UObject/Object.h"
#include "UObject/WeakObjectPtr.h"
#include "BoneProxy.generated.h"


class UDebugSkelMeshComponent;

/** Proxy object used to display/edit bone transforms */
UCLASS()
class UBoneProxy : public UObject, public FTickableEditorObject
{
	GENERATED_BODY()

public:
	UBoneProxy();

	/** UObject interface */
	virtual void PreEditChange(FEditPropertyChain& PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/** FTickableEditorObject interface */
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	/** The name of the bone we have selected */
	UPROPERTY(VisibleAnywhere, Category = "Bone")
	FName BoneName;

	/** Bone location */
	UPROPERTY(EditAnywhere, Category = "Transform")
	FVector Location;

	/** Bone rotation */
	UPROPERTY(EditAnywhere, Category = "Transform")
	FRotator Rotation;

	/** Bone scale */
	UPROPERTY(EditAnywhere, Category = "Transform")
	FVector Scale;

	/** Bone reference location (local) */
	UPROPERTY(VisibleAnywhere, Category = "Reference Transform")
	FVector ReferenceLocation;

	/** Bone reference rotation (local) */
	UPROPERTY(VisibleAnywhere, Category = "Reference Transform")
	FRotator ReferenceRotation;

	/** Bone reference scale (local) */
	UPROPERTY(VisibleAnywhere, Category = "Reference Transform")
	FVector ReferenceScale;

	UPROPERTY()
	TWeakObjectPtr<UDebugSkelMeshComponent> SkelMeshComponent;

	/** Whether to use local or world location */
	bool bLocalLocation;

	/** Whether to use local or world rotation */
	bool bLocalRotation;

	// Handle property deltas
	FVector PreviousLocation;
	FRotator PreviousRotation;
	FVector PreviousScale;

	bool bManipulating;
};