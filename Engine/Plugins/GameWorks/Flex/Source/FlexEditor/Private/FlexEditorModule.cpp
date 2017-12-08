// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IFlexEditorModule.h"

#include "FlexEditorPluginBridge.h"

#include "AssetTypeActions_FlexContainer.h"
#include "AssetTypeActions_FlexFluidSurface.h"

#include "ContentBrowserExtensions/ContentBrowserExtensions.h"
#include "LevelEditorMenuExtensions/LevelEditorMenuExtensions.h"

#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"

#include "ObjectTools.h"

#include "Engine/StaticMesh.h"
#include "Misc/CoreDelegates.h"
#include "FlexStaticMesh.h"
#include "FlexAsset.h"

#include "Misc/MessageDialog.h"


DECLARE_LOG_CATEGORY_EXTERN(LogFlexEditor, Log, All);
DEFINE_LOG_CATEGORY(LogFlexEditor);

class FFlexEditorModule : public IFlexEditorModule
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void OnFilesLoaded();

private:
	TSharedPtr<IAssetTypeActions>	FlexContainerAssetActions;
	TSharedPtr<IAssetTypeActions>	FlexFluidAssetActions;
};

IMPLEMENT_MODULE( FFlexEditorModule, FlexEditor )



void FFlexEditorModule::StartupModule()
{
	GFlexEditorPluginBridge = new FFlexEditorPluginBridge();

	// Register asset types
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	FlexContainerAssetActions = MakeShareable(new FAssetTypeActions_FlexContainer);
	FlexFluidAssetActions = MakeShareable(new FAssetTypeActions_FlexFluidSurface);

	AssetTools.RegisterAssetTypeActions(FlexContainerAssetActions.ToSharedRef());
	AssetTools.RegisterAssetTypeActions(FlexFluidAssetActions.ToSharedRef());

	if (!IsRunningCommandlet())
	{
		FFlexContentBrowserExtensions::InstallHooks();
		FFlexLevelEditorMenuExtensions::InstallHooks();

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().OnFilesLoaded().AddRaw(this, &FFlexEditorModule::OnFilesLoaded);
	}
}


void FFlexEditorModule::ShutdownModule()
{
	if (UObjectInitialized())
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().OnFilesLoaded().RemoveAll(this);

		FFlexContentBrowserExtensions::RemoveHooks();
		FFlexLevelEditorMenuExtensions::RemoveHooks();
	}

	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		if (FlexContainerAssetActions.IsValid())
		{
			AssetTools.UnregisterAssetTypeActions(FlexContainerAssetActions.ToSharedRef());
		}

		if (FlexFluidAssetActions.IsValid())
		{
			AssetTools.UnregisterAssetTypeActions(FlexFluidAssetActions.ToSharedRef());
		}
	}

	delete GFlexEditorPluginBridge;
	GFlexEditorPluginBridge = nullptr;
}

void FFlexEditorModule::OnFilesLoaded()
{
	UE_LOG(LogFlexEditor, Log, TEXT("FFlexEditorModule::OnFilesLoaded() ProjectFilePath = '%s'"), *FPaths::GetProjectFilePath());
#if 0
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

	TArray<FAssetData> AssetData;
	FARFilter Filter;
	//Filter.ClassNames.Add(UStaticMesh::StaticClass()->GetFName());
	//Filter.ClassNames.Add(UParticleEmitter::StaticClass()->GetFName());
	Filter.PackagePaths.Add("/Game");
	Filter.bRecursivePaths = true;
	if (AssetRegistryModule.Get().GetAssets(Filter, AssetData) && AssetData.Num() > 0)
	{
		UE_LOG(LogFlexEditor, Log, TEXT("StaticMesh asset list:"));


		TArray<UStaticMesh*> StaticMeshesToConvert;

		for (auto AssetIt = AssetData.CreateConstIterator(); AssetIt; ++AssetIt)
		{
			const FAssetData& Asset = *AssetIt;

			UE_LOG(LogFlexEditor, Log, TEXT("Asset %s"), *Asset.GetFullName());
			UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset.GetAsset());
			if (StaticMesh && StaticMesh->FlexAsset_DEPRECATED)
			{
				UE_LOG(LogFlexEditor, Log, TEXT("--- StaticMesh %s"), *StaticMesh->GetFullName());
				StaticMeshesToConvert.Add(StaticMesh);
			}
		}

		if (StaticMeshesToConvert.Num() == 0)
		{
			return;
		}

		const FText Message = FText::Format(NSLOCTEXT("FlexEditorPlugin", "Convert Static Mesh Assets", "Found {0} non-plugin Flex Static Mesh Assets. Would you like to convert them to use with Flex plugin?"), FText::AsNumber(StaticMeshesToConvert.Num()));
		EAppReturnType::Type Response = FMessageDialog::Open(EAppMsgType::YesNo, Message);
		if (Response == EAppReturnType::Yes)
		{
			for (auto StaticMeshIt = StaticMeshesToConvert.CreateConstIterator(); StaticMeshIt; ++StaticMeshIt)
			{
				UStaticMesh* StaticMesh = *StaticMeshIt;

				UPackage* Package = StaticMesh->GetOutermost();
				const FString PackageName = Package->GetName();
				const FString ObjectName = StaticMesh->GetName();


				ObjectTools::FPackageGroupName PGN;
				PGN.GroupName = TEXT("");
				AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT("_OLD"), /*out*/ PGN.PackageName, /*out*/ PGN.ObjectName);

				TSet<UPackage*> ObjectsUserRefusedToFullyLoad;
				FText ErrorMessage;
				if (ObjectTools::RenameSingleObject(StaticMesh, PGN, ObjectsUserRefusedToFullyLoad, ErrorMessage, NULL, false))
				{
					UFlexStaticMesh* FSM = Cast<UFlexStaticMesh>(StaticDuplicateObject(StaticMesh, Package, *ObjectName, RF_AllFlags, UFlexStaticMesh::StaticClass()));
					if (FSM)
					{
						UE_LOG(LogFlexEditor, Log, TEXT("FlexAsset_DEPRECATED %s %s %x"), *StaticMesh->FlexAsset_DEPRECATED->GetName(), *StaticMesh->FlexAsset_DEPRECATED->GetOutermost()->GetName(), int32(StaticMesh->FlexAsset_DEPRECATED->GetFlags()));

						FSM->FlexAsset = Cast<UFlexAsset>(StaticDuplicateObject(StaticMesh->FlexAsset_DEPRECATED, Package));
						StaticMesh->FlexAsset_DEPRECATED = nullptr;

						UE_LOG(LogFlexEditor, Log, TEXT("FlexAsset %s %s %x"), *FSM->FlexAsset->GetName(), *FSM->FlexAsset->GetOutermost()->GetName(), int32(FSM->FlexAsset->GetFlags()));

						//Package->MarkPackageDirty();

						// Notify the asset registry
						//FAssetRegistryModule::AssetCreated(FSM);

						TArray<UObject*> ObjectsToConsolidate;
						ObjectsToConsolidate.Add(StaticMesh);
						ObjectTools::ConsolidateObjects(FSM, ObjectsToConsolidate, false);
					}
				}
			}
		}
	}
#endif
}
