// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AttributeSet.generated.h"

class UAbilitySystemComponent;

USTRUCT(BlueprintType)
struct GAMEPLAYABILITIES_API FGameplayAttribute
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

	float GetNumericValueChecked(const UAttributeSet* Src) const;
	
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

UCLASS(DefaultToInstanced, Blueprintable)
class GAMEPLAYABILITIES_API UAttributeSet : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	bool IsSupportedForNetworking() const override
	{
		return true;
	}

	/** Called just before modifying the value of an attribute. AttributeSet can make additional modifications here. */
	virtual void PreAttributeModify(struct FGameplayEffectModCallbackData &Data) { }
	
	/** Called just after modifying the value of an attribute. No more changes can be made */
	virtual void PostAttributeModify(const struct FGameplayEffectModCallbackData &Data) { }

	void InitFromMetaDataTable(const UDataTable* DataTable);

	void ClampFromMetaDataTable(const UDataTable *DataTable);

	virtual void PrintDebug();
};

USTRUCT()
struct GAMEPLAYABILITIES_API FGlobalCurveDataOverride
{
	GENERATED_USTRUCT_BODY()

	TArray<UCurveTable*>	Overrides;
};


/**
 *	Generic numerical value in the form Coeffecient * Curve[Level] 
 */
USTRUCT()
struct GAMEPLAYABILITIES_API FScalableFloat
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
 *	DataTable that allows us to define meta data about attributes. Still a work in progress.
 */
USTRUCT(BlueprintType)
struct GAMEPLAYABILITIES_API FAttributeMetaData : public FTableRowBase
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

/**
 *	Helper struct that facilates initializing attribute set default values from spread sheets (UCurveTable).
 *	Projects are free to initialize their attribute sets however they want. This is just want example that is 
 *	useful in some cases.
 *	
 *	Basic idea is to have a spreadsheet in this form: 
 *	
 *									1	2	3	4	5	6	7	8	9	10	11	12	13	14	15	16	17	18	19	20
 *
 *	Default.Health.MaxHealth		100	200	300	400	500	600	700	800	900	999	999	999	999	999	999	999	999	999	999	999
 *	Default.Health.HealthRegenRate	1	1	1	1	1	1	1	1	1	1	1	1	1	1	1	1	1	1	1	1
 *	Default.Health.AttackRating		10	10	10	10	10	10	10	10	10	10	10	10	10	10	10	10	10	10	10	10
 *	Default.Move.MaxMoveSpeed		500	500	500	500	500	500	500	500	500	500	500	500	500	500	500	500	500	500	500	500
 *	Hero1.Health.MaxHealth			100	100	100	100	100	100	100	100	100	100	100	100	100	100	100	100	100	100	100	100
 *	Hero1.Health.HealthRegenRate	1	1	1	1	1	1	1	1	1	1	1	1	1	1	1	1 	1	1	1	1
 *	Hero1.Health.AttackRating		10	10	10	10	10	10	10	10	10	10	10	10	10	10	10	10	10	10	10	10
 *	Hero1.Move.MaxMoveSpeed			500	500	500	500	500	500	500	500	500	500	500	500	500	500	500	500	500	500	500	500
 *	
 *	
 *	Where rows are in the form: [GroupName].[AttributeSetName].[Attribute]
 *	GroupName			- arbitrary name to identify the "group"
 *	AttributeSetName	- what UAttributeSet the attributes belong to. (Note that this is a simple partial match on the UClass name. "Health" matches "UMyGameHealthSet").
 *	Attribute			- the name of the actual attribute property (matches full name).
 *		
 *	Colums represent "Level". 
 *	
 *	FAttributeSetInitter::PreloadAttributeSetData(UCurveTable*)
 *	This transforms the CurveTable into a more effecient format to read in at run time. Should be called from UAbilitySystemGlobals for example.
 *
 *	FAttributeSetInitter::InitAttributeSetDefaults(UAbilitySystemComponent* AbilitySystemComponent, FName GroupName, int32 Level) const;
 *	This initializes the given AbilitySystemComponent's attribute sets with the specified GroupName and Level. Game code would be expected to call
 *	this when spawning a new Actor, or leveling up an actor, etc.
 *	
 *	Example Game code usage:
 *	
 *	IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals()->GetAttributeSetInitter()->InitAttributeSetDefaults(MyCharacter->AbilitySystemComponent, "Hero1", MyLevel);
 *	
 *	Notes:
 *	-This lets system designers specify arbitrary values for attributes. They can be based on any formula they want.
 *	-Projects with very large level caps may wish to take a simpler "Attributes gained per level" approach.
 *	-Anything initialized in this method should not be directly modified by gameplay effects. E.g., if MaxMoveSpeed scales with level, anything else that 
 *		modifies MaxMoveSpeed should do so with a non-instant GameplayEffect.
 *	-"Default" is currently the hardcoded, fallback GroupName. If InitAttributeSetDefaults is called without a valid GroupName, we will fallback to default.
 *
 *
 */

struct GAMEPLAYABILITIES_API FAttributeSetInitter
{
	void PreloadAttributeSetData(UCurveTable* CurveData);

	void InitAttributeSetDefaults(UAbilitySystemComponent* AbilitySystemComponent, FName GroupName, int32 Level) const;

private:

	struct FAttributeDefaultValueList
	{
		void AddPair(UNumericProperty* InProperty, float InValue)
		{
			List.Add(FOffsetValuePair(InProperty, InValue));
		}

		struct FOffsetValuePair
		{
			FOffsetValuePair(UNumericProperty* InProperty, float InValue)
			: Property(InProperty), Value(InValue) { }

			UNumericProperty*	Property;
			float		Value;
		};

		TArray<FOffsetValuePair>	List;
	};

	struct FAttributeSetDefaults
	{
		TMap<TSubclassOf<UAttributeSet>, FAttributeDefaultValueList> DataMap;
	};

	struct FAttributeSetDefaulsCollection
	{
		TArray<FAttributeSetDefaults>		LevelData;
	};

	TMap<FName, FAttributeSetDefaulsCollection>	Defaults;
};