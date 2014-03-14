// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "BlueprintGraphDefinitions.h"
#include "VectorVM.h"

UNiagaraNodeOp::UNiagaraNodeOp(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

const TCHAR* UNiagaraNodeOp::InPinNames[3] =
{
	TEXT("A"),
	TEXT("B"),
	TEXT("C")
};

void UNiagaraNodeOp::AllocateDefaultPins()
{
	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();
	VectorVM::FVectorVMOpInfo const& Info = VectorVM::GetOpCodeInfo(OpIndex);

	for (int32 SrcIndex = 0; SrcIndex < 3; ++SrcIndex)
	{
		VectorVM::EOpSrc::Type PinType = Info.SrcTypes[SrcIndex];
		if (PinType != VectorVM::EOpSrc::Invalid)
		{
			UEdGraphPin* Pin = CreatePin(EGPD_Input, Schema->PC_Float, TEXT(""), NULL, false, false, InPinNames[SrcIndex]);
			check(Pin);
			Pin->bDefaultValueIsIgnored = false;
			Pin->bNotConnectable = false;
			Pin->DefaultValue = TEXT("0.0");
		}
	}
	CreatePin(EGPD_Output, Schema->PC_Float, TEXT(""), NULL, false, false, TEXT("Result"));
}

FString UNiagaraNodeOp::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	VectorVM::FVectorVMOpInfo const& Info = VectorVM::GetOpCodeInfo(OpIndex);
	return Info.FriendlyName;
}

FLinearColor UNiagaraNodeOp::GetNodeTitleColor() const
{
	return GEditor->AccessEditorUserSettings().FunctionCallNodeTitleColor;
}
