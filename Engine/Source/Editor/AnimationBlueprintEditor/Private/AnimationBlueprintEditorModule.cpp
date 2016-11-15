// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AnimationBlueprintEditorPrivatePCH.h"
#include "AnimationBlueprintEditorModule.h"
#include "AnimationBlueprintEditor.h"
#include "BlueprintUtilities.h"
#include "AnimGraphDefinitions.h"
#include "Toolkits/ToolkitManager.h"
#include "Animation/AnimInstance.h"

IMPLEMENT_MODULE( FAnimationBlueprintEditorModule, AnimationBlueprintEditor);

#define LOCTEXT_NAMESPACE "AnimationBlueprintEditorModule"

void FAnimationBlueprintEditorModule::StartupModule()
{
	MenuExtensibilityManager = MakeShareable(new FExtensibilityManager);
	ToolBarExtensibilityManager = MakeShareable(new FExtensibilityManager);

	FKismetEditorUtilities::RegisterOnBlueprintCreatedCallback(this, UAnimInstance::StaticClass(), FKismetEditorUtilities::FOnBlueprintCreated::CreateRaw(this, &FAnimationBlueprintEditorModule::OnNewBlueprintCreated));
}

void FAnimationBlueprintEditorModule::ShutdownModule()
{
	FKismetEditorUtilities::UnregisterAutoBlueprintNodeCreation(this);

	MenuExtensibilityManager.Reset();
	ToolBarExtensibilityManager.Reset();
}

TSharedRef<IAnimationBlueprintEditor> FAnimationBlueprintEditorModule::CreateAnimationBlueprintEditor( const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, class UAnimBlueprint* InAnimBlueprint)
{
	TSharedRef< FAnimationBlueprintEditor > NewAnimationBlueprintEditor( new FAnimationBlueprintEditor() );
	NewAnimationBlueprintEditor->InitAnimationBlueprintEditor( Mode, InitToolkitHost, InAnimBlueprint);
	return NewAnimationBlueprintEditor;
}

void FAnimationBlueprintEditorModule::OnNewBlueprintCreated(UBlueprint* InBlueprint)
{
	if (ensure(InBlueprint->UbergraphPages.Num() > 0))
	{
		UEdGraph* EventGraph = InBlueprint->UbergraphPages[0];

		int32 SafeXPosition = 0;
		int32 SafeYPosition = 0;

		if (EventGraph->Nodes.Num() != 0)
		{
			SafeXPosition = EventGraph->Nodes[0]->NodePosX;
			SafeYPosition = EventGraph->Nodes[EventGraph->Nodes.Num() - 1]->NodePosY + EventGraph->Nodes[EventGraph->Nodes.Num() - 1]->NodeHeight + 100;
		}

		// add try get owner node
		UK2Node_CallFunction* GetOwnerNode = NewObject<UK2Node_CallFunction>(EventGraph);
		UFunction* MakeNodeFunction = UAnimInstance::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UAnimInstance, TryGetPawnOwner));
		GetOwnerNode->CreateNewGuid();
		GetOwnerNode->PostPlacedNewNode();
		GetOwnerNode->SetFromFunction(MakeNodeFunction);
		GetOwnerNode->SetFlags(RF_Transactional);
		GetOwnerNode->AllocateDefaultPins();
		GetOwnerNode->NodePosX = SafeXPosition;
		GetOwnerNode->NodePosY = SafeYPosition;
		UEdGraphSchema_K2::SetNodeMetaData(GetOwnerNode, FNodeMetadata::DefaultGraphNode);
		GetOwnerNode->DisableNode();

		EventGraph->AddNode(GetOwnerNode);
	}
}

#undef LOCTEXT_NAMESPACE