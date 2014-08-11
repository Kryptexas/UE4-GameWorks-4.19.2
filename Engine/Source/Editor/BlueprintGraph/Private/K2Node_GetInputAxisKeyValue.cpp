// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "K2Node_GetInputAxisKeyValue.h"
#include "CompilerResultsLog.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"
#include "Engine/InputAxisKeyDelegateBinding.h"

#define LOCTEXT_NAMESPACE "K2Node_GetInputAxisKeyValue"

UK2Node_GetInputAxisKeyValue::UK2Node_GetInputAxisKeyValue(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bConsumeInput = true;
}

void UK2Node_GetInputAxisKeyValue::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	UEdGraphPin* InputAxisKeyPin = FindPinChecked(TEXT("InputAxisKey"));
	InputAxisKeyPin->DefaultValue = InputAxisKey.ToString();
}

void UK2Node_GetInputAxisKeyValue::Initialize(const FKey AxisKey)
{
	InputAxisKey = AxisKey;
	SetFromFunction(AActor::StaticClass()->FindFunctionByName(TEXT("GetInputAxisKeyValue")));
}

FText UK2Node_GetInputAxisKeyValue::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("AxisKey"), InputAxisKey.GetDisplayName());
	
	FText TitleFormat = NSLOCTEXT("K2Node", "GetInputAxisKey_Name", "Get {AxisKey}");
	if (TitleType == ENodeTitleType::ListView)
	{
		TitleFormat = NSLOCTEXT("K2Node", "GetInputAxisKey_ListTitle", "{AxisKey}");
	}
	return FText::Format(TitleFormat, Args);
}

FString UK2Node_GetInputAxisKeyValue::GetTooltip() const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("AxisKey"), InputAxisKey.GetDisplayName());
	return FText::Format(NSLOCTEXT("K2Node", "GetInputAxisKey_Tooltip", "Returns the current value of input axis key {AxisKey}.  If input is disabled for the actor the value will be 0."), Args).ToString();
}

void UK2Node_GetInputAxisKeyValue::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	if (!InputAxisKey.IsValid())
	{
		MessageLog.Warning(*FText::Format(NSLOCTEXT("KismetCompiler", "Invalid_GetInputAxisKey_Warning", "GetInputAxisKey Value specifies invalid FKey'{0}' for @@"), FText::FromString(InputAxisKey.ToString())).ToString(), this);
	}
	else if (!InputAxisKey.IsFloatAxis())
	{
		MessageLog.Warning(*FText::Format(NSLOCTEXT("KismetCompiler", "NotAxis_GetInputAxisKey_Warning", "GetInputAxisKey Value specifies FKey'{0}' which is not a float axis for @@"), FText::FromString(InputAxisKey.ToString())).ToString(), this);
	}
	else if (!InputAxisKey.IsBindableInBlueprints())
	{
		MessageLog.Warning(*FText::Format(NSLOCTEXT("KismetCompiler", "NotBindanble_GetInputAxisKey_Warning", "GetInputAxisKey Value specifies FKey'{0}' that is not blueprint bindable for @@"), FText::FromString(InputAxisKey.ToString())).ToString(), this);
	}
}

UClass* UK2Node_GetInputAxisKeyValue::GetDynamicBindingClass() const
{
	return UInputAxisKeyDelegateBinding::StaticClass();
}

void UK2Node_GetInputAxisKeyValue::RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const
{
	UInputAxisKeyDelegateBinding* InputAxisKeyBindingObject = CastChecked<UInputAxisKeyDelegateBinding>(BindingObject);

	FBlueprintInputAxisKeyDelegateBinding Binding;
	Binding.AxisKey = InputAxisKey;
	Binding.bConsumeInput = bConsumeInput;
	Binding.bExecuteWhenPaused = bExecuteWhenPaused;

	InputAxisKeyBindingObject->InputAxisKeyDelegateBindings.Add(Binding);
}

FName UK2Node_GetInputAxisKeyValue::GetPaletteIcon(FLinearColor& OutColor) const
{
	if (InputAxisKey.IsMouseButton())
	{
		return TEXT("GraphEditor.MouseEvent_16x");
	}
	else if (InputAxisKey.IsGamepadKey())
	{
		return TEXT("GraphEditor.PadEvent_16x");
	}
	else
	{
		return TEXT("GraphEditor.KeyEvent_16x");
	}
}


void UK2Node_GetInputAxisKeyValue::GetMenuActions(TArray<UBlueprintNodeSpawner*>& ActionListOut) const
{
	TArray<FKey> AllKeys;
	EKeys::GetAllKeys(AllKeys);
	
	for (FKey const Key : AllKeys)
	{
		if (!Key.IsBindableInBlueprints() || !Key.IsFloatAxis())
		{
			continue;
		}
		
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		
		auto CustomizeInputNodeLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, FKey Key)
		{
			UK2Node_GetInputAxisKeyValue* InputNode = CastChecked<UK2Node_GetInputAxisKeyValue>(NewNode);
			InputNode->Initialize(Key);
		};
		
		NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(CustomizeInputNodeLambda, Key);
		ActionListOut.Add(NodeSpawner);
	}
}

FText UK2Node_GetInputAxisKeyValue::GetMenuCategory() const
{
	FText SubCategory;
	if (InputAxisKey.IsGamepadKey())
	{
		SubCategory = LOCTEXT("GamepadSubCategory", "Gamepad Values");
	}
	else if (InputAxisKey.IsMouseButton())
	{
		SubCategory = LOCTEXT("MouseSubCategory", "Mouse Values");
	}
	else
	{
		SubCategory = LOCTEXT("KeySubCategory", "Key Values");
	}
	
	return FEditorCategoryUtils::BuildCategoryString(FCommonEditorCategory::Input, SubCategory);
}

#undef LOCTEXT_NAMESPACE