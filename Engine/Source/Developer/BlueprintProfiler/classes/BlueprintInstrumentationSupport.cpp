// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Private/BlueprintProfilerPCH.h"

//////////////////////////////////////////////////////////////////////////
// FBlueprintInstrumentedEvent

void FBlueprintInstrumentedEvent::SetData(EScriptInstrumentation::Type InEventType, const FString& InPathData, const int32 InCodeOffset)
{
	EventType = InEventType;
	PathData = InPathData;
	CodeOffset = InCodeOffset;
	Time = FPlatformTime::Seconds();
}

FName FBlueprintInstrumentedEvent::GetFunctionName() const
{
	FName Result = NAME_None;
	int32 FunctionStart = INDEX_NONE;
	if (PathData.FindLastChar(':', FunctionStart))
	{
		FunctionStart++;
		Result = FName(*PathData.Right(PathData.Len() - FunctionStart));
	}
	return Result;
}

//////////////////////////////////////////////////////////////////////////
// FKismetInstrumentContext

void FKismetInstrumentContext::UpdateContext(const UObject* InContextObject, TArray<FBlueprintInstrumentedEvent>& ProfilingData)
{
	// Handle instance context switch
	if (ContextObject != InContextObject && InContextObject)
	{
		// Handle class context switch
		if (const UClass* NewClass = InContextObject->GetClass())
		{
			if (NewClass != ContextClass)
			{
				ProfilingData.Add(FBlueprintInstrumentedEvent(EScriptInstrumentation::Class, NewClass->GetPathName(), -1));
				ContextClass = NewClass;
			}
		}
		// Handle instance change
		ProfilingData.Add(FBlueprintInstrumentedEvent(EScriptInstrumentation::Instance, InContextObject->GetPathName(), -1));
		ContextObject = InContextObject;
	}
}

void FKismetInstrumentContext::ResetContext()
{
	ContextClass.Reset();
	ContextObject.Reset();
}
