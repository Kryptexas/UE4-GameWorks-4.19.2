// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

// Common libraries
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"

// Special libraries
#include "Kismet/DataTableFunctionLibrary.h"

#include "Stack.h"

FORCEINLINE UClass* DynamicMetaCast(const UClass* DesiredClass, UClass* SourceClass)
{
	return ((SourceClass)->IsChildOf(DesiredClass)) ? SourceClass : NULL;
}

FORCEINLINE bool IsValid(const FScriptInterface& Test)
{
	return IsValid(Test.GetObject()) && (nullptr != Test.GetInterface());
}

struct FCustomThunkTemplates
{
private:
	static void ExecutionMessage(const TCHAR* Message, ELogVerbosity::Type Verbosity)
	{
		FFrame::KismetExecutionMessage(Message, Verbosity);
	}

	template<typename T>
	static int32 LastIndexForLog(const TArray<T>& TargetArray)
	{
		const int32 ArraySize = TargetArray.Num();
		return (ArraySize > 0) ? (ArraySize - 1) : 0;
	}

public:
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
		if ((Index >= 0) && (Index <= TargetArray.Num()))
		{
			TargetArray.Insert(NewItem, Index);
		}
		else
		{
			ExecutionMessage(*FString::Printf(TEXT("Attempted to insert an item into array out of bounds [%d/%d]!"), Index, LastIndexForLog(TargetArray)), ELogVerbosity::Warning);
		}
	}

	template<typename T>
	static void Array_Remove(TArray<T>& TargetArray, int32 IndexToRemove)
	{
		if (TargetArray.IsValidIndex(IndexToRemove))
		{
			TargetArray.RemoveAt(IndexToRemove);
		}
		else
		{
			ExecutionMessage(*FString::Printf(TEXT("Attempted to remove an item from an invalid index from array [%d/%d]!"), IndexToRemove, LastIndexForLog(TargetArray)), ELogVerbosity::Warning);
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
		if (Size >= 0)
		{
			TargetArray.SetNum(Size);
		}
		else
		{
			ExecutionMessage(*FString::Printf(TEXT("Attempted to resize an array using negative size: Size = %d!"), Size), ELogVerbosity::Warning);
		}
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
		if (TargetArray.IsValidIndex(Index))
		{
			Item = TargetArray[Index];
		}
		else
		{
			ExecutionMessage(*FString::Printf(TEXT("Attempted to get an item from array out of bounds [%d/%d]!"), Index, LastIndexForLog(TargetArray)), ELogVerbosity::Warning);
			Item = T{};
		}
	}

	template<typename T>
	static void Array_Set(TArray<T>& TargetArray, int32 Index, const T& Item, bool bSizeToFit)
	{
		if (!TargetArray.IsValidIndex(Index) && bSizeToFit && (Index >= 0))
		{
			TargetArray.SetNum(Index + 1);
		}

		if (TargetArray.IsValidIndex(Index))
		{
			TargetArray[Index] = Item;
		}
		else
		{
			ExecutionMessage(*FString::Printf(TEXT("Attempted to set an invalid index on array [%d/%d]!"), Index, LastIndexForLog(TargetArray)), ELogVerbosity::Warning);
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

template<typename IndexType, typename ValueType>
struct TSwitchPair
{
	const IndexType& IndexRef;
	ValueType& ValueRef;

	TSwitchPair(const IndexType& InIndexRef, ValueType& InValueRef)
		: IndexRef(InIndexRef)
		, ValueRef(InValueRef)
	{}
};

#if PLATFORM_COMPILER_HAS_VARIADIC_TEMPLATES
template<typename IndexType, typename ValueType>
ValueType& TSwitchValue(const IndexType& CurrentIndex, ValueType& DefaultValue, int OptionsNum)
{
	return DefaultValue;
}

template<typename IndexType, typename ValueType, typename Head, typename... Tail>
ValueType& TSwitchValue(const IndexType& CurrentIndex, ValueType& DefaultValue, int OptionsNum, Head HeadOption, Tail... TailOptions)
{
	if (CurrentIndex == HeadOption.IndexRef)
	{
		return HeadOption.ValueRef;
	}
	return TSwitchValue<IndexType, ValueType, Tail...>(CurrentIndex, DefaultValue, OptionsNum, TailOptions...);
}
#else //PLATFORM_COMPILER_HAS_VARIADIC_TEMPLATES
template<typename IndexType, typename ValueType>
ValueType& TSwitchValue(const IndexType& CurrentIndex, ValueType& DefaultValue, int OptionsNum, ...)
{
	typedef TSwitchPair < IndexType, ValueType > OptionType;

	ValueType* SelectedValuePtr = nullptr;

	va_list Options;
	va_start(Options, OptionsNum);
	for (int OptionIt = 0; OptionIt < OptionsNum; ++OptionIt)
	{
		OptionType Option = va_arg(Options, OptionType);
		if (Option.IndexRef == CurrentIndex)
		{
			SelectedValuePtr = &Option.ValueRef;
			break;
		}
	}
	va_end(Options);

	return SelectedValuePtr ? *SelectedValuePtr : DefaultValue;
}
#endif //PLATFORM_COMPILER_HAS_VARIADIC_TEMPLATES