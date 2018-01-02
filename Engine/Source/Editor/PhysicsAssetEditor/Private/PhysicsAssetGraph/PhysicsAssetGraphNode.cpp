// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PhysicsAssetGraphNode.h"
#include "PhysicsAssetGraph.h"
#include "EdGraph/EdGraphPin.h"

UPhysicsAssetGraphNode::UPhysicsAssetGraphNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputPin = nullptr;
	OutputPin = nullptr;
}

void UPhysicsAssetGraphNode::SetupPhysicsAssetNode()
{

}

UObject* UPhysicsAssetGraphNode::GetDetailsObject()
{
	return nullptr;
}

UPhysicsAssetGraph* UPhysicsAssetGraphNode::GetPhysicsAssetGraph() const
{
	return CastChecked<UPhysicsAssetGraph>(GetOuter());
}

FText UPhysicsAssetGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NodeTitle;
}

void UPhysicsAssetGraphNode::AllocateDefaultPins()
{
	InputPin = CreatePin(EEdGraphPinDirection::EGPD_Input, NAME_None, NAME_None, NAME_None);
	InputPin->bHidden = true;
	OutputPin = CreatePin(EEdGraphPinDirection::EGPD_Output, NAME_None, NAME_None, NAME_None);
	OutputPin->bHidden = true;
}

UEdGraphPin& UPhysicsAssetGraphNode::GetInputPin() const
{
	return *InputPin;
}

UEdGraphPin& UPhysicsAssetGraphNode::GetOutputPin() const
{
	return *OutputPin;
}
