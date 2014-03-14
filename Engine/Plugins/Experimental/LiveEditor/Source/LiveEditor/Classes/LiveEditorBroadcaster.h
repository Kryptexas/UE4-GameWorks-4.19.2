// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "LiveEditorBroadcaster.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams( FLiveEditorEventMIDIMultiCast, int32, RawDeltaMIDI, ELiveEditControllerType::Type, ControlType );

UCLASS(DependsOn=ULiveEditorTypes,MinimalAPI)
class ULiveEditorBroadcaster : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintAssignable)
	FLiveEditorEventMIDIMultiCast OnEventMIDI;
};
