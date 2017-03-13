// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Constraint.h: Render core module definitions.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Constraint.generated.h"

USTRUCT(BlueprintType)
struct ANIMATIONCORE_API FConstraintAxesOption
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FConstraintAxesOption)
	bool bX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FConstraintAxesOption)
	bool bY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FConstraintAxesOption)
	bool bZ;

	FConstraintAxesOption()
		: bX(true)
		, bY(true)
		, bZ(true)
	{}

	// @todo: this might not work with quaternion
	void FilterVector(FVector4& Input) const
	{
		if (!bX)
		{
			Input.X = 0.f;
		}

		if (!bY)
		{
			Input.Y = 0.f;
		}

		if (!bZ)
		{
			Input.Z = 0.f;
		}
	}

	friend FArchive & operator<<(FArchive & Ar, FConstraintAxesOption & D)
	{
		Ar << D.bX;
		Ar << D.bY;
		Ar << D.bZ;

		return Ar;
	}
};

/** A description of how to apply a simple transform constraint */
USTRUCT(BlueprintType)
struct ANIMATIONCORE_API FConstraintDescription
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Constraint")
	bool bTranslation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Constraint")
	bool bRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Constraint")
	bool bScale;

	// this does composed transform - where as individual will accumulate per component
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Constraint")
	bool bParent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Constraint")
	FConstraintAxesOption TranslationAxes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Constraint")
	FConstraintAxesOption RotationAxes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Constraint")
	FConstraintAxesOption ScaleAxes;

	FConstraintDescription()
		: bTranslation(true)
		, bRotation(true)
		, bScale(true)
		, bParent(false)
	{
	}

	friend FArchive & operator<<(FArchive & Ar, FConstraintDescription & D)
	{
		Ar << D.bTranslation;
		Ar << D.bRotation;
		Ar << D.bScale;
		Ar << D.bParent;
		Ar << D.TranslationAxes;
		Ar << D.RotationAxes;
		Ar << D.ScaleAxes;

		return Ar;
	}
};

// individual component is saving different delta
// and they accumulate different
USTRUCT()
struct ANIMATIONCORE_API FConstraintOffset
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FVector Translation;

	UPROPERTY()
	FQuat Rotation;

	UPROPERTY()
	FVector Scale;

	UPROPERTY()
	FTransform Parent;

	FConstraintOffset()
		: Translation(FVector::ZeroVector)
		, Rotation(FQuat::Identity)
		, Scale(FVector::OneVector)
		, Parent(FTransform::Identity)
	{}

	// this applies inverse offset
	void ApplyInverseOffset(const FTransform& InTarget, FTransform& OutSource) const;
	void SaveInverseOffset(const FTransform& Source, const FTransform& Target, const FConstraintDescription& Operator);
	void Reset()
	{
		Translation = FVector::ZeroVector;
		Rotation = FQuat::Identity;
		Scale = FVector::OneVector;
		Parent = FTransform::Identity;
	}

	friend FArchive & operator<<(FArchive & Ar, FConstraintOffset & D)
	{
		Ar << D.Translation;
		Ar << D.Rotation;
		Ar << D.Scale;
		Ar << D.Parent;

		return Ar;
	}
};

USTRUCT(BlueprintType)
struct ANIMATIONCORE_API FTransformConstraint
{
	GENERATED_USTRUCT_BODY()

	// @note thought of separating this out per each but we'll have an issue with applying transform in what order
	// but something to think about if that seems better
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform Constraint")
	FConstraintDescription Operator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform Constraint")
	FName SourceNode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform Constraint")
	FName TargetNode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform Constraint")
	float Weight;

	/** When the constraint is first applied, maintain the offset from the target node */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform Constraint")
	bool bMaintainOffset;

	FTransformConstraint()
		: SourceNode(NAME_None)
		, TargetNode(NAME_None)
		, Weight(1.f)
		, bMaintainOffset(true)
	{}

	friend FArchive & operator<<(FArchive & Ar, FTransformConstraint & D)
	{
		Ar << D.Operator;
		Ar << D.SourceNode;
		Ar << D.TargetNode;
		Ar << D.Weight;
		Ar << D.bMaintainOffset;

		return Ar;
	}
};