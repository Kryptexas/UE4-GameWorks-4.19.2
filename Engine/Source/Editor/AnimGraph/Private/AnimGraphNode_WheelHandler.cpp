// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"
#include "AnimGraphNode_WheelHandler.h"
#include "CompilerResultsLog.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_WheelHandler

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_WheelHandler::UAnimGraphNode_WheelHandler(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FText UAnimGraphNode_WheelHandler::GetControllerDescription() const
{
	return LOCTEXT("AnimGraphNode_WheelHandler", "Wheel Handler for WheeledVehicle");
}

FString UAnimGraphNode_WheelHandler::GetKeywords() const
{
	return TEXT("Modify, Wheel, Vehicle");
}

FString UAnimGraphNode_WheelHandler::GetTooltip() const
{
	return LOCTEXT("AnimGraphNode_WheelHandler_Tooltip", "This alters the wheel transform based on set up in Wheeled Vehicle. This only works when the owner is WheeledVehicle.").ToString();
}

FText UAnimGraphNode_WheelHandler::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	// we don't have any run-time information, so it's limited to print anymore than whaat it is
	// it would be nice to print more data such as name of bones for wheels, but it's not available in Persona
	return FText(LOCTEXT("AnimGraphNode_WheelHandler_Title", "Wheel Handler"));
}

void UAnimGraphNode_WheelHandler::ValidateAnimNodePostCompile(class FCompilerResultsLog& MessageLog, class UAnimBlueprintGeneratedClass* CompiledClass, int32 CompiledNodeIndex)
{
	// we only support vehicle anim instance
	if ( CompiledClass->IsChildOf(UVehicleAnimInstance::StaticClass())  == false )
	{
		MessageLog.Error(TEXT("@@ is only allowwed in VehicleAnimInstance. If this is for vehicle, please change parent to be VehicleAnimInstancen (Reparent Class)."), this);
	}
}

void UAnimGraphNode_WheelHandler::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	UAnimBlueprint* Blueprint = Cast<UAnimBlueprint> (FBlueprintEditorUtils::FindBlueprintForGraph(ContextMenuBuilder.CurrentGraph));

	if (Blueprint &&  Blueprint->ParentClass->IsChildOf(UVehicleAnimInstance::StaticClass()))
	{
		Super::GetMenuEntries( ContextMenuBuilder );
	}

	// else we don't show
}

#undef LOCTEXT_NAMESPACE
