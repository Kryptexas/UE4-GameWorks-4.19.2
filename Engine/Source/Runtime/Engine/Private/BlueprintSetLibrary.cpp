// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Blueprint/BlueprintSupport.h"
#include "Kismet/BlueprintSetLibrary.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Templates/HasGetTypeHash.h"

UBlueprintSetLibrary::UBlueprintSetLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UBlueprintSetLibrary::GenericSet_Add(const void* TargetSet, const USetProperty* SetProperty, const void* ItemPtr)
{
	if (TargetSet)
	{
		FScriptSetHelper SetHelper(SetProperty, TargetSet);
		SetHelper.AddElement(ItemPtr);
	}
}

void UBlueprintSetLibrary::GenericSet_AddItems(const void* TargetSet, const USetProperty* SetProperty, const void* TargetArray, const UArrayProperty* ArrayProperty)
{
	if(TargetSet && TargetArray)
	{
		FScriptArrayHelper ArrayHelper(ArrayProperty, TargetArray);

		const int32 Size = ArrayHelper.Num();
		for (int32 I = 0; I < Size; ++I)
		{
			GenericSet_Add(TargetSet, SetProperty, ArrayHelper.GetRawPtr(I));
		}
	}
}

void UBlueprintSetLibrary::GenericSet_Remove(const void* TargetSet, const USetProperty* SetProperty, const void* ItemPtr)
{
	if (TargetSet)
	{
		FScriptSetHelper SetHelper(SetProperty, TargetSet);
		SetHelper.RemoveElement(ItemPtr);
	}
}

void UBlueprintSetLibrary::GenericSet_RemoveItems(const void* TargetSet, const USetProperty* SetProperty, const void* TargetArray, const UArrayProperty* ArrayProperty)
{
	if (TargetSet && TargetArray)
	{
		FScriptArrayHelper ArrayHelper(ArrayProperty, TargetArray);

		const int32 Size = ArrayHelper.Num();
		for (int32 I = 0; I < Size; ++I)
		{
			GenericSet_Remove(TargetSet, SetProperty, ArrayHelper.GetRawPtr(I));
		}
	}
}

void UBlueprintSetLibrary::GenericSet_ToArray(const void* TargetSet, const USetProperty* SetProperty, void* TargetArray, const UArrayProperty* ArrayProperty)
{
	if (TargetSet && TargetArray)
	{
		FScriptSetHelper SetHelper(SetProperty, TargetSet);

		const int32 Size = SetHelper.Num();
		for (int32 I = 0; I < Size; ++I)
		{
			UKismetArrayLibrary::GenericArray_Add(TargetArray, ArrayProperty, SetHelper.GetElementPtr(I));
		}
	}
}

void UBlueprintSetLibrary::GenericSet_Clear(const void* TargetSet, const USetProperty* SetProperty)
{
	if (TargetSet)
	{
		FScriptSetHelper SetHelper(SetProperty, TargetSet);
		SetHelper.EmptyElements();
	}
}

int32 UBlueprintSetLibrary::GenericSet_Length(const void* TargetSet, const USetProperty* SetProperty)
{
	if (TargetSet)
	{
		FScriptSetHelper SetHelper(SetProperty, TargetSet);
		return SetHelper.Num();
	}

	return 0;
}

bool UBlueprintSetLibrary::GenericSet_Contains(const void* TargetSet, const USetProperty* SetProperty, const void* ItemToFind)
{
	if (TargetSet)
	{
		FScriptSetHelper SetHelper(SetProperty, TargetSet);
		UProperty* ElementProp = SetProperty->ElementProp;

		return SetHelper.FindElementFromHash(ItemToFind) != nullptr;
	}

	return false;
}

void UBlueprintSetLibrary::GenericSet_Intersect(const void* SetA, const USetProperty* SetPropertyA, const void* SetB, const USetProperty* SetPropertyB, const void* SetResult, const USetProperty* SetPropertyResult)
{
	if (SetA && SetB && SetResult)
	{
		FScriptSetHelper SetHelperA(SetPropertyA, SetA);
		FScriptSetHelper SetHelperB(SetPropertyB, SetB);
		FScriptSetHelper SetHelperResult(SetPropertyResult, SetResult);

		const int32 Size = SetHelperA.Num();
		for (int32 I = 0; I < Size; ++I)
		{
			const void* EntryInA = SetHelperA.GetElementPtr(I);
			if (SetHelperB.FindElementFromHash(EntryInA) != nullptr)
			{
				SetHelperResult.AddElement(EntryInA);
			}
		}
	}
}

void UBlueprintSetLibrary::GenericSet_Union(const void* SetA, const USetProperty* SetPropertyA, const void* SetB, const USetProperty* SetPropertyB, const void* SetResult, const USetProperty* SetPropertyResult)
{
	if (SetA && SetB && SetResult)
	{
		FScriptSetHelper SetHelperA(SetPropertyA, SetA);
		FScriptSetHelper SetHelperB(SetPropertyB, SetB);
		FScriptSetHelper SetHelperResult(SetPropertyResult, SetResult);

		const int32 SizeA = SetHelperA.Num();
		for (int32 I = 0; I < SizeA; ++I)
		{
			SetHelperResult.AddElement(SetHelperA.GetElementPtr(I));
		}

		const int32 SizeB = SetHelperB.Num();
		for (int32 I = 0; I < SizeB; ++I)
		{
			SetHelperResult.AddElement(SetHelperB.GetElementPtr(I));
		}
	}
}

void UBlueprintSetLibrary::GenericSet_Difference(const void* SetA, const USetProperty* SetPropertyA, const void* SetB, const USetProperty* SetPropertyB, const void* SetResult, const USetProperty* SetPropertyResult)
{
	if (SetA && SetB && SetResult)
	{
		FScriptSetHelper SetHelperA(SetPropertyA, SetA);
		FScriptSetHelper SetHelperB(SetPropertyB, SetB);
		FScriptSetHelper SetHelperResult(SetPropertyResult, SetResult);

		const int32 Size = SetHelperA.Num();
		for (int32 I = 0; I < Size; ++I)
		{
			const void* EntryInA = SetHelperA.GetElementPtr(I);
			if (SetHelperB.FindElementFromHash(EntryInA) == nullptr)
			{
				SetHelperResult.AddElement(EntryInA);
			}
		}
	}
}

