// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ObjectMacros.h"
#include "UObject/NoExportTypes.h"
#include "HierarchicalRig.h"
#include "HumanRig.generated.h"

//  @note this is not really property safe. You really have to know what you're asking and getting
// @ if you set different property than what you're asking from BP, it won't work. I
USTRUCT()
struct FCustomProperty
{
	GENERATED_USTRUCT_BODY()

	/** The name of the property that this manipulator controls (this can be a transform, vector, rotator etc.) */
	UPROPERTY(EditAnywhere, Category = FCustomProperty)
	FName PropertyToCache;

	/** The property that this manipulator controls (this can be a transform, vector, rotator etc.) p */
	// weak object is better choice but expensive. For now it has to live within the object,
	UPROPERTY(Transient)
	UObject* Container;

	FCustomProperty()
		: Container(nullptr)
	{
	}

	bool IsValid() const
	{
		return Container != nullptr && PropertyToCache != NAME_None;
	}

	void Initialize(UObject* InContainer)
	{
		Container = InContainer;
	}

	// this won't work with array
	template< typename T, typename P >
	T GetValue() const;

	// this won't work with array
	template< typename T, typename P >
	void SetValue(const T& Value);

	void NotifyPreEditChangeProperty(UObject* InContainer, UProperty* CachedProperty);
	void NotifyPostEditChangeProperty(UObject* InContainer, UProperty* CachedProperty);
};

USTRUCT()
struct FLimbControl
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = FLimbControl)
	// FK data
	FName				FKChainName[3];

	UPROPERTY(EditAnywhere, Category = FLimbControl)
	// IK data
	FName				IKChainName[3];

	UPROPERTY(EditAnywhere, Category = FLimbControl)
	FName				IKEffectorName;
	
	UPROPERTY(EditAnywhere, Category = FLimbControl)
	FName				IKJointTargetName;

	// @todo: for no flip ik?
	// FName				IKJointTargetBase;

	// Output
	UPROPERTY(EditAnywhere, Category = FLimbControl)
	FName				ResultChain[3];

	UPROPERTY(EditAnywhere, Category = FLimbControl)
	FCustomProperty		IKBlendWeight;

	UPROPERTY(EditAnywhere, Category = FLimbControl)
	FCustomProperty		IKSpaceMode;

	// keep the initial length 
	UPROPERTY(transient)
	float				UpperLimbLength;

	UPROPERTY(transient)
	float				LowerLimbLength;

	FLimbControl()
	{}

	void Initialize(UObject* InContainer, float InUpperLimbLen, float InLowerLimbLen);
};

UCLASS()
class CONTROLRIG_API UHumanRig : public UHierarchicalRig
{
	GENERATED_UCLASS_BODY()


#if WITH_EDITOR
	// BEGIN: editor set up functions functions
	void AddLeg(FName UpperLegNode, FName LowerLegNode, FName AnkleLegNode);
	void AddTwoBoneIK(FName UpperNode, FName MiddleNode, FName EndNode, FName& OutJointTarget, FName& OutEffector);
	void AddFKNode(FName& InOutNodeName, FName& OutCtrlNodeName, FName InParentNode, FTransform InTransform, FName LinkNode);
	void EnsureUniqueName(FName& InOutNodeName);
	// END: editor set up functions functions
#endif // WITH_EDITOR
private:
	// ControlRig interface start
	virtual void Evaluate() override;
	virtual void Initialize() override;
	// ControlRig interface end

	// UHierarchicalRig interface start
	virtual bool IsManipulatorEnabled(UControlManipulator* InManipulator) const override;
	// UHierarchicalRig interface end

	// Helper functions
	FTransform Lerp(const FName& ANode, const FName& BNode, float Weight);

	UPROPERTY(EditAnywhere, Category = Setup)
	TArray<FLimbControl> Limbs;

public:
	/** Switch functions */
	void CorrectIKSpace(FLimbControl& Control);
	void SwitchToIK(FLimbControl& Control);
	void SwitchToFK(FLimbControl& Control);

	/** Get the space setting that the lib this node is in is using */
	bool GetIKSpaceForNode(FName Node, EIKSpaceMode& OutIKSpace) const;
};
