// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BlueprintProfilerPCH.h"

//////////////////////////////////////////////////////////////////////////
// FScriptInstrumentedEvent

void FScriptInstrumentedEvent::SetData(EScriptInstrumentation::Type InEventType, const FString& InPathData, const int32 InCodeOffset)
{
	EventType = InEventType;
	PathData = InPathData;
	CodeOffset = InCodeOffset;
	Time = FPlatformTime::Seconds();
}

FName FScriptInstrumentedEvent::GetScopedFunctionName() const
{
	return FName(*FString::Printf(TEXT("%s::%s"), *FunctionClassScopeName.ToString(), *FunctionName.ToString()));
}

//////////////////////////////////////////////////////////////////////////
// FInstrumentationCaptureContext

bool FInstrumentationCaptureContext::UpdateContext(const FScriptInstrumentationSignal& InstrumentSignal, TArray<FScriptInstrumentedEvent>& InstrumentationQueue)
{
	if (InstrumentSignal.GetType() == EScriptInstrumentation::InlineEvent)
	{
		// This inline event needs to be processed using the next incoming event's function class scope,
		// so we need to store the data until the following UpdateContext call
		PendingInlineEventName = InstrumentSignal.GetFunctionName();
		return false;
	}
	else if (PendingInlineEventName != NAME_None)
	{
		// The pending inline event needs to be added first to make sure our events are processed in the proper order.
		InstrumentationQueue.Add(FScriptInstrumentedEvent(EScriptInstrumentation::Event, InstrumentSignal.GetFunctionClassScope()->GetFName(), PendingInlineEventName));
		PendingInlineEventName = NAME_None;
	}

	// Handle instance context switch
	if (const UObject* NewContextObject = InstrumentSignal.GetContextObject())
	{
		if (NewContextObject != ContextObject)
		{
			// Handle class context switch
			if (const UClass* NewClass = InstrumentSignal.GetClass())
			{
				if (NewClass != ContextClass)
				{
					InstrumentationQueue.Add(FScriptInstrumentedEvent(EScriptInstrumentation::Class, InstrumentSignal.GetFunctionClassScope()->GetFName(), NewClass->GetPathName()));
					ContextClass = NewClass;
				}
			}
			// Handle instance change
			InstrumentationQueue.Add(FScriptInstrumentedEvent(EScriptInstrumentation::Instance, ContextClass->GetFName(), NewContextObject->GetPathName()));
			ContextObject = NewContextObject;
		}
		else
		{
			TWeakObjectPtr<const UClass> SignalScope = InstrumentSignal.GetFunctionClassScope();
			// Handle function class scope changes
			if (ContextFunctionClassScope != SignalScope)
			{
				FName ClassScopeName = SignalScope->GetFName();
				if (InstrumentSignal.GetType() == EScriptInstrumentation::Stop)
				{
					// In a Stop event, we need to use the last class scope on the stack to signify
					// that we are reverting back to the previous class scope
					ClassScopeName = ScopeStack.Pop();
				}
				else
				{
					// Stop events cause the Context to be reset so we need to check against the top scope on the stack before pushing it to the stack
					if (ScopeStack.Num() == 0 || ScopeStack.Last() != ClassScopeName)
					{
						ScopeStack.Push(ClassScopeName);
					}
					ContextFunctionClassScope = SignalScope;
				}

				InstrumentationQueue.Add(FScriptInstrumentedEvent(EScriptInstrumentation::ClassScope, ClassScopeName, InstrumentSignal.GetFunctionName()));
			}
		}
	}
	
	return true;
}

void FInstrumentationCaptureContext::ResetContext()
{
	ContextClass.Reset();
	ContextFunctionClassScope.Reset();
	ContextObject.Reset();
}