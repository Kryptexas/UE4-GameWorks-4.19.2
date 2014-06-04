// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"
#include "DetailCustomizations/BehaviorDecoratorDetails.h"
#include "DetailCustomizations/BlackboardDecoratorDetails.h"
#include "DetailCustomizations/BlackboardSelectorDetails.h"

#include "ModuleManager.h"
#include "Toolkits/ToolkitManager.h"
#include "SGraphNode_BehaviorTree.h"
#include "SGraphNode_Decorator.h"
#include "EdGraphUtilities.h"


IMPLEMENT_MODULE( FBehaviorTreeEditorModule, BehaviorTreeEditor );
DEFINE_LOG_CATEGORY(LogBehaviorTreeEditor);

const FName FBehaviorTreeEditorModule::BehaviorTreeEditorAppIdentifier( TEXT( "BehaviorTreeEditorApp" ) );

class FGraphPanelNodeFactory_BehaviorTree : public FGraphPanelNodeFactory
{
	virtual TSharedPtr<class SGraphNode> CreateNode(UEdGraphNode* Node) const OVERRIDE
	{
		if (UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(Node))
		{
			return SNew(SGraphNode_BehaviorTree, BTNode);
		}

		if (UBehaviorTreeDecoratorGraphNode_Decorator* InnerNode = Cast<UBehaviorTreeDecoratorGraphNode_Decorator>(Node))
		{
			return SNew(SGraphNode_Decorator, InnerNode);
		}

		return NULL;
	}
};

TSharedPtr<FGraphPanelNodeFactory> GraphPanelNodeFactory_BehaviorTree;

void FBehaviorTreeEditorModule::StartupModule()
{
	MenuExtensibilityManager = MakeShareable(new FExtensibilityManager);
	ToolBarExtensibilityManager = MakeShareable(new FExtensibilityManager);

	GraphPanelNodeFactory_BehaviorTree = MakeShareable( new FGraphPanelNodeFactory_BehaviorTree() );
	FEdGraphUtilities::RegisterVisualNodeFactory(GraphPanelNodeFactory_BehaviorTree);

	ItemDataAssetTypeActions = MakeShareable(new FAssetTypeActions_BehaviorTree);
	FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get().RegisterAssetTypeActions(ItemDataAssetTypeActions.ToSharedRef());

	// Register the details customizer
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout( "BlackboardKeySelector", FOnGetPropertyTypeCustomizationInstance::CreateStatic( &FBlackboardSelectorDetails::MakeInstance ) );
	PropertyModule.RegisterCustomPropertyLayout( "BTDecorator_Blackboard", FOnGetDetailCustomizationInstance::CreateStatic( &FBlackboardDecoratorDetails::MakeInstance ) );
	PropertyModule.RegisterCustomPropertyLayout( "BTDecorator", FOnGetDetailCustomizationInstance::CreateStatic( &FBehaviorDecoratorDetails::MakeInstance ) );
	PropertyModule.NotifyCustomizationModuleChanged();
}


void FBehaviorTreeEditorModule::ShutdownModule()
{
	if (!UObjectInitialized())
	{
		return;
	}

	MenuExtensibilityManager.Reset();
	ToolBarExtensibilityManager.Reset();
	ClassCache.Reset();

	if ( GraphPanelNodeFactory_BehaviorTree.IsValid() )
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(GraphPanelNodeFactory_BehaviorTree);
		GraphPanelNodeFactory_BehaviorTree.Reset();
	}

	// Unregister the BehaviorTree item data asset type actions
	if (ItemDataAssetTypeActions.IsValid())
	{
		if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
		{
			FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get().UnregisterAssetTypeActions(ItemDataAssetTypeActions.ToSharedRef());
		}
		ItemDataAssetTypeActions.Reset();
	}

	// Unregister the details customization
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterStructPropertyLayout( "BlackboardKeySelector" );
		PropertyModule.UnregisterCustomPropertyLayout( "BTDecorator_Blackboard" );
		PropertyModule.UnregisterCustomPropertyLayout( "BTDecorator" );
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

void FBehaviorTreeEditorModule::HandleExperimentalSettingChanged(FName PropertyName)
{
	if (PropertyName == TEXT("bBehaviorTreeEditor"))
	{
		UBehaviorTreeFactory* EditorUtilityBlueprintFactory = Cast<UBehaviorTreeFactory>(UBehaviorTreeFactory::StaticClass()->GetDefaultObject());
		EditorUtilityBlueprintFactory->bCreateNew = GetDefault<UEditorExperimentalSettings>()->bBehaviorTreeEditor;;
	}
}

TSharedRef<IBehaviorTreeEditor> FBehaviorTreeEditorModule::CreateBehaviorTreeEditor( const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UBehaviorTree* Script )
{
	if (!ClassCache.IsValid())
	{
		ClassCache = MakeShareable(new FClassBrowseHelper);
	}

	TSharedRef< FBehaviorTreeEditor > NewBehaviorTreeEditor( new FBehaviorTreeEditor() );
	NewBehaviorTreeEditor->InitBehaviorTreeEditor( Mode, InitToolkitHost, Script );
	return NewBehaviorTreeEditor;
}

