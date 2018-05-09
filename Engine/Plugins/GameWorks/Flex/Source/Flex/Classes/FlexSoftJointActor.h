// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "PhysicsEngine/RigidBodyBase.h"
#include "FlexSoftJointActor.generated.h"

class UBillboardComponent;

UCLASS(MinimalAPI, hideCategories = (Collision, Input), showCategories = ("Input|MouseInput", "Input|TouchInput"), ComponentWrapperClass)
class AFlexSoftJointActor : public ARigidBodyBase
{
	GENERATED_UCLASS_BODY()

public:
#if WITH_EDITOR
	//~ Begin AActor Interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;
	//~ End AActor Interface.
#endif

public:
	/** Returns SoftJointComponent subobject **/
	FLEX_API class UFlexSoftJointComponent* GetSoftJointComponent() const;
#if WITH_EDITORONLY_DATA
	/** Returns SpriteComponent subobject **/
	FLEX_API UBillboardComponent* GetSpriteComponent() const;
#endif

private:
	/** Soft joint component */
	UPROPERTY(Category = FlexSoftJointActor, VisibleAnywhere, BlueprintReadOnly, meta = (ExposeFunctionCategories = "Activation,Components|Activation,Physics,Physics|Components|SoftJoint", AllowPrivateAccess = "true"))
	class UFlexSoftJointComponent* SoftJointComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UBillboardComponent* SpriteComponent;
#endif
};



