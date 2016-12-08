// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Kismet/BlueprintAsyncActionBase.h"

//////////////////////////////////////////////////////////////////////////
// UBlueprintAsyncActionBase

UBlueprintAsyncActionBase::UBlueprintAsyncActionBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetFlags(RF_StrongRefOnFrame);
}

void UBlueprintAsyncActionBase::Activate()
{
}
