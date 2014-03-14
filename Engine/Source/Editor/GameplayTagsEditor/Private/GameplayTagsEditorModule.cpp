// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameplayTagsEditorModulePrivatePCH.h"
#include "GameplayTagsGraphPanelPinFactory.h"
#include "GameplayTagContainerCustomization.h"
#include "GameplayTagsEditor.generated.inl"


class FGameplayTagsEditorModule : public IGameplayTagsEditorModule
{
	// Begin IModuleInterface
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;
	// End IModuleInterface
};

IMPLEMENT_MODULE( FGameplayTagsEditorModule, GameplayTagsEditor )

void FGameplayTagsEditorModule::StartupModule()
{
	// Register the details customizer
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterStructPropertyLayout("GameplayTagContainer", FOnGetStructCustomizationInstance::CreateStatic(&FGameplayTagContainerCustomization::MakeInstance));

	TSharedPtr<FGameplayTagsGraphPanelPinFactory> GameplayTagsGraphPanelPinFactory = MakeShareable( new FGameplayTagsGraphPanelPinFactory() );
	FEdGraphUtilities::RegisterVisualPinFactory(GameplayTagsGraphPanelPinFactory);
	
}

void FGameplayTagsEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}
