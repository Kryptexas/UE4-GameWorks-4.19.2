// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AttributeSet.h"
#include "HAL/OutputDevices.h"

#include "ComponentReregisterContext.h"	

void FGameplayAttribute::SetNumericValueChecked(const float NewValue, class UAttributeSet* Dest) const
{
	UNumericProperty *NumericProperty = CastChecked<UNumericProperty>(Attribute);
	void * ValuePtr = NumericProperty->ContainerPtrToValuePtr<void>(Dest);
	NumericProperty->SetFloatingPointPropertyValue(ValuePtr, NewValue);
}

float FGameplayAttribute::GetNumericValueChecked(class UAttributeSet* Src) const
{
	UNumericProperty *NumericProperty = CastChecked<UNumericProperty>(Attribute);
	void * ValuePtr = NumericProperty->ContainerPtrToValuePtr<void>(Src);
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
		UProperty *Property = *It;
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

UProperty* FAttributeModifierTest::GetUProperty()
{
	if (!CachedUProperty)
	{
		FString ClassName;
		FString ThisPropertyName;

		PropertyName.Split( TEXT("."), &ClassName, &ThisPropertyName);

		UClass *FoundClass = FindObject<UClass>(ANY_PACKAGE, *ClassName);
		if (FoundClass)
		{
			CachedUProperty = FindField<UProperty>(FoundClass, *ThisPropertyName);
		}
	}

	return CachedUProperty;
}

void FAttributeModifierTest::ApplyModifier(UAttributeSet *Object)
{
	UProperty *TheProperty = GetUProperty();
	if (TheProperty)
	{
		void * ValuePtr = TheProperty->ContainerPtrToValuePtr<void>(Object);
		switch( ModifierType)
		{
			case EAttributeModifierType::EATTRMOD_Add:
			{
				UNumericProperty *NumericProperty = Cast<UNumericProperty>(TheProperty);
				if (NumericProperty)
				{
					float CurrentValue = NumericProperty->GetFloatingPointPropertyValue(ValuePtr);
					float AdditionalValue = 0.f;
					TTypeFromString<float>::FromString(AdditionalValue, *this->NewValue);
					
					float FinalValue = CurrentValue + AdditionalValue;
					NumericProperty->SetFloatingPointPropertyValue(ValuePtr, FinalValue);

					UE_LOG(LogActorComponent, Warning, TEXT("FAttributeModifierTest::ApplyModifier - %s. EATTRMOD_Add - CurrentValue: %.2f AdditionalValue: %.2f -> FinalValue: %.2f"), *TheProperty->GetName(),
						CurrentValue, AdditionalValue, FinalValue );
				}
			}
			break;

			case EAttributeModifierType::EATTRMOD_Multiple:
			{
				UNumericProperty *NumericProperty = Cast<UNumericProperty>(TheProperty);
				if (NumericProperty)
				{
					float CurrentValue = NumericProperty->GetFloatingPointPropertyValue(ValuePtr);
					float Factor = 0.f;
					TTypeFromString<float>::FromString(Factor, *this->NewValue);

					float FinalValue = CurrentValue * Factor;
					NumericProperty->SetFloatingPointPropertyValue(ValuePtr, FinalValue);

					UE_LOG(LogActorComponent, Warning, TEXT("FAttributeModifierTest::ApplyModifier - %s. EATTRMOD_Multiple - CurrentValue: %.2f Factor: %.2f -> FinalValue: %.2f"), *TheProperty->GetName(),
						CurrentValue, Factor, FinalValue );
				}
			}
			break;

			case EAttributeModifierType::EATTRMOD_Override:
				TheProperty->ImportText(*this->NewValue, ValuePtr, 0, Object); // Not sure if the 4th parameter is right

				UE_LOG(LogActorComponent, Warning, TEXT("FAttributeModifierTest::ApplyModifier - %s. EATTRMOD_Override - %s"), *TheProperty->GetName(), *this->NewValue );
				break;
		}
	}

	Object->PrintDebug();
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
