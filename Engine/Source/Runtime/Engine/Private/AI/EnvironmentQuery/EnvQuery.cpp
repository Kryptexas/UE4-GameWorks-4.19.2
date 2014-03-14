// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UEnvQuery::UEnvQuery(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

static bool HasNamedValue(const FName& ParamName, const TArray<FEnvNamedValue>& NamedValues)
{
	for (int32 i = 0; i < NamedValues.Num(); i++)
	{
		if (NamedValues[i].ParamName == ParamName)
		{
			return true;
		}
	}

	return false;
}

static void AddNamedValue(const FName& ParamName, const EEnvQueryParam::Type& ParamType, float Value,
						  TArray<FEnvNamedValue>& NamedValues, TArray<FName>& RequiredParams)
{
	if (ParamName != NAME_None)
	{
		if (!HasNamedValue(ParamName, NamedValues))
		{
			FEnvNamedValue NewValue;
			NewValue.ParamName = ParamName;
			NewValue.ParamType = ParamType;
			NewValue.Value = Value;
			NamedValues.Add(NewValue);
		}

		RequiredParams.AddUnique(ParamName);
	}
}

#define GET_STRUCT_NAME_CHECKED(StructName) \
	((void)sizeof(StructName), TEXT(#StructName))

static void AddNamedValuesFromObject(const UObject* Ob, TArray<FEnvNamedValue>& NamedValues, TArray<FName>& RequiredParams)
{
	if (Ob == NULL)
		return;

	for (UProperty* TestProperty = Ob->GetClass()->PropertyLink; TestProperty; TestProperty = TestProperty->PropertyLinkNext)
	{
		UStructProperty* TestStruct = Cast<UStructProperty>(TestProperty);
		if (TestStruct == NULL)
		{
			continue;
		}

		FString TypeDesc = TestStruct->GetCPPType(NULL, CPPF_None);
		if (TypeDesc.Contains(GET_STRUCT_NAME_CHECKED(FEnvIntParam)))
		{
			const FEnvIntParam* PropertyValue = TestStruct->ContainerPtrToValuePtr<FEnvIntParam>(Ob);
			AddNamedValue(PropertyValue->ParamName, EEnvQueryParam::Int, *((float*)&PropertyValue->Value), NamedValues, RequiredParams);
		}
		else if (TypeDesc.Contains(GET_STRUCT_NAME_CHECKED(FEnvFloatParam)))
		{
			const FEnvFloatParam* PropertyValue = TestStruct->ContainerPtrToValuePtr<FEnvFloatParam>(Ob);
			AddNamedValue(PropertyValue->ParamName, EEnvQueryParam::Float, PropertyValue->Value, NamedValues, RequiredParams);
		}
		else if (TypeDesc.Contains(GET_STRUCT_NAME_CHECKED(FEnvBoolParam)))
		{
			const FEnvBoolParam* PropertyValue = TestStruct->ContainerPtrToValuePtr<FEnvBoolParam>(Ob);
			AddNamedValue(PropertyValue->ParamName, EEnvQueryParam::Bool, PropertyValue->Value ? 1.0f : -1.0f, NamedValues, RequiredParams);
		}
	}
}

#undef GET_STRUCT_NAME_CHECKED

void UEnvQuery::CollectQueryParams(TArray<FEnvNamedValue>& NamedValues) const
{
	TArray<FName> RequiredParams;

	// collect all params
	for (int32 iOption = 0; iOption < Options.Num(); iOption++)
	{
		const UEnvQueryOption* Option = Options[iOption];
		AddNamedValuesFromObject(Option->Generator, NamedValues, RequiredParams);

		for (int32 iTest = 0; iTest < Option->Tests.Num(); iTest++)
		{
			const UEnvQueryTest* TestOb = Option->Tests[iTest];
			AddNamedValuesFromObject(TestOb, NamedValues, RequiredParams);
		}
	}

	// remove unnecessary params
	for (int32 i = NamedValues.Num() - 1; i >= 0; i--)
	{
		if (!RequiredParams.Contains(NamedValues[i].ParamName))
		{
			NamedValues.RemoveAt(i);
		}
	}
}
