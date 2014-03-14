// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "BlueprintUtilities.h"

UBreakpoint::UBreakpoint(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bEnabled = false;
	bStepOnce = false;
	bStepOnce_WasPreviouslyDisabled = false;
	bStepOnce_RemoveAfterHit = false;
}

FString UBreakpoint::GetLocationDescription() const
{
#if WITH_EDITORONLY_DATA
	if (Node != NULL)
	{
		FString Result
#if WITH_EDITOR
			= Node->GetDescriptiveCompiledName()
#endif
			;
		if (!Node->NodeComment.IsEmpty())
		{
			Result += TEXT(" // ");
			Result += Node->NodeComment;
		}

		return Result;
	}
	else
	{
		return TEXT("Error: Invalid location");
	}
#else	//#if WITH_EDITORONLY_DATA
	return TEXT("--- NO EDITOR DATA! ---");
#endif	//#if WITH_EDITORONLY_DATA
}
