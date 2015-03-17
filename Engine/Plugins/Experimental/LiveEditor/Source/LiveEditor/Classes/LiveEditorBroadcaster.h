// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LiveEditorTypes.h"

#include "LiveEditorBroadcaster.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams( FLiveEditorEventMIDIMultiCast, int32, Delta, int32, MidiValue, ELiveEditControllerType::Type, ControlType );

UCLASS(MinimalAPI)
class ULiveEditorBroadcaster : public UObject
{
	GENERATED_BODY()
public:
	LIVEEDITOR_API ULiveEditorBroadcaster(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(BlueprintAssignable)
	FLiveEditorEventMIDIMultiCast OnEventMIDI;
};
