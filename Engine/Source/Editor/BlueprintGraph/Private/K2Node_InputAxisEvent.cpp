// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "CompilerResultsLog.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"
#include "Engine/InputAxisDelegateBinding.h"
#include "BlueprintActionDatabaseRegistrar.h"

#define LOCTEXT_NAMESPACE "K2Node_InputAxisEvent"

UK2Node_InputAxisEvent::UK2Node_InputAxisEvent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bConsumeInput = true;
	bOverrideParentBinding = true;
	bInternalEvent = true;

	EventSignatureName = TEXT("InputAxisHandlerDynamicSignature__DelegateSignature");
	EventSignatureClass = UInputComponent::StaticClass();
}

void UK2Node_InputAxisEvent::PostLoad()
{
	Super::PostLoad();

	if (GetLinkerUE4Version() < VER_UE4_BLUEPRINT_INPUT_BINDING_OVERRIDES)
	{
		// Don't change existing behaviors
		bOverrideParentBinding = false;
	}
}

void UK2Node_InputAxisEvent::Initialize(const FName AxisName)
{
	InputAxisName = AxisName;
	CustomFunctionName = FName( *FString::Printf(TEXT("InpAxisEvt_%s_%s"), *InputAxisName.ToString(), *GetName()));
}

FText UK2Node_InputAxisEvent::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::ListView)
	{
		return FText::FromName(InputAxisName);
	}
	else if (CachedNodeTitle.IsOutOfDate())
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("InputAxisName"), FText::FromName(InputAxisName));

		FText LocFormat = NSLOCTEXT("K2Node", "InputAxis_Name", "InputAxis {InputAxisName}");
		// FText::Format() is slow, so we cache this to save on performance
		CachedNodeTitle = FText::Format(LocFormat, Args);
	}

	return CachedNodeTitle;
}

FText UK2Node_InputAxisEvent::GetTooltipText() const
{
	if (CachedTooltip.IsOutOfDate())
	{
		// FText::Format() is slow, so we cache this to save on performance
		CachedTooltip = FText::Format(NSLOCTEXT("K2Node", "InputAxis_Tooltip", "Event that provides the current value of the {0} axis once per frame when input is enabled for the containing actor."), FText::FromName(InputAxisName));
	}
	return CachedTooltip;
}

void UK2Node_InputAxisEvent::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	TArray<FName> AxisNames;
	GetDefault<UInputSettings>()->GetAxisNames(AxisNames);
	if (!AxisNames.Contains(InputAxisName))
	{
		MessageLog.Warning(*FString::Printf(*NSLOCTEXT("KismetCompiler", "MissingInputAxisEvent_Warning", "Input Axis Event references unknown Axis '%s' for @@").ToString(), *InputAxisName.ToString()), this);
	}
}

UClass* UK2Node_InputAxisEvent::GetDynamicBindingClass() const
{
	return UInputAxisDelegateBinding::StaticClass();
}

void UK2Node_InputAxisEvent::RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const
{
	UInputAxisDelegateBinding* InputAxisBindingObject = CastChecked<UInputAxisDelegateBinding>(BindingObject);

	FBlueprintInputAxisDelegateBinding Binding;
	Binding.InputAxisName = InputAxisName;
	Binding.bConsumeInput = bConsumeInput;
	Binding.bExecuteWhenPaused = bExecuteWhenPaused;
	Binding.bOverrideParentBinding = bOverrideParentBinding;
	Binding.FunctionNameToBind = CustomFunctionName;

	InputAxisBindingObject->InputAxisDelegateBindings.Add(Binding);
}

bool UK2Node_InputAxisEvent::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	// By default, to be safe, we don't allow events to be pasted, except under special circumstances (see below)
	bool bIsCompatible = false;

	// Find the Blueprint that owns the target graph
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph);
	if (Blueprint && Blueprint->SkeletonGeneratedClass)
	{
		bIsCompatible = Blueprint->ParentClass->IsChildOf(AActor::StaticClass());
	}

	return bIsCompatible && Super::IsCompatibleWithGraph(TargetGraph);
}

void UK2Node_InputAxisEvent::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	TArray<FName> AxisNames;
	GetDefault<UInputSettings>()->GetAxisNames(AxisNames);

	for (FName const InputAxisName : AxisNames)
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		auto CustomizeInputNodeLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, FName AxisName)
		{
			UK2Node_InputAxisEvent* InputNode = CastChecked<UK2Node_InputAxisEvent>(NewNode);
			InputNode->Initialize(AxisName);
		};

		NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(CustomizeInputNodeLambda, InputAxisName);
		ActionRegistrar.AddBlueprintAction(NodeSpawner);
	}
}

FText UK2Node_InputAxisEvent::GetMenuCategory() const
{
	static FNodeTextCache CachedCategory;
	if (CachedCategory.IsOutOfDate())
	{
		// FText::Format() is slow, so we cache this to save on performance
		CachedCategory = FEditorCategoryUtils::BuildCategoryString(FCommonEditorCategory::Input, LOCTEXT("ActionMenuCategory", "Axis Events"));
	}
	return CachedCategory;
}

#undef LOCTEXT_NAMESPACE
