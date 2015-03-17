// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "Components/InputComponent.h"
#include "K2Node_Event.h"
#include "K2Node_InputKeyEvent.generated.h"

UCLASS(MinimalAPI)
class UK2Node_InputKeyEvent : public UK2Node_Event
{
	GENERATED_BODY()
public:
	BLUEPRINTGRAPH_API UK2Node_InputKeyEvent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY()
	FInputChord InputChord;

	UPROPERTY()
	TEnumAsByte<EInputEvent> InputKeyEvent;

	UPROPERTY()
	uint32 bConsumeInput:1;

	UPROPERTY()
	uint32 bExecuteWhenPaused:1;

	UPROPERTY()
	uint32 bOverrideParentBinding:1;

	// Begin UK2Node interface
	virtual UClass* GetDynamicBindingClass() const override;
	virtual void RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const override;
	// End UK2Node interface
};
