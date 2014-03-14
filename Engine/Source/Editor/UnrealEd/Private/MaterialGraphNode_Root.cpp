// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MaterialGraphNode_Root.cpp
=============================================================================*/

#include "UnrealEd.h"
#include "MaterialEditorUtilities.h"
#include "GraphEditorActions.h"

#define LOCTEXT_NAMESPACE "MaterialGraphNode_Root"

/////////////////////////////////////////////////////
// UMaterialGraphNode_Root

UMaterialGraphNode_Root::UMaterialGraphNode_Root(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UMaterialGraphNode_Root::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FMaterialEditorUtilities::GetOriginalObjectName(this->GetGraph());
}

FLinearColor UMaterialGraphNode_Root::GetNodeTitleColor() const
{
	return FLinearColor(1.0f, 0.65f, 0.4f);
}

FString UMaterialGraphNode_Root::GetTooltip() const
{
	return LOCTEXT("RootToolTip", "Description of final material inputs").ToString();
}

void UMaterialGraphNode_Root::PostPlacedNewNode()
{
	if (Material)
	{
		NodePosX = Material->EditorX;
		NodePosY = Material->EditorY;
	}
}

void UMaterialGraphNode_Root::CreateInputPins()
{
	UMaterialGraph* MaterialGraph = CastChecked<UMaterialGraph>(GetGraph());
	const UMaterialGraphSchema* Schema = CastChecked<UMaterialGraphSchema>(GetSchema());

	for (int32 Index = 0; Index < MaterialGraph->MaterialInputs.Num(); ++Index)
	{
		UEdGraphPin* InputPin = CreatePin(EGPD_Input, Schema->PC_MaterialInput, TEXT(""), NULL, /*bIsArray=*/ false, /*bIsReference=*/ false, MaterialGraph->MaterialInputs[Index].Name.ToString());
	}
}

int32 UMaterialGraphNode_Root::GetInputIndex(const UEdGraphPin* InputPin) const
{
	for (int32 Index = 0; Index < Pins.Num(); ++Index)
	{
		if (InputPin == Pins[Index])
		{
			return Index;
		}
	}

	return -1;
}

uint32 UMaterialGraphNode_Root::GetInputType(const UEdGraphPin* InputPin) const
{
	UMaterialGraph* MaterialGraph = CastChecked<UMaterialGraph>(GetGraph());
	return GetMaterialPropertyType(MaterialGraph->MaterialInputs[GetInputIndex(InputPin)].Property);
}

#undef LOCTEXT_NAMESPACE
