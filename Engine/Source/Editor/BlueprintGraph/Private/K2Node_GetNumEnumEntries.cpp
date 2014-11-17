// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"
#include "../../../Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h"
#include "K2Node_GetNumEnumEntries.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"

UK2Node_GetNumEnumEntries::UK2Node_GetNumEnumEntries(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_GetNumEnumEntries::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	// Create the return value pin
	CreatePin(EGPD_Output, Schema->PC_Int, TEXT(""), NULL, false, false, Schema->PN_ReturnValue);

	Super::AllocateDefaultPins();
}

FString UK2Node_GetNumEnumEntries::GetTooltip() const
{
	const FString EnumName = (Enum != NULL) ? Enum->GetName() : TEXT("(bad enum)");

	return FString::Printf(*NSLOCTEXT("K2Node", "GetNumEnumEntries_Tooltip", "Returns %s_MAX value").ToString(), *EnumName);
}

FText UK2Node_GetNumEnumEntries::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const FText EnumName = (Enum != NULL) ? FText::FromString(Enum->GetName()) : NSLOCTEXT("K2Node", "BadEnum", "(bad enum)");

	FFormatNamedArguments Args;
	Args.Add(TEXT("EnumName"), EnumName);
	return FText::Format(NSLOCTEXT("K2Node", "GetNumEnumEntries_Title", "Get number of entries in {EnumName}"), Args);
}

void UK2Node_GetNumEnumEntries::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (CompilerContext.bIsFullCompile)
	{
		if(NULL == Enum)
		{
			CompilerContext.MessageLog.Error(*FString::Printf(*NSLOCTEXT("K2Node", "GetNumEnumEntries_Error", "@@ must have a valid enum defined").ToString()), this);
			return;
		}

		// Force the enum to load its values if it hasn't already
		if (Enum->HasAnyFlags(RF_NeedLoad))
		{
			Enum->GetLinker()->Preload(Enum);
		}

		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
		check(NULL != Schema);

		//MAKE LITERAL
		const FName FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeLiteralInt);
		UK2Node_CallFunction* MakeLiteralInt = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph); 
		MakeLiteralInt->SetFromFunction(UKismetSystemLibrary::StaticClass()->FindFunctionByName(FunctionName));
		MakeLiteralInt->AllocateDefaultPins();

		//OPUTPUT PIN
		UEdGraphPin* OrgReturnPin = FindPinChecked(Schema->PN_ReturnValue);
		UEdGraphPin* NewReturnPin = MakeLiteralInt->GetReturnValuePin();
		check(NULL != NewReturnPin);
		CompilerContext.MovePinLinksToIntermediate(*OrgReturnPin, *NewReturnPin);

		//INPUT PIN
		UEdGraphPin* InputPin = MakeLiteralInt->FindPinChecked(TEXT("Value"));
		check(EGPD_Input == InputPin->Direction);
		const FString DefaultValue = FString::FromInt(Enum->NumEnums() - 1);
		InputPin->DefaultValue = DefaultValue;

		BreakAllNodeLinks();
	}
}

void UK2Node_GetNumEnumEntries::GetMenuActions(TArray<UBlueprintNodeSpawner*>& ActionListOut) const
{
	for (TObjectIterator<UEnum> EnumIt; EnumIt; ++EnumIt)
	{
		UEnum const* Enum = (*EnumIt);
		// we only want to add global "standalone" enums here; those belonging to a 
		// certain class should instead be associated with that class (so when 
		// the class is modified we can easily handle any enums that were changed).
		//
		// @TODO: don't love how this code is essentially duplicated in BlueprintActionDatabase.cpp, for class enums
		bool bIsStandaloneEnum = Enum->GetOuter()->IsA(UPackage::StaticClass());

		if (!bIsStandaloneEnum || !UEdGraphSchema_K2::IsAllowableBlueprintVariableType(Enum))
		{
			continue;
		}

		auto CustomizeEnumNodeLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, TWeakObjectPtr<UEnum> EnumPtr)
		{
			UK2Node_GetNumEnumEntries* EnumNode = CastChecked<UK2Node_GetNumEnumEntries>(NewNode);
			if (EnumPtr.IsValid())
			{
				EnumNode->Enum = EnumPtr.Get();
			}
		};

		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionListOut.Add(NodeSpawner);

		TWeakObjectPtr<UEnum> EnumPtr = Enum;
		NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(CustomizeEnumNodeLambda, EnumPtr);
	}
}

FText UK2Node_GetNumEnumEntries::GetMenuCategory() const
{
	return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Enum);
}

