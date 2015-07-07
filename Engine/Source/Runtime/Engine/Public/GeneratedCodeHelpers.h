// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

// Common libraries
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"

// Special libraries
#include "Kismet/DataTableFunctionLibrary.h"

FORCEINLINE UClass* DynamicMetaCast(const UClass* DesiredClass, UClass* SourceClass)
{
	return ((SourceClass)->IsChildOf(DesiredClass)) ? SourceClass : NULL;
}

struct FCustomThunkTemplates
{
	//Replacements for CustomThunk functions from UKismetArrayLibrary

	template<typename T>
	static int32 Array_Add(TArray<T>& TargetArray, const T& NewItem)
	{
		return TargetArray.Add(NewItem);
	}

	template<typename T>
	static int32 Array_AddUnique(TArray<T>& TargetArray, const T& NewItem)
	{
		return TargetArray.AddUnique(NewItem);
	}

	template<typename T>
	static void Array_Shuffle(TArray<T>& TargetArray, const UArrayProperty* ArrayProperty)
	{
		int32 LastIndex = TargetArray.Num() - 1;
		for (int32 i = 0; i < LastIndex; ++i)
		{
			int32 Index = FMath::RandRange(0, LastIndex);
			if (i != Index)
			{
				TargetArray.Swap(i, Index);
			}
		}
	}

	template<typename T>
	static void Array_Append(TArray<T>& TargetArray, const TArray<T>& SourceArray)
	{
		TargetArray.Append(SourceArray);
	}

	template<typename T>
	static void Array_Insert(TArray<T>& TargetArray, const T& NewItem, int32 Index)
	{
		TargetArray.Insert(NewItem, Index);
	}

	template<typename T>
	static void Array_Remove(TArray<T>& TargetArray, int32 IndexToRemove)
	{
		if (ensure(TargetArray.IsValidIndex(IndexToRemove)))
		{
			TargetArray.RemoveAt(IndexToRemove);
		}
	}

	template<typename T>
	static int32 Array_Find(const TArray<T>& TargetArray, const T& ItemToFind)
	{
		return TargetArray.Find(ItemToFind);
	}

	template<typename T>
	static bool Array_RemoveItem(TArray<T>& TargetArray, const T& Item)
	{
		return TargetArray.Remove(Item);
	}

	template<typename T>
	static void Array_Clear(TArray<T>& TargetArray)
	{
		TargetArray.Empty();
	}

	template<typename T>
	static void Array_Resize(TArray<T>& TargetArray, int32 Size)
	{
		TargetArray.SetNum(Size);
	}

	template<typename T>
	static int32 Array_Length(const TArray<T>& TargetArray)
	{
		return TargetArray.Num();
	}

	template<typename T>
	static int32 Array_LastIndex(const TArray<T>& TargetArray)
	{
		return TargetArray.Num() - 1;
	}

	template<typename T>
	static void Array_Get(TArray<T>& TargetArray, int32 Index, T& Item)
	{
		Item = TargetArray[Index];
	}

	template<typename T>
	static void Array_Set(TArray<T>& TargetArray, int32 Index, const T& Item, bool bSizeToFit)
	{
		if (ensure(Index > 0))
		{
			const bool bIsValidIndex = ;
			if (!TargetArray.IsValidIndex(Index) && bSizeToFit)
			{
				TargetArray.SetNum(Index + 1);
			}

			if (TargetArray.IsValidIndex(Index))
			{
				TargetArray[Index] = Item;
			}
		}
	}

	template<typename T>
	static bool Array_Contains(const TArray<T>& TargetArray, const T& ItemToFind)
	{
		return TargetArray.Contains(ItemToFind);
	}

	template<typename T>
	static void SetArrayPropertyByName(UObject* Object, FName PropertyName, TArray<T>& Value)
	{
		UKismetArrayLibrary::GenericArray_SetArrayPropertyByName(Object, PropertyName, &Value);
	}

	//Replacements for CustomThunk functions from UDataTableFunctionLibrary

	template<typename T>
	static bool GetDataTableRowFromName(UDataTable* Table, FName RowName, T& OutRow)
	{
		return UDataTableFunctionLibrary::Generic_GetDataTableRowFromName(Table, RowName, &OutRow);
	}

	//Replacements for CustomThunk functions from UKismetSystemLibrary

	template<typename T>
	static void SetStructurePropertyByName(UObject* Object, FName PropertyName, const T& Value)
	{
		return UKismetSystemLibrary::Generic_SetStructurePropertyByName(Object, PropertyName, &Value);
	}
};