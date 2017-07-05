// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "PhysicsEngine/RigidBodyBase.h"
#include "SoftJointActor.generated.h"

class UBillboardComponent;

UCLASS(MinimalAPI, hideCategories = (Collision, Input), showCategories = ("Input|MouseInput", "Input|TouchInput"), ComponentWrapperClass)
class ASoftJointActor : public ARigidBodyBase
{
	GENERATED_UCLASS_BODY()

private_subobject:
	/** Soft joint component */
	UPROPERTY(Category = SoftJointActor, VisibleAnywhere, BlueprintReadOnly, meta = (ExposeFunctionCategories = "Activation,Components|Activation,Physics,Physics|Components|SoftJoint", AllowPrivateAccess = "true"))
	class USoftJointComponent* SoftJointComponent;

#if WITH_EDITORONLY_DATA
	DEPRECATED_FORGAME(4.6, "SpriteComponent should not be accessed directly, please use GetSpriteComponent() function instead. SpriteComponent will soon be private and your code will not compile.")
	UPROPERTY()
	UBillboardComponent* SpriteComponent;
#endif
public:


#if WITH_EDITOR
	//~ Begin AActor Interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;
	//~ End AActor Interface.
#endif

public:
	/** Returns SoftJointComponent subobject **/
	ENGINE_API class USoftJointComponent* GetSoftJointComponent() const;
#if WITH_EDITORONLY_DATA
	/** Returns SpriteComponent subobject **/
	ENGINE_API UBillboardComponent* GetSpriteComponent() const;
#endif
};



