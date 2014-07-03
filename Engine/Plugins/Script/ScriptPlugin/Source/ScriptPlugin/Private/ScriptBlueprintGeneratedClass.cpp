// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "ScriptPluginPrivatePCH.h"
#include "ScriptBlueprintGeneratedClass.h"
#if WITH_LUA
#include "LuaIntegration.h"
#endif

/////////////////////////////////////////////////////
// UScriptBlueprintGeneratedClass

UScriptBlueprintGeneratedClass::UScriptBlueprintGeneratedClass(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UScriptBlueprintGeneratedClass::PostInitProperties()
{
	Super::PostInitProperties();
}

void UScriptBlueprintGeneratedClass::Link(FArchive& Ar, bool bRelinkExistingProperties)
{
	Super::Link(Ar, bRelinkExistingProperties);
}

FScriptContextBase* UScriptBlueprintGeneratedClass::CreateContext()
{
	FScriptContextBase* NewContext = NULL;
#if WITH_LUA
	NewContext = new FLuaContext();
#endif
	return NewContext;
}

void UScriptBlueprintGeneratedClass::AddUniqueNativeFunction(const FName& InName, Native InPointer)
{
	// Find the function in the class's native function lookup table.
	for (int32 FunctionIndex = 0; FunctionIndex < NativeFunctionLookupTable.Num(); ++FunctionIndex)
	{
		FNativeFunctionLookup& NativeFunctionLookup = NativeFunctionLookupTable[FunctionIndex];
		if (NativeFunctionLookup.Name == InName)
		{
			return;
		}
	}
	new(NativeFunctionLookupTable)FNativeFunctionLookup(InName, InPointer);
}

void UScriptBlueprintGeneratedClass::RemoveNativeFunction(const FName& InName)
{
	// Find the function in the class's native function lookup table.
	for (int32 FunctionIndex = 0; FunctionIndex < NativeFunctionLookupTable.Num(); ++FunctionIndex)
	{
		FNativeFunctionLookup& NativeFunctionLookup = NativeFunctionLookupTable[FunctionIndex];
		if (NativeFunctionLookup.Name == InName)
		{
			NativeFunctionLookupTable.RemoveAt(FunctionIndex);
			return;
		}
	}
}