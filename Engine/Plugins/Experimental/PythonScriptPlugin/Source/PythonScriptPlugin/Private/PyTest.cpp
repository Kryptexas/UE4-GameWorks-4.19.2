// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PyTest.h"
#include "PyUtil.h"

FPyTestStruct::FPyTestStruct()
{
	Bool = false;
	Int = 0;
	Float = 0.0f;
	Enum = EPyTestEnum::One;
}

UPyTestObject::UPyTestObject()
{
	StructArray.AddDefaulted();
	StructArray.AddDefaulted();
}

int32 UPyTestObject::FuncBlueprintNative_Implementation(const int32 InValue) const
{
	return InValue;
}

int32 UPyTestObject::CallFuncBlueprintImplementable(const int32 InValue) const
{
	return FuncBlueprintImplementable(InValue);
}

int32 UPyTestObject::CallFuncBlueprintNative(const int32 InValue) const
{
	return FuncBlueprintNative(InValue);
}

void UPyTestObject::FuncTakingPyTestStruct(const FPyTestStruct& InStruct) const
{
}

void UPyTestObject::FuncTakingPyTestChildStruct(const FPyTestChildStruct& InStruct) const
{
}

int32 UPyTestObject::FuncTakingPyTestDelegate(const FPyTestDelegate& InDelegate, const int32 InValue) const
{
	return InDelegate.IsBound() ? InDelegate.Execute(InValue) : INDEX_NONE;
}

int32 UPyTestObject::DelegatePropertyCallback(const int32 InValue) const
{
	if (InValue != Int)
	{
		UE_LOG(LogPython, Error, TEXT("Given value (%d) did not match the Int property value (%d)"), InValue, Int);
	}

	return InValue;
}

void UPyTestObject::MulticastDelegatePropertyCallback(FString InStr) const
{
	if (InStr != String)
	{
		UE_LOG(LogPython, Error, TEXT("Given value (%s) did not match the String property value (%s)"), *InStr, *String);
	}
}
