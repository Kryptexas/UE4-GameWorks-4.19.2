// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GraphEditorSettings.h: Declares the UGraphEditorSettings class.
=============================================================================*/

#pragma once

#include "GraphEditorSettings.generated.h"


/**
 * Implements settings for the graph editor.
 */
UCLASS(config=EditorUserSettings)
class GRAPHEDITOR_API UGraphEditorSettings
	:	public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/** The visual styling to use for graph editor pins (in Blueprints, materials, etc...) */
	UPROPERTY(config, EditAnywhere, Category=Blueprint)
	TEnumAsByte<EBlueprintPinStyleType> DataPinStyle;

public:

	/** The default color is used only for types not specifically defined below.  Generally if it's seen, it means another type needs to be defined so that the wire in question can have an appropriate color. */
	UPROPERTY(EditAnywhere,  config, Category=PinColors)
	FLinearColor DefaultPinTypeColor;

	/** Execution pin type color */
	UPROPERTY(EditAnywhere,  config, Category=PinColors)
	FLinearColor ExecutionPinTypeColor;

	/** Boolean pin type color */
	UPROPERTY(EditAnywhere,  config, Category=PinColors)
	FLinearColor BooleanPinTypeColor;

	/** Byte pin type color */
	UPROPERTY(EditAnywhere,  config, Category=PinColors)
	FLinearColor BytePinTypeColor;

	/** Class pin type color */
	UPROPERTY(EditAnywhere,  config, Category=PinColors)
	FLinearColor ClassPinTypeColor;

	/** Integer pin type color */
	UPROPERTY(EditAnywhere,  config, Category=PinColors)
	FLinearColor IntPinTypeColor;

	/** Floating-point pin type color */
	UPROPERTY(EditAnywhere,  config, Category=PinColors)
	FLinearColor FloatPinTypeColor;

	/** Name pin type color */
	UPROPERTY(EditAnywhere,  config, Category=PinColors)
	FLinearColor NamePinTypeColor;

	/** Delegate pin type color */
	UPROPERTY(EditAnywhere,  config, Category=PinColors)
	FLinearColor DelegatePinTypeColor;

	/** Object pin type color */
	UPROPERTY(EditAnywhere,  config, Category=PinColors)
	FLinearColor ObjectPinTypeColor;

	/** Interface pin type color */
	UPROPERTY(EditAnywhere,  config, Category=PinColors)
	FLinearColor InterfacePinTypeColor;

	/** String pin type color */
	UPROPERTY(EditAnywhere,  config, Category=PinColors)
	FLinearColor StringPinTypeColor;

	/** Text pin type color */
	UPROPERTY(EditAnywhere,  config, Category=PinColors)
	FLinearColor TextPinTypeColor;

	/** Struct pin type color */
	UPROPERTY(EditAnywhere,  config, Category=PinColors)
	FLinearColor StructPinTypeColor;

	/** Wildcard pin type color */
	UPROPERTY(EditAnywhere,  config, Category=PinColors)
	FLinearColor WildcardPinTypeColor;

	/** Vector pin type color */
	UPROPERTY(EditAnywhere,  config, Category=PinColors)
	FLinearColor VectorPinTypeColor;

	/** Rotator pin type color */
	UPROPERTY(EditAnywhere,  config, Category=PinColors)
	FLinearColor RotatorPinTypeColor;

	/** Transform pin type color */
	UPROPERTY(EditAnywhere,  config, Category=PinColors)
	FLinearColor TransformPinTypeColor;

	/** Index pin type color */
	UPROPERTY(EditAnywhere,  config, Category=PinColors)
	FLinearColor IndexPinTypeColor;

public:

	/** Event node title color */
	UPROPERTY(EditAnywhere,  config, Category=NodeTitleColors)
	FLinearColor EventNodeTitleColor;

	/** CallFunction node title color */
	UPROPERTY(EditAnywhere,  config, Category=NodeTitleColors)
	FLinearColor FunctionCallNodeTitleColor;

	/** Pure function call node title color */
	UPROPERTY(EditAnywhere,  config, Category=NodeTitleColors)
	FLinearColor PureFunctionCallNodeTitleColor;

	/** Parent class function call node title color */
	UPROPERTY(EditAnywhere,  config, Category=NodeTitleColors)
	FLinearColor ParentFunctionCallNodeTitleColor;

	/** Function Terminator node title color */
	UPROPERTY(EditAnywhere,  config, Category=NodeTitleColors)
	FLinearColor FunctionTerminatorNodeTitleColor;

	/** Exec Branch node title color */
	UPROPERTY(EditAnywhere,  config, Category=NodeTitleColors)
	FLinearColor ExecBranchNodeTitleColor;

	/** Exec Sequence node title color */
	UPROPERTY(EditAnywhere,  config, Category=NodeTitleColors)
	FLinearColor ExecSequenceNodeTitleColor;

	/** Result node title color */
	UPROPERTY(EditAnywhere,  config, Category=NodeTitleColors)
	FLinearColor ResultNodeTitleColor;

public:

	UPROPERTY(EditAnywhere,  config, Category=Tracing)
	FLinearColor TraceAttackColor;

	UPROPERTY(EditAnywhere,  config, Category=Tracing)
	float TraceAttackWireThickness;

	/** How long is the attack color fully visible */
	UPROPERTY()
	float TraceAttackHoldPeriod;

	/** How long does it take to fade from the attack to the sustain color */
	UPROPERTY()
	float TraceDecayPeriod;

	UPROPERTY()
	float TraceDecayExponent;

	UPROPERTY(EditAnywhere,  config, Category=Tracing)
	FLinearColor TraceSustainColor;

	UPROPERTY(EditAnywhere,  config, Category=Tracing)
	float TraceSustainWireThickness;

	/** How long is the sustain color fully visible */
	UPROPERTY()
	float TraceSustainHoldPeriod;

	UPROPERTY(EditAnywhere,  config, Category=Tracing)
	FLinearColor TraceReleaseColor;

	UPROPERTY(EditAnywhere,  config, Category=Tracing)
	float TraceReleaseWireThickness;

	/** How long does it take to fade from the sustain to the release color */
	UPROPERTY()
	float TraceReleasePeriod;

	UPROPERTY()
	float TraceReleaseExponent;

	/** How much of a bonus does an exec get for being near the top of the trace stack, and how does that fall off with position? */
	UPROPERTY()
	float TracePositionBonusPeriod;

	UPROPERTY()
	float TracePositionExponent;
};
