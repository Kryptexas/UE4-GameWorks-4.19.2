// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "K2Node_GetInputAxisValue.h"
#include "CompilerResultsLog.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"

#define LOCTEXT_NAMESPACE "K2Node_GetInputAxisValue"

UK2Node_GetInputAxisValue::UK2Node_GetInputAxisValue(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bConsumeInput = true;
}

void UK2Node_GetInputAxisValue::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	UEdGraphPin* InputAxisNamePin = FindPinChecked(TEXT("InputAxisName"));
	InputAxisNamePin->DefaultValue = InputAxisName.ToString();
}

void UK2Node_GetInputAxisValue::Initialize(const FName AxisName)
{
	InputAxisName = AxisName;
	SetFromFunction(AActor::StaticClass()->FindFunctionByName(TEXT("GetInputAxisValue")));
}

FText UK2Node_GetInputAxisValue::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("InputAxisName"), FText::FromName(InputAxisName));

	FText LocFormat = NSLOCTEXT("K2Node", "GetInputAxis_Name", "Get {InputAxisName}");
	if (TitleType == ENodeTitleType::ListView)
	{
		LocFormat = NSLOCTEXT("K2Node", "GetInputAxis_ListTitle", "{InputAxisName}");
	}

	return FText::Format(LocFormat, Args);
}

FString UK2Node_GetInputAxisValue::GetTooltip() const
{
	return FString::Printf(*NSLOCTEXT("K2Node", "GetInputAxis_Tooltip", "Returns the current value of input axis %s.  If input is disabled for the actor the value will be 0.").ToString(), *InputAxisName.ToString());
}

void UK2Node_GetInputAxisValue::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	TArray<FName> AxisNames;
	GetDefault<UInputSettings>()->GetAxisNames(AxisNames);
	if (!AxisNames.Contains(InputAxisName))
	{
		MessageLog.Warning(*FString::Printf(*NSLOCTEXT("KismetCompiler", "MissingInputAxis_Warning", "Get Input Axis references unknown Axis '%s' for @@").ToString(), *InputAxisName.ToString()), this);
	}
}

UClass* UK2Node_GetInputAxisValue::GetDynamicBindingClass() const
{
	return UInputAxisDelegateBinding::StaticClass();
}

void UK2Node_GetInputAxisValue::RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const
{
	UInputAxisDelegateBinding* InputAxisBindingObject = CastChecked<UInputAxisDelegateBinding>(BindingObject);

	FBlueprintInputAxisDelegateBinding Binding;
	Binding.InputAxisName = InputAxisName;
	Binding.bConsumeInput = bConsumeInput;
	Binding.bExecuteWhenPaused = bExecuteWhenPaused;

	InputAxisBindingObject->InputAxisDelegateBindings.Add(Binding);
}

void UK2Node_GetInputAxisValue::GetMenuActions(TArray<UBlueprintNodeSpawner*>& ActionListOut) const
{
	TArray<FName> AxisNames;
	GetDefault<UInputSettings>()->GetAxisNames(AxisNames);

	for (FName const InputAxisName : AxisNames)
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		auto CustomizeInputNodeLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, FName AxisName)
		{
			UK2Node_GetInputAxisValue* InputNode = CastChecked<UK2Node_GetInputAxisValue>(NewNode);
			InputNode->Initialize(AxisName);
		};

		NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(CustomizeInputNodeLambda, InputAxisName);
		ActionListOut.Add(NodeSpawner);
	}
}

FText UK2Node_GetInputAxisValue::GetMenuCategory() const
{
	return FEditorCategoryUtils::BuildCategoryString(FCommonEditorCategory::Input, LOCTEXT("ActionMenuCategory", "Axis Values"));
}

#undef LOCTEXT_NAMESPACE
