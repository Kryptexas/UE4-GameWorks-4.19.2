// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "HAL/OutputDevices.h"

#include "ComponentReregisterContext.h"	

void FGameplayAttribute::SetNumericValueChecked(const float NewValue, class UAttributeSet* Dest) const
{
	UNumericProperty *NumericProperty = CastChecked<UNumericProperty>(Attribute);
	void * ValuePtr = NumericProperty->ContainerPtrToValuePtr<void>(Dest);
	NumericProperty->SetFloatingPointPropertyValue(ValuePtr, NewValue);
}

float FGameplayAttribute::GetNumericValueChecked(const UAttributeSet* Src) const
{
	UNumericProperty* NumericProperty = CastChecked<UNumericProperty>(Attribute);
	const void* ValuePtr = NumericProperty->ContainerPtrToValuePtr<void>(Src);
	return NumericProperty->GetFloatingPointPropertyValue(ValuePtr);
}

UAttributeSet::UAttributeSet(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

void UAttributeSet::InitFromMetaDataTable(const UDataTable* DataTable)
{
	static const FString Context = FString(TEXT("UAttribute::BindToMetaDataTable"));

	for( TFieldIterator<UProperty> It(GetClass(), EFieldIteratorFlags::IncludeSuper) ; It ; ++It )
	{
		UProperty* Property = *It;
		UNumericProperty *NumericProperty = Cast<UNumericProperty>(Property);
		if (NumericProperty)
		{
			FString RowNameStr = FString::Printf(TEXT("%s.%s"), *Property->GetOuter()->GetName(), *Property->GetName());
		
			FAttributeMetaData * MetaData = DataTable->FindRow<FAttributeMetaData>(FName(*RowNameStr), Context, false);
			if (MetaData)
			{
				void *Data = NumericProperty->ContainerPtrToValuePtr<void>(this);
				NumericProperty->SetFloatingPointPropertyValue(Data, MetaData->BaseValue);
			}
		}
	}

	PrintDebug();
}

void UAttributeSet::ClampFromMetaDataTable(const UDataTable *DataTable)
{
	// Looking up a datatable row everytime a value changes in order to apply clamping pretty much sucks.
	static const FString Context = FString(TEXT("UAttribute::BindToMetaDataTable"));

	UE_LOG(LogActorComponent, Warning, TEXT("UAttribute::ClampFromMetaDataTable. UAttribute %s from UDataTable %s"), *GetName(), *DataTable->GetName());

	for( TFieldIterator<UProperty> It(GetClass(), EFieldIteratorFlags::IncludeSuper) ; It ; ++It )
	{
		UProperty *Property = *It;
		UNumericProperty *NumericProperty = Cast<UNumericProperty>(Property);
		if (NumericProperty)
		{
			FString RowNameStr = FString::Printf(TEXT("%s.%s"), *Property->GetOuter()->GetName(), *Property->GetName());
			FAttributeMetaData * MetaData = DataTable->FindRow<FAttributeMetaData>(FName(*RowNameStr), Context, false);
			if (MetaData)
			{
				float CurrentValue = NumericProperty->GetFloatingPointPropertyValue(this);
				CurrentValue = FMath::Clamp(CurrentValue, MetaData->MinValue, MetaData->MaxValue);

				UE_LOG(LogActorComponent, Warning, TEXT("   Found Row for %s. Clamping: %.2f <%.2f, %.2f>"), *RowNameStr, CurrentValue, MetaData->MinValue, MetaData->MaxValue );

				NumericProperty->SetFloatingPointPropertyValue(NumericProperty->ContainerPtrToValuePtr<void>(this), CurrentValue);
			}
		}
	}

	PrintDebug();
}

void UAttributeSet::PrintDebug()
{
	
}

FAttributeMetaData::FAttributeMetaData()
	: MinValue(0.f)
	, MaxValue(1.f)
{

}

void FScalableFloat::FinalizeCurveData(const FGlobalCurveDataOverride *GlobalOverrides)
{
	static const FString ContextString = TEXT("FScalableFloat::FinalizeCurveData");

	// We are a static value, so do nothing.
	if (Curve.RowName == NAME_None)
	{
		return;
	}

	// Tied to an explicit table, so bind now.
	if (Curve.CurveTable != NULL)
	{
		FinalCurve = Curve.GetCurve(ContextString);
		return;
	}

	// We have overrides
	if (GlobalOverrides)
	{
		for (UCurveTable* OverrideTable : GlobalOverrides->Overrides)
		{
			FinalCurve = OverrideTable->FindCurve(Curve.RowName, ContextString, false);
			if (FinalCurve)
			{
				return;
			}
		}
	}

	// Look at global defaults
	const UCurveTable * GlobalTable = IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals()->GetGlobalCurveTable();
	if (GlobalTable)
	{
		FinalCurve = GlobalTable->FindCurve(Curve.RowName, ContextString, false);
	}

	if (!FinalCurve)
	{
		ABILITY_LOG(Warning, TEXT("Unable to find RowName: %s for FScalableFloat."), *Curve.RowName.ToString());
	}
}

float FScalableFloat::GetValueAtLevel(float Level) const
{
	if (FinalCurve)
	{
		return Value * FinalCurve->Eval(Level);
	}

	return Value;
}

bool FGameplayAttribute::operator==(const FGameplayAttribute& Other) const
{
	return ((Other.Attribute == Attribute));
}

bool FGameplayAttribute::operator!=(const FGameplayAttribute& Other) const
{
	return ((Other.Attribute != Attribute));
}

bool FScalableFloat::operator==(const FScalableFloat& Other) const
{
	return ((Other.Curve == Curve) && (Other.Value == Value));
}

bool FScalableFloat::operator!=(const FScalableFloat& Other) const
{
	return ((Other.Curve != Curve) || (Other.Value != Value));
}

// ------------------------------------------------------------------------------------
//
// ------------------------------------------------------------------------------------
TSubclassOf<UAttributeSet> FindBestAttributeClass(TArray<TSubclassOf<UAttributeSet> >& ClassList, FString PartialName)
{
	for (auto Class : ClassList)
	{
		if (Class->GetName().Contains(PartialName))
		{
			return Class;
		}
	}

	return nullptr;
}

/**
 *	Transforms CurveTable data into format more effecient to read at runtime.
 *	UCurveTable requires string parsing to map to GroupName/AttributeSet/Attribute
 *	Each curve in the table represents a *single attribute's values for all levels*.
 *	At runtime, we want *all attribute values at given level*.
 */
void FAttributeSetInitter::PreloadAttributeSetData(UCurveTable* CurveData)
{
	if(!ensure(CurveData))
	{
		return;
	}

	/**
	 *	Get list of AttributeSet classes loaded
	 */

	TArray<TSubclassOf<UAttributeSet> >	ClassList;
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* TestClass = *ClassIt;
		if (TestClass->IsChildOf(UAttributeSet::StaticClass()))
		{
			ClassList.Add(TestClass);
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			// This can only work right now on POD attribute sets. If we ever support FStrings or TArrays in AttributeSets
			// we will need to update this code to not use memcpy etc.
			for (TFieldIterator<UProperty> PropIt(TestClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
			{
				if (!PropIt->HasAllPropertyFlags(CPF_IsPlainOldData))
				{
					ABILITY_LOG(Error, TEXT("FAttributeSetInitter::PreloadAttributeSetData Unable to Handle AttributeClass %s because it has a non POD property: %s"),
						*TestClass->GetName(), *PropIt->GetName());
					return;
				}
			}
#endif
		}
	}

	/**
	 *	Loop through CurveData table and build sets of Defaults that keyed off of Name + Level
	 */

	for (auto It = CurveData->RowMap.CreateConstIterator(); It; ++It)
	{
		FString RowName = It.Key().ToString();
		FString ClassName;
		FString SetName;
		FString AttributeName;
		FString Temp;

		RowName.Split(TEXT("."), &ClassName, &Temp);
		Temp.Split(TEXT("."), &SetName, &AttributeName);

		if (!ensure(!ClassName.IsEmpty() && !SetName.IsEmpty() && !AttributeName.IsEmpty()))
		{
			ABILITY_LOG(Warning, TEXT("FAttributeSetInitter::PreloadAttributeSetData Unable to parse row %s in %s"), *RowName, *CurveData->GetName());
			continue;
		}

		// Find the AttributeSet

		TSubclassOf<UAttributeSet> Set = FindBestAttributeClass(ClassList, SetName);
		if (!Set)
		{
			ABILITY_LOG(Warning, TEXT("FAttributeSetInitter::PreloadAttributeSetData Unable to match AttributeSet from %s (row: %s)"), *SetName, *RowName);
			continue;
		}

		// Find the UProperty

		UNumericProperty* Property = FindField<UNumericProperty>(*Set, *AttributeName);
		if (!Property)
		{
			ABILITY_LOG(Warning, TEXT("FAttributeSetInitter::PreloadAttributeSetData Unable to match Attribute from %s (row: %s)"), *AttributeName, *RowName);
			continue;
		}

		FRichCurve* Curve = It.Value();
		FName ClassFName = FName(*ClassName);
		FAttributeSetDefaulsCollection& DefaultCollection = Defaults.FindOrAdd(ClassFName);

		int32 LastLevel = Curve->GetLastKey().Time;
		DefaultCollection.LevelData.SetNum(FMath::Max(LastLevel, DefaultCollection.LevelData.Num()));

		
		//At this point we know the Name of this "class"/"group", the AttributeSet, and the Property Name. Now loop through the values on the curve to get the attribute default value at each level.
		for (auto KeyIter = Curve->GetKeyIterator(); KeyIter; ++KeyIter)
		{
			const FRichCurveKey& CurveKey = *KeyIter;

			int32 Level = CurveKey.Time;
			float Value = CurveKey.Value;

			FAttributeSetDefaults& SetDefaults = DefaultCollection.LevelData[Level-1];

			FAttributeDefaultValueList* DefaultDataList = SetDefaults.DataMap.Find(Set);
			if (DefaultDataList == nullptr)
			{
				ABILITY_LOG(Verbose, TEXT("Initializing new default set for %s[%d]. PropertySize: %d.. DefaultSize: %d"), *Set->GetName(), Level, Set->GetPropertiesSize(), UAttributeSet::StaticClass()->GetPropertiesSize());
				
				DefaultDataList = &SetDefaults.DataMap.Add(Set);
			}

			// Import curve value into default data

			check(DefaultDataList);
			DefaultDataList->AddPair(Property, Value);
		}


	}
}

void FAttributeSetInitter::InitAttributeSetDefaults(UAbilitySystemComponent* AbilitySystemComponent, FName GroupName, int32 Level) const
{
	SCOPE_CYCLE_COUNTER(STAT_InitAttributeSetDefaults);
	
	const FAttributeSetDefaulsCollection* Collection = Defaults.Find(GroupName);
	if (!Collection)
	{
		ABILITY_LOG(Warning, TEXT("Unable to find DefaultAttributeSet Group %s. Failing back to Defaults"), *GroupName.ToString());
		Collection = Defaults.Find(FName(TEXT("Default")));
		if (!Collection)
		{
			ABILITY_LOG(Error, TEXT("FAttributeSetInitter::InitAttributeSetDefaults Default DefaultAttributeSet not found! Skipping Initialization"));
			return;
		}
	}

	if (!Collection->LevelData.IsValidIndex(Level))
	{
		// We could eventually extrapolate values outside of the max defined levels
		ABILITY_LOG(Warning, TEXT("Attribute defaults for Level %d are not defined! Skipping"), Level);
		return;
	}

	const FAttributeSetDefaults& SetDefaults = Collection->LevelData[Level];
	for (const UAttributeSet* Set : AbilitySystemComponent->SpawnedAttributes)
	{
		const FAttributeDefaultValueList* DefaultDataList = SetDefaults.DataMap.Find(Set->GetClass());
		if (DefaultDataList)
		{
			ABILITY_LOG(Warning, TEXT("Initializing Set %s"), *Set->GetName());

			for (auto& DataPair : DefaultDataList->List)
			{
				check(DataPair.Property);

				DataPair.Property->SetFloatingPointPropertyValue(DataPair.Property->ContainerPtrToValuePtr<void>(const_cast<UAttributeSet*>(Set)), DataPair.Value);
			}
		}		
	}
}
