// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnvironmentQueryEditorPrivatePCH.h"
#include "DetailCustomizations/EnvQueryParamSetupCustomization.h"
#include "DetailCustomizations/EnvQueryTestDetails.h"
#include "DetailCustomizations/EnvQueryTest_TraceDetails.h"
#include "DetailCustomizations/EnvQueryTest_DotDetails.h"
#include "ModuleManager.h"
#include "Toolkits/ToolkitManager.h"
#include "SGraphNode_EnvironmentQuery.h"
#include "EdGraphUtilities.h"
#include "EnvironmentQueryEditor.generated.inl"

IMPLEMENT_MODULE( FEnvironmentQueryEditorModule, EnvironmentQueryEditor );
DEFINE_LOG_CATEGORY(LogEnvironmentQueryEditor);

const FName FEnvironmentQueryEditorModule::EnvironmentQueryEditorAppIdentifier( TEXT( "EnvironmentQueryEditorApp" ) );

class FGraphPanelNodeFactory_EnvironmentQuery : public FGraphPanelNodeFactory
{
	virtual TSharedPtr<class SGraphNode> CreateNode(UEdGraphNode* Node) const OVERRIDE
	{
		if (UEnvironmentQueryGraphNode* EnvQueryNode = Cast<UEnvironmentQueryGraphNode>(Node))
		{
			return SNew(SGraphNode_EnvironmentQuery, EnvQueryNode);
		}

		return NULL;
	}
};

TSharedPtr<FGraphPanelNodeFactory> GraphPanelNodeFactory_EnvironmentQuery;

void FEnvironmentQueryEditorModule::StartupModule()
{
	MenuExtensibilityManager = MakeShareable(new FExtensibilityManager);
	ToolBarExtensibilityManager = MakeShareable(new FExtensibilityManager);

	GraphPanelNodeFactory_EnvironmentQuery = MakeShareable( new FGraphPanelNodeFactory_EnvironmentQuery() );
	FEdGraphUtilities::RegisterVisualNodeFactory(GraphPanelNodeFactory_EnvironmentQuery);

	ItemDataAssetTypeActions = MakeShareable(new FAssetTypeActions_EnvironmentQuery);
	FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get().RegisterAssetTypeActions(ItemDataAssetTypeActions.ToSharedRef());

	// Register the details customizer
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterStructPropertyLayout( "EnvFloatParam", FOnGetStructCustomizationInstance::CreateStatic( &FEnvQueryParamSetupCustomization::MakeInstance ) );
	PropertyModule.RegisterStructPropertyLayout( "EnvIntParam", FOnGetStructCustomizationInstance::CreateStatic( &FEnvQueryParamSetupCustomization::MakeInstance ) );
	PropertyModule.RegisterStructPropertyLayout( "EnvBoolParam", FOnGetStructCustomizationInstance::CreateStatic( &FEnvQueryParamSetupCustomization::MakeInstance ) );
	PropertyModule.RegisterCustomPropertyLayout( UEnvQueryTest_Trace::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FEnvQueryTest_TraceDetails::MakeInstance ) );
	PropertyModule.RegisterCustomPropertyLayout( UEnvQueryTest_Dot::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FEnvQueryTest_DotDetails::MakeInstance ) );
	PropertyModule.RegisterCustomPropertyLayout( UEnvQueryTest::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic( &FEnvQueryTestDetails::MakeInstance ) );
	PropertyModule.NotifyCustomizationModuleChanged();
}


void FEnvironmentQueryEditorModule::ShutdownModule()
{
	if (!UObjectInitialized())
	{
		return;
	}

	MenuExtensibilityManager.Reset();
	ToolBarExtensibilityManager.Reset();

	if ( GraphPanelNodeFactory_EnvironmentQuery.IsValid() )
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(GraphPanelNodeFactory_EnvironmentQuery);
		GraphPanelNodeFactory_EnvironmentQuery.Reset();
	}

	// Unregister the EnvironmentQuery item data asset type actions
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
		PropertyModule.UnregisterStructPropertyLayout( "EnvFloatParam" );
		PropertyModule.UnregisterStructPropertyLayout( "EnvIntParam" );
		PropertyModule.UnregisterStructPropertyLayout( "EnvBoolParam" );
		PropertyModule.UnregisterCustomPropertyLayout( UEnvQueryTest::StaticClass() );
		PropertyModule.UnregisterCustomPropertyLayout( UEnvQueryTest_Dot::StaticClass() );
		PropertyModule.UnregisterCustomPropertyLayout( UEnvQueryTest_Trace::StaticClass() );
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}


TSharedRef<IEnvironmentQueryEditor> FEnvironmentQueryEditorModule::CreateEnvironmentQueryEditor( const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UEnvQuery* Script )
{
	TSharedRef< FEnvironmentQueryEditor > NewEnvironmentQueryEditor( new FEnvironmentQueryEditor() );
	NewEnvironmentQueryEditor->InitEnvironmentQueryEditor( Mode, InitToolkitHost, Script );
	return NewEnvironmentQueryEditor;
}

