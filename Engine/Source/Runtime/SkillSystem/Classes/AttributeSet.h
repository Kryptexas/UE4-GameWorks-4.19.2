// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

//#include "../Engine/DataTable.h"
#include "AttributeSet.generated.h"

USTRUCT(BlueprintType)
struct SKILLSYSTEM_API FGameplayAttribute
{
	GENERATED_USTRUCT_BODY()

	FGameplayAttribute()
		: Attribute(NULL)
	{
	}

	FGameplayAttribute(UProperty *NewProperty)
		: Attribute(NewProperty)
	{
	}

	void SetUProperty(UProperty *NewProperty)
	{
		Attribute = NewProperty;
	}

	UProperty * GetUProperty() const
	{
		return Attribute;
	}

	UClass * GetAttributeSetClass() const
	{
		check(Attribute);
		return CastChecked<UClass>(Attribute->GetOuter());
	}

	void SetNumericValueChecked(const float NewValue, class UAttributeSet* Dest) const;

	float GetNumericValueChecked(class UAttributeSet* Src) const;

	/** Equality/Inequality operators */
	bool operator==(const FGameplayAttribute& Other) const;
	bool operator!=(const FGameplayAttribute& Other) const;

	friend uint32 GetTypeHash( const FGameplayAttribute& InAttribute )
	{
		// FIXME: Use ObjectID or something to get a better, less collision prone hash
		return PointerHash(InAttribute.Attribute);
	}

	FString GetName() const
	{
		return Attribute->GetName();
	}

private:
	friend class FAttributePropertyDetails;

	UPROPERTY(Category=GameplayAttribute, EditDefaultsOnly)
	UProperty*	Attribute;
};

UCLASS(Blueprintable)
class SKILLSYSTEM_API UAttributeSet : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/** Called just before modifying the value of an attribute. AttributeSet can make additional modifications here. */
	virtual void PreAttributeModify(struct FGameplayEffectModCallbackData &Data) { }
	
	/** Called just after modifying the value of an attribute. No more changes can be made */
	virtual void PostAttributeModify(const struct FGameplayEffectModCallbackData &Data) { }

	void InitFromMetaDataTable(const UDataTable* DataTable);

	void ClampFromMetaDataTable(const UDataTable *DataTable);

	virtual void PrintDebug();

private:

	void OnAttributeChange(UProperty *Property);
};

USTRUCT()
struct SKILLSYSTEM_API FGlobalCurveDataOverride
{
	GENERATED_USTRUCT_BODY()

	TArray<UCurveTable*>	Overrides;
};

USTRUCT()
struct SKILLSYSTEM_API FScalableFloat
{
	GENERATED_USTRUCT_BODY()

	FScalableFloat()
		: Value(0.f)
		, FinalCurve(NULL)
	{

	}

public:

	UPROPERTY(EditDefaultsOnly, Category=ScalableFloat)
	float	Value;

	UPROPERTY(EditDefaultsOnly, Category=ScalableFloat)
	FCurveTableRowHandle	Curve;

	void FinalizeCurveData(const FGlobalCurveDataOverride *GlobalOverrides);

	float GetValueAtLevel(float Level) const;

	FScalableFloat MakeFinalizedCopy(const FGlobalCurveDataOverride *GlobalOverrides) const
	{
		FScalableFloat Copy(*this);
		Copy.FinalizeCurveData(GlobalOverrides);
		return Copy;
	}

	bool IsStatic() const
	{
		return (Curve.RowName == NAME_None);
	}

	void SetValue(float NewValue)
	{
		Value = NewValue;
		Curve.CurveTable = NULL;
		Curve.RowName = NAME_None;
		FinalCurve = NULL;
	}

	void SetScalingValue(float InCoeffecient, FName InRowName, UCurveTable * InTable)
	{
		Value = InCoeffecient;
		Curve.RowName = InRowName;
		Curve.CurveTable = InTable;
		FinalCurve = NULL;
	}

	void LockValueAtLevel(float Level, FGlobalCurveDataOverride *GlobalOverrides)
	{
		SetValue(GetValueAtLevel(Level));
	}

	float GetValueChecked() const
	{
		check(IsStatic());
		return Value;
	}

	FString ToSimpleString() const
	{
		if (Curve.RowName != NAME_None)
		{
			return FString::Printf(TEXT("%.2f - %s@%s"), Value, *Curve.RowName.ToString(), Curve.CurveTable ? *Curve.CurveTable->GetName() : TEXT("None"));
		}
		return FString::Printf(TEXT("%.2f"), Value);
	}

	/** Equality/Inequality operators */
	bool operator==(const FScalableFloat& Other) const;
	bool operator!=(const FScalableFloat& Other) const;

private:

	FRichCurve * FinalCurve;
};


/**
 *	This is an example of using the property name as the row handle name.
 *	FAttributeMetaData is a table where each row is a single property in a UAttribute class.
 *
 *	I need an exmaple of having a property reference in line
 *
*/
USTRUCT(BlueprintType)
struct SKILLSYSTEM_API FAttributeMetaData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:

	FAttributeMetaData();

	UPROPERTY()
	float		BaseValue;

	UPROPERTY()
	float		MinValue;

	UPROPERTY()
	float		MaxValue;

	UPROPERTY()
	FString		DerivedAttributeInfo;

	UPROPERTY()
	bool		bCanStack;
};


UENUM()
enum EAttributeModifierType
{
	EATTRMOD_Add,
	EATTRMOD_Multiple,
	EATTRMOD_Override,
};

USTRUCT(BlueprintType)
struct FAttributeModifierTest : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:

	UPROPERTY()
	FString	PropertyName;

	UPROPERTY()
	TEnumAsByte<EAttributeModifierType>	ModifierType;

	UPROPERTY()
	FString NewValue;

	UProperty*	GetUProperty();

	void	ApplyModifier(UAttributeSet *Object);

private:

	UProperty *CachedUProperty;
};

USTRUCT()
struct SKILLSYSTEM_API FAttributeModifier
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FGameplayAttribute	PropertyToModify;

	UPROPERTY()
	float		NumericValue;

	UPROPERTY()
	TEnumAsByte<EAttributeModifierType>	ModifierType;
};

USTRUCT()
struct FSomeThingThatHoldsAListOfOtherthings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray< FAttributeModifier	>	ListOfModifiers;

};