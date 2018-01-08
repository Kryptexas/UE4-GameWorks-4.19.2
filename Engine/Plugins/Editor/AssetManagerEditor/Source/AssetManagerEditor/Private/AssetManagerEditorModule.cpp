// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AssetManagerEditorModule.h"
#include "ModuleManager.h"
#include "AssetRegistryModule.h"
#include "PrimaryAssetTypeCustomization.h"
#include "PrimaryAssetIdCustomization.h"
#include "SAssetAuditBrowser.h"
#include "Engine/PrimaryAssetLabel.h"

#include "JsonReader.h"
#include "JsonSerializer.h"
#include "CollectionManagerModule.h"
#include "GameDelegates.h"
#include "ICollectionManager.h"
#include "ARFilter.h"
#include "FileHelper.h"
#include "ProfilingHelpers.h"
#include "StatsMisc.h"
#include "Engine/AssetManager.h"
#include "PropertyEditorModule.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/Commands.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/SToolTip.h"
#include "PropertyCustomizationHelpers.h"
#include "AssetEditorToolkit.h"
#include "LevelEditor.h"
#include "GraphEditorModule.h"
#include "AssetData.h"
#include "Engine/World.h"
#include "Misc/App.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Interfaces/ITargetPlatform.h"
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "IPlatformFileSandboxWrapper.h"
#include "HAL/PlatformFilemanager.h"
#include "Serialization/ArrayReader.h"
#include "EdGraphUtilities.h"
#include "EdGraphSchema_K2.h"
#include "SGraphPin.h"
#include "AssetManagerEditorCommands.h"
#include "ReferenceViewer/SReferenceViewer.h"
#include "ReferenceViewer/SReferenceNode.h"
#include "ReferenceViewer/EdGraphNode_Reference.h"
#include "SSizeMap.h"
#include "Editor.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "DesktopPlatformModule.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "UObject/ConstructorHelpers.h"


#define LOCTEXT_NAMESPACE "AssetManagerEditor"

DEFINE_LOG_CATEGORY(LogAssetManagerEditor);

class FAssetManagerGraphPanelNodeFactory : public FGraphPanelNodeFactory
{
	virtual TSharedPtr<class SGraphNode> CreateNode(UEdGraphNode* Node) const override
	{
		if (UEdGraphNode_Reference* DependencyNode = Cast<UEdGraphNode_Reference>(Node))
		{
			return SNew(SReferenceNode, DependencyNode);
		}

		return nullptr;
	}
};

class FAssetManagerGraphPanelPinFactory : public FGraphPanelPinFactory
{
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* InPin) const override
	{
		if (InPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct && InPin->PinType.PinSubCategoryObject == TBaseStructure<FPrimaryAssetId>::Get())
		{
			return SNew(SPrimaryAssetIdGraphPin, InPin);
		}
		if (InPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct && InPin->PinType.PinSubCategoryObject == TBaseStructure<FPrimaryAssetType>::Get())
		{
			return SNew(SPrimaryAssetTypeGraphPin, InPin);
		}

		return nullptr;
	}
};

const FName IAssetManagerEditorModule::ChunkFakeAssetDataPackageName = FName("/Temp/PrimaryAsset/PackageChunk");
const FName IAssetManagerEditorModule::PrimaryAssetFakeAssetDataPackagePath = FName("/Temp/PrimaryAsset");
const FPrimaryAssetType IAssetManagerEditorModule::AllPrimaryAssetTypes = FName("AllTypes");

const FName IAssetManagerEditorModule::ResourceSizeName = FName("ResourceSize");
const FName IAssetManagerEditorModule::DiskSizeName = FName("DiskSize");
const FName IAssetManagerEditorModule::ManagedResourceSizeName = FName("ManagedResourceSize");
const FName IAssetManagerEditorModule::ManagedDiskSizeName = FName("ManagedDiskSize");
const FName IAssetManagerEditorModule::TotalUsageName = FName("TotalUsage");
const FName IAssetManagerEditorModule::CookRuleName = FName("CookRule");
const FName IAssetManagerEditorModule::ChunksName = FName("Chunks");

const FString FAssetManagerEditorRegistrySource::EditorSourceName = TEXT("Editor");
const FString FAssetManagerEditorRegistrySource::CustomSourceName = TEXT("Custom");

TSharedRef<SWidget> IAssetManagerEditorModule::MakePrimaryAssetTypeSelector(FOnGetPrimaryAssetDisplayText OnGetDisplayText, FOnSetPrimaryAssetType OnSetType, bool bAllowClear, bool bAllowAll)
{
	FOnGetPropertyComboBoxStrings GetStrings = FOnGetPropertyComboBoxStrings::CreateStatic(&IAssetManagerEditorModule::GeneratePrimaryAssetTypeComboBoxStrings, bAllowClear, bAllowAll);
	FOnGetPropertyComboBoxValue GetValue = FOnGetPropertyComboBoxValue::CreateLambda([OnGetDisplayText]
	{
		return OnGetDisplayText.Execute().ToString();
	});
	FOnPropertyComboBoxValueSelected SetValue = FOnPropertyComboBoxValueSelected::CreateLambda([OnSetType](const FString& StringValue)
	{
		OnSetType.Execute(FPrimaryAssetType(*StringValue));
	});

	return PropertyCustomizationHelpers::MakePropertyComboBox(nullptr, GetStrings, GetValue, SetValue);
}

TSharedRef<SWidget> IAssetManagerEditorModule::MakePrimaryAssetIdSelector(FOnGetPrimaryAssetDisplayText OnGetDisplayText, FOnSetPrimaryAssetId OnSetId, bool bAllowClear, TArray<FPrimaryAssetType> AllowedTypes)
{
	FOnGetContent OnCreateMenuContent = FOnGetContent::CreateLambda([OnGetDisplayText, OnSetId, bAllowClear, AllowedTypes]()
	{
		FOnShouldFilterAsset AssetFilter = FOnShouldFilterAsset::CreateStatic(&IAssetManagerEditorModule::OnShouldFilterPrimaryAsset, AllowedTypes);
		FOnSetObject OnSetObject = FOnSetObject::CreateLambda([OnSetId](const FAssetData& AssetData)
		{
			FSlateApplication::Get().DismissAllMenus();
			UAssetManager& Manager = UAssetManager::Get();

			FPrimaryAssetId AssetId;
			if (AssetData.IsValid())
			{
				AssetId = Manager.GetPrimaryAssetIdForData(AssetData);
				ensure(AssetId.IsValid());
			}

			OnSetId.Execute(AssetId);
		});

		TArray<const UClass*> AllowedClasses;
		TArray<UFactory*> NewAssetFactories;

		return PropertyCustomizationHelpers::MakeAssetPickerWithMenu(
			FAssetData(),
			bAllowClear,
			AllowedClasses,
			NewAssetFactories,
			AssetFilter,
			OnSetObject,
			FSimpleDelegate());
	});

	TAttribute<FText> OnGetObjectText = TAttribute<FText>::Create(OnGetDisplayText);

	return SNew(SComboButton)
		.OnGetMenuContent(OnCreateMenuContent)
		.ContentPadding(FMargin(2.0f, 2.0f))
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text(OnGetObjectText)
			.ToolTipText(OnGetObjectText)
			.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
		];
}

void IAssetManagerEditorModule::GeneratePrimaryAssetTypeComboBoxStrings(TArray< TSharedPtr<FString> >& OutComboBoxStrings, TArray<TSharedPtr<class SToolTip>>& OutToolTips, TArray<bool>& OutRestrictedItems, bool bAllowClear, bool bAllowAll)
{
	UAssetManager& AssetManager = UAssetManager::Get();

	TArray<FPrimaryAssetTypeInfo> TypeInfos;

	AssetManager.GetPrimaryAssetTypeInfoList(TypeInfos);
	TypeInfos.Sort([](const FPrimaryAssetTypeInfo& LHS, const FPrimaryAssetTypeInfo& RHS) { return LHS.PrimaryAssetType < RHS.PrimaryAssetType; });

	// Can the field be cleared
	if (bAllowClear)
	{
		// Add None
		OutComboBoxStrings.Add(MakeShared<FString>(FPrimaryAssetType().ToString()));
		OutToolTips.Add(SNew(SToolTip).Text(LOCTEXT("NoType", "None")));
		OutRestrictedItems.Add(false);
	}

	for (FPrimaryAssetTypeInfo& Info : TypeInfos)
	{
		OutComboBoxStrings.Add(MakeShared<FString>(Info.PrimaryAssetType.ToString()));

		FText TooltipText = FText::Format(LOCTEXT("ToolTipFormat", "{0}:{1}{2}"),
			FText::FromString(Info.PrimaryAssetType.ToString()),
			Info.bIsEditorOnly ? LOCTEXT("EditorOnly", " EditorOnly") : FText(),
			Info.bHasBlueprintClasses ? LOCTEXT("Blueprints", " Blueprints") : FText());

		OutToolTips.Add(SNew(SToolTip).Text(TooltipText));
		OutRestrictedItems.Add(false);
	}

	if (bAllowAll)
	{
		// Add All
		OutComboBoxStrings.Add(MakeShared<FString>(IAssetManagerEditorModule::AllPrimaryAssetTypes.ToString()));
		OutToolTips.Add(SNew(SToolTip).Text(LOCTEXT("AllTypes", "All Primary Asset Types")));
		OutRestrictedItems.Add(false);
	}
}

bool IAssetManagerEditorModule::OnShouldFilterPrimaryAsset(const FAssetData& InAssetData, TArray<FPrimaryAssetType> AllowedTypes)
{
	// Make sure it has a primary asset id, and do type check
	UAssetManager& Manager = UAssetManager::Get();

	if (InAssetData.IsValid())
	{
		FPrimaryAssetId AssetId = Manager.GetPrimaryAssetIdForData(InAssetData);
		if (AssetId.IsValid())
		{
			if (AllowedTypes.Num() > 0)
			{
				if (!AllowedTypes.Contains(AssetId.PrimaryAssetType))
				{
					return true;
				}
			}

			return false;
		}
	}

	return true;
}

FAssetData IAssetManagerEditorModule::CreateFakeAssetDataFromChunkId(int32 ChunkID)
{
	return CreateFakeAssetDataFromPrimaryAssetId(UAssetManager::CreatePrimaryAssetIdFromChunkId(ChunkID));
}

int32 IAssetManagerEditorModule::ExtractChunkIdFromFakeAssetData(const FAssetData& InAssetData)
{
	return UAssetManager::ExtractChunkIdFromPrimaryAssetId(ExtractPrimaryAssetIdFromFakeAssetData(InAssetData));
}

FAssetData IAssetManagerEditorModule::CreateFakeAssetDataFromPrimaryAssetId(const FPrimaryAssetId& PrimaryAssetId)
{
	FString PackageNameString = PrimaryAssetFakeAssetDataPackagePath.ToString() / PrimaryAssetId.PrimaryAssetType.ToString();

	return FAssetData(*PackageNameString, PrimaryAssetFakeAssetDataPackagePath, PrimaryAssetId.PrimaryAssetName, PrimaryAssetId.PrimaryAssetType);
}

FPrimaryAssetId IAssetManagerEditorModule::ExtractPrimaryAssetIdFromFakeAssetData(const FAssetData& InAssetData)
{
	if (InAssetData.PackagePath == PrimaryAssetFakeAssetDataPackagePath)
	{
		return FPrimaryAssetId(InAssetData.AssetClass, InAssetData.AssetName);
	}
	return FPrimaryAssetId();
}

void IAssetManagerEditorModule::ExtractAssetIdentifiersFromAssetDataList(const TArray<FAssetData>& AssetDataList, TArray<FAssetIdentifier>& OutAssetIdentifiers)
{
	for (const FAssetData& AssetData : AssetDataList)
	{
		FPrimaryAssetId PrimaryAssetId = IAssetManagerEditorModule::ExtractPrimaryAssetIdFromFakeAssetData(AssetData);

		if (PrimaryAssetId.IsValid())
		{
			OutAssetIdentifiers.Add(PrimaryAssetId);
		}
		else
		{
			OutAssetIdentifiers.Add(AssetData.PackageName);
		}
	}
}

// Concrete implementation

class FAssetManagerEditorModule : public IAssetManagerEditorModule
{
public:
	// IModuleInterface interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	// End of IModuleInterface interface

	void PerformAuditConsoleCommand(const TArray<FString>& Args);
	void PerformDependencyChainConsoleCommand(const TArray<FString>& Args);
	void PerformDependencyClassConsoleCommand(const TArray<FString>& Args);
	void DumpAssetDependencies(const TArray<FString>& Args);

	virtual void OpenAssetAuditUI(TArray<FAssetData> SelectedAssets) override;
	virtual void OpenAssetAuditUI(TArray<FName> SelectedPackages) override;
	virtual void OpenReferenceViewerUI(TArray<FAssetIdentifier> SelectedIdentifiers) override;
	virtual void OpenReferenceViewerUI(TArray<FName> SelectedPackages) override;
	virtual void OpenSizeMapUI(TArray<FAssetIdentifier> SelectedIdentifiers) override;
	virtual void OpenSizeMapUI(TArray<FName> SelectedPackages) override;	
	virtual bool GetStringValueForCustomColumn(const FAssetData& AssetData, FName ColumnName, FString& OutValue) override;
	virtual bool GetDisplayTextForCustomColumn(const FAssetData& AssetData, FName ColumnName, FText& OutValue) override;
	virtual bool GetIntegerValueForCustomColumn(const FAssetData& AssetData, FName ColumnName, int64& OutValue) override;
	virtual bool GetManagedPackageListForAssetData(const FAssetData& AssetData, TSet<FName>& ManagedPackageSet) override;
	virtual void GetAvailableRegistrySources(TArray<const FAssetManagerEditorRegistrySource*>& AvailableSources) override;
	virtual const FAssetManagerEditorRegistrySource* GetCurrentRegistrySource(bool bNeedManagementData = false) override;
	virtual void SetCurrentRegistrySource(const FString& SourceName) override;
	virtual void RefreshRegistryData() override;
	virtual bool IsPackageInCurrentRegistrySource(FName PackageName) override;
	virtual bool FilterAssetIdentifiersForCurrentRegistrySource(TArray<FAssetIdentifier>& AssetIdentifiers, EAssetRegistryDependencyType::Type DependencyType = EAssetRegistryDependencyType::None, bool bForwardDependency = true) override;
	virtual bool WriteCollection(FName CollectionName, ECollectionShareType::Type ShareType, const TArray<FName>& PackageNames, bool bShowFeedback) override;
private:

	static bool GetDependencyTypeArg(const FString& Arg, EAssetRegistryDependencyType::Type& OutDepType);

	//Prints all dependency chains from assets in the search path to the target package.
	void FindReferenceChains(FName TargetPackageName, FName RootSearchPath, EAssetRegistryDependencyType::Type DependencyType);

	//Prints all dependency chains from the PackageName to any dependency of one of the given class names.
	//If the package name is a path rather than a package, then it will do this for each package in the path.
	void FindClassDependencies(FName PackagePath, const TArray<FName>& TargetClasses, EAssetRegistryDependencyType::Type DependencyType);

	bool GetPackageDependencyChain(FName SourcePackage, FName TargetPackage, TArray<FName>& VisitedPackages, TArray<FName>& OutDependencyChain, EAssetRegistryDependencyType::Type DependencyType);
	void GetPackageDependenciesPerClass(FName SourcePackage, const TArray<FName>& TargetClasses, TArray<FName>& VisitedPackages, TArray<FName>& OutDependentPackages, EAssetRegistryDependencyType::Type DependencyType);

	void LogAssetsWithMultipleLabels();
	bool CreateOrEmptyCollection(FName CollectionName, ECollectionShareType::Type ShareType);
	void WriteProfileFile(const FString& Extension, const FString& FileContents);
	
	FString GetSavedAssetRegistryPath(ITargetPlatform* TargetPlatform);
	void GetAssetDataInPaths(const TArray<FString>& Paths, TArray<FAssetData>& OutAssetData);
	bool AreLevelEditorPackagesSelected();
	TArray<FName> GetLevelEditorSelectedAssetPackages();
	TArray<FName> GetContentBrowserSelectedAssetPackages(FOnContentBrowserGetSelection GetSelectionDelegate);
	void InitializeRegistrySources(bool bNeedManagementData);

	TArray<IConsoleObject*> AuditCmds;

	static const TCHAR* FindDepChainHelpText;
	static const TCHAR* FindClassDepHelpText;
	static const FName AssetAuditTabName;
	static const FName ReferenceViewerTabName;
	static const FName SizeMapTabName;

	FDelegateHandle ContentBrowserAssetExtenderDelegateHandle;
	FDelegateHandle ContentBrowserPathExtenderDelegateHandle;
	FDelegateHandle ContentBrowserCommandExtenderDelegateHandle;
	FDelegateHandle ReferenceViewerDelegateHandle;
	FDelegateHandle AssetEditorExtenderDelegateHandle;
	FDelegateHandle LevelEditorExtenderDelegateHandle;

	TWeakPtr<SDockTab> AssetAuditTab;
	TWeakPtr<SDockTab> ReferenceViewerTab;
	TWeakPtr<SDockTab> SizeMapTab;
	TWeakPtr<SAssetAuditBrowser> AssetAuditUI;
	TWeakPtr<SSizeMap> SizeMapUI;
	TWeakPtr<SReferenceViewer> ReferenceViewerUI;
	TMap<FString, FAssetManagerEditorRegistrySource> RegistrySourceMap;
	FAssetManagerEditorRegistrySource* CurrentRegistrySource;

	IAssetRegistry* AssetRegistry;
	FSandboxPlatformFile* CookedSandbox;
	FSandboxPlatformFile* EditorCookedSandbox;
	TSharedPtr<FAssetManagerGraphPanelNodeFactory> AssetManagerGraphPanelNodeFactory;
	TSharedPtr<FAssetManagerGraphPanelPinFactory> AssetManagerGraphPanelPinFactory;

	void CreateAssetContextMenu(FMenuBuilder& MenuBuilder);
	void OnExtendContentBrowserCommands(TSharedRef<FUICommandList> CommandList, FOnContentBrowserGetSelection GetSelectionDelegate);
	void OnExtendLevelEditorCommands(TSharedRef<FUICommandList> CommandList);
	TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets);
	TSharedRef<FExtender> OnExtendContentBrowserPathSelectionMenu(const TArray<FString>& SelectedPaths);
	TSharedRef<FExtender> OnExtendAssetEditor(const TSharedRef<FUICommandList> CommandList, const TArray<UObject*> ContextSensitiveObjects);
	TSharedRef<FExtender> OnExtendLevelEditor(const TSharedRef<FUICommandList> CommandList, const TArray<AActor*> SelectedActors);

	TSharedRef<SDockTab> SpawnAssetAuditTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnReferenceViewerTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnSizeMapTab(const FSpawnTabArgs& Args);
};

const TCHAR* FAssetManagerEditorModule::FindDepChainHelpText = TEXT("Finds all dependency chains from assets in the given search path, to the target package.\n Usage: FindDepChain TargetPackagePath SearchRootPath (optional: -hardonly/-softonly)\n e.g. FindDepChain /game/characters/heroes/muriel/meshes/muriel /game/cards ");
const TCHAR* FAssetManagerEditorModule::FindClassDepHelpText = TEXT("Finds all dependencies of a certain set of classes to the target asset.\n Usage: FindDepClasses TargetPackagePath ClassName1 ClassName2 etc (optional: -hardonly/-softonly) \n e.g. FindDepChain /game/characters/heroes/muriel/meshes/muriel /game/cards");
const FName FAssetManagerEditorModule::AssetAuditTabName = TEXT("AssetAudit");
const FName FAssetManagerEditorModule::ReferenceViewerTabName = TEXT("ReferenceViewer");
const FName FAssetManagerEditorModule::SizeMapTabName = TEXT("SizeMap");

///////////////////////////////////////////

IMPLEMENT_MODULE(FAssetManagerEditorModule, AssetManagerEditor);


void FAssetManagerEditorModule::StartupModule()
{
	CookedSandbox = nullptr;
	EditorCookedSandbox = nullptr;
	CurrentRegistrySource = nullptr;

	// Load the tree map module so we can use it for size map
	FModuleManager::Get().LoadModule(TEXT("TreeMap"));

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistry = &AssetRegistryModule.Get();

	FAssetManagerEditorCommands::Register();

	if (GIsEditor && !IsRunningCommandlet())
	{
		AuditCmds.Add(IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("AssetManager.AssetAudit"),
			TEXT("Dumps statistics about assets to the log."),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FAssetManagerEditorModule::PerformAuditConsoleCommand),
			ECVF_Default
			));

		AuditCmds.Add(IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("AssetManager.FindDepChain"),
			FindDepChainHelpText,
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FAssetManagerEditorModule::PerformDependencyChainConsoleCommand),
			ECVF_Default
			));

		AuditCmds.Add(IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("AssetManager.FindDepClasses"),
			FindClassDepHelpText,
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FAssetManagerEditorModule::PerformDependencyClassConsoleCommand),
			ECVF_Default
			));

		AuditCmds.Add(IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("AssetManager.DumpAssetDependencies"),
			TEXT("Shows a list of all primary assets and the secondary assets that they depend on. Also writes out a .graphviz file"),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FAssetManagerEditorModule::DumpAssetDependencies),
			ECVF_Default
			));

		// Register customizations
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomPropertyTypeLayout("PrimaryAssetType", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPrimaryAssetTypeCustomization::MakeInstance));
		PropertyModule.RegisterCustomPropertyTypeLayout("PrimaryAssetId", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPrimaryAssetIdCustomization::MakeInstance));
		PropertyModule.NotifyCustomizationModuleChanged();

		// Register Pins and nodes
		AssetManagerGraphPanelPinFactory = MakeShareable(new FAssetManagerGraphPanelPinFactory());
		FEdGraphUtilities::RegisterVisualPinFactory(AssetManagerGraphPanelPinFactory);

		AssetManagerGraphPanelNodeFactory = MakeShareable(new FAssetManagerGraphPanelNodeFactory());
		FEdGraphUtilities::RegisterVisualNodeFactory(AssetManagerGraphPanelNodeFactory);

		// Register content browser hook
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
		
		TArray<FContentBrowserMenuExtender_SelectedAssets>& CBAssetMenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
		CBAssetMenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateRaw(this, &FAssetManagerEditorModule::OnExtendContentBrowserAssetSelectionMenu));
		ContentBrowserAssetExtenderDelegateHandle = CBAssetMenuExtenderDelegates.Last().GetHandle();

		TArray<FContentBrowserMenuExtender_SelectedPaths>& CBPathExtenderDelegates = ContentBrowserModule.GetAllPathViewContextMenuExtenders();
		CBPathExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedPaths::CreateRaw(this, &FAssetManagerEditorModule::OnExtendContentBrowserPathSelectionMenu));
		ContentBrowserPathExtenderDelegateHandle = CBPathExtenderDelegates.Last().GetHandle();

		TArray<FContentBrowserCommandExtender>& CBCommandExtenderDelegates = ContentBrowserModule.GetAllContentBrowserCommandExtenders();
		CBCommandExtenderDelegates.Add(FContentBrowserCommandExtender::CreateRaw(this, &FAssetManagerEditorModule::OnExtendContentBrowserCommands));
		ContentBrowserCommandExtenderDelegateHandle = CBCommandExtenderDelegates.Last().GetHandle();

		// Register asset editor hooks
		TArray<FAssetEditorExtender>& AssetEditorMenuExtenderDelegates = FAssetEditorToolkit::GetSharedMenuExtensibilityManager()->GetExtenderDelegates();
		AssetEditorMenuExtenderDelegates.Add(FAssetEditorExtender::CreateRaw(this, &FAssetManagerEditorModule::OnExtendAssetEditor));
		AssetEditorExtenderDelegateHandle = AssetEditorMenuExtenderDelegates.Last().GetHandle();

		// Register level editor hooks and commands
		FLevelEditorModule& LevelEditorModule = FModuleManager::Get().LoadModuleChecked<FLevelEditorModule>("LevelEditor");

		TSharedRef<FUICommandList> CommandList = LevelEditorModule.GetGlobalLevelEditorActions();
		OnExtendLevelEditorCommands(CommandList);

		TArray<FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors>& LevelEditorMenuExtenderDelegates = LevelEditorModule.GetAllLevelViewportContextMenuExtenders();
		LevelEditorMenuExtenderDelegates.Add(FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors::CreateRaw(this, &FAssetManagerEditorModule::OnExtendLevelEditor));
		LevelEditorExtenderDelegateHandle = AssetEditorMenuExtenderDelegates.Last().GetHandle();

		// Add nomad tabs
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(AssetAuditTabName, FOnSpawnTab::CreateRaw(this, &FAssetManagerEditorModule::SpawnAssetAuditTab))
			.SetDisplayName(LOCTEXT("AssetAuditTitle", "Asset Audit"))
			.SetTooltipText(LOCTEXT("AssetAuditTooltip", "Open Asset Audit window, allows viewing detailed information about assets."))
			.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory());

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ReferenceViewerTabName, FOnSpawnTab::CreateRaw(this, &FAssetManagerEditorModule::SpawnReferenceViewerTab))
			.SetDisplayName(LOCTEXT("ReferenceViewerTitle", "Reference Viewer"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(SizeMapTabName, FOnSpawnTab::CreateRaw(this, &FAssetManagerEditorModule::SpawnSizeMapTab))
			.SetDisplayName(LOCTEXT("SizeMapTitle", "Size Map"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);
	}
}

void FAssetManagerEditorModule::ShutdownModule()
{
	if (CookedSandbox)
	{
		delete CookedSandbox;
		CookedSandbox = nullptr;
	}

	if (EditorCookedSandbox)
	{
		delete EditorCookedSandbox;
		EditorCookedSandbox = nullptr;
	}

	for (IConsoleObject* AuditCmd : AuditCmds)
	{
		IConsoleManager::Get().UnregisterConsoleObject(AuditCmd);
	}
	AuditCmds.Empty();

	if ((GIsEditor && !IsRunningCommandlet()) && UObjectInitialized() && FSlateApplication::IsInitialized())
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
		
		TArray<FContentBrowserMenuExtender_SelectedAssets>& CBAssetMenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
		CBAssetMenuExtenderDelegates.RemoveAll([this](const FContentBrowserMenuExtender_SelectedAssets& Delegate) { return Delegate.GetHandle() == ContentBrowserAssetExtenderDelegateHandle; });
		TArray<FContentBrowserMenuExtender_SelectedPaths>& CBPathMenuExtenderDelegates = ContentBrowserModule.GetAllPathViewContextMenuExtenders();
		CBPathMenuExtenderDelegates.RemoveAll([this](const FContentBrowserMenuExtender_SelectedPaths& Delegate) { return Delegate.GetHandle() == ContentBrowserPathExtenderDelegateHandle; });
		TArray<FContentBrowserCommandExtender>& CBCommandExtenderDelegates = ContentBrowserModule.GetAllContentBrowserCommandExtenders();
		CBCommandExtenderDelegates.RemoveAll([this](const FContentBrowserCommandExtender& Delegate) { return Delegate.GetHandle() == ContentBrowserCommandExtenderDelegateHandle; });
		
		FGraphEditorModule& GraphEdModule = FModuleManager::LoadModuleChecked<FGraphEditorModule>(TEXT("GraphEditor"));
		
		TArray<FGraphEditorModule::FGraphEditorMenuExtender_SelectedNode>& ReferenceViewerMenuExtenderDelegates = GraphEdModule.GetAllGraphEditorContextMenuExtender();
		ReferenceViewerMenuExtenderDelegates.RemoveAll([this](const FGraphEditorModule::FGraphEditorMenuExtender_SelectedNode& Delegate) { return Delegate.GetHandle() == ReferenceViewerDelegateHandle; });

		TArray<FAssetEditorExtender>& AssetEditorMenuExtenderDelegates = FAssetEditorToolkit::GetSharedMenuExtensibilityManager()->GetExtenderDelegates();
		AssetEditorMenuExtenderDelegates.RemoveAll([this](const FAssetEditorExtender& Delegate) { return Delegate.GetHandle() == AssetEditorExtenderDelegateHandle; });

		FLevelEditorModule& LevelEditorModule = FModuleManager::Get().LoadModuleChecked<FLevelEditorModule>("LevelEditor");

		TSharedRef<FUICommandList> CommandList = LevelEditorModule.GetGlobalLevelEditorActions();
		CommandList->UnmapAction(FAssetManagerEditorCommands::Get().ViewReferences);
		CommandList->UnmapAction(FAssetManagerEditorCommands::Get().ViewSizeMap);
		CommandList->UnmapAction(FAssetManagerEditorCommands::Get().ViewAssetAudit);

		TArray<FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors>& LevelEditorMenuExtenderDelegates = LevelEditorModule.GetAllLevelViewportContextMenuExtenders();
		LevelEditorMenuExtenderDelegates.RemoveAll([this](const FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors& Delegate) { return Delegate.GetHandle() == LevelEditorExtenderDelegateHandle; });

		if (AssetManagerGraphPanelNodeFactory.IsValid())
		{
			FEdGraphUtilities::UnregisterVisualNodeFactory(AssetManagerGraphPanelNodeFactory);
		}

		if (AssetManagerGraphPanelPinFactory.IsValid())
		{
			FEdGraphUtilities::UnregisterVisualPinFactory(AssetManagerGraphPanelPinFactory);
		}

		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AssetAuditTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ReferenceViewerTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SizeMapTabName);

		if (AssetAuditTab.IsValid())
		{
			AssetAuditTab.Pin()->RequestCloseTab();
		}
		if (ReferenceViewerTab.IsValid())
		{
			ReferenceViewerTab.Pin()->RequestCloseTab();
		}
		if (SizeMapTab.IsValid())
		{
			SizeMapTab.Pin()->RequestCloseTab();
		}
	}
}

TSharedRef<SDockTab> FAssetManagerEditorModule::SpawnAssetAuditTab(const FSpawnTabArgs& Args)
{
	if (!UAssetManager::IsValid())
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("BadAssetAuditUI", "Cannot load Asset Audit if there is no asset manager!"))
			];
	}
	
	return SAssignNew(AssetAuditTab, SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SAssignNew(AssetAuditUI, SAssetAuditBrowser)
		];
}

TSharedRef<SDockTab> FAssetManagerEditorModule::SpawnReferenceViewerTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> NewTab = SAssignNew(ReferenceViewerTab, SDockTab)
		.TabRole(ETabRole::NomadTab);

	NewTab->SetContent(SAssignNew(ReferenceViewerUI, SReferenceViewer));

	return NewTab;
}

TSharedRef<SDockTab> FAssetManagerEditorModule::SpawnSizeMapTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> NewTab = SAssignNew(SizeMapTab, SDockTab)
		.TabRole(ETabRole::NomadTab);

	NewTab->SetContent(SAssignNew(SizeMapUI, SSizeMap));

	return NewTab;
}

void FAssetManagerEditorModule::OpenAssetAuditUI(TArray<FAssetData> SelectedAssets)
{
	FGlobalTabmanager::Get()->InvokeTab(AssetAuditTabName);

	if (AssetAuditUI.IsValid())
	{
		AssetAuditUI.Pin()->AddAssetsToList(SelectedAssets, false);
	}
}

void FAssetManagerEditorModule::OpenAssetAuditUI(TArray<FName> SelectedPackages)
{
	FGlobalTabmanager::Get()->InvokeTab(AssetAuditTabName);

	if (AssetAuditUI.IsValid())
	{
		AssetAuditUI.Pin()->AddAssetsToList(SelectedPackages, false);
	}
}

void FAssetManagerEditorModule::OpenReferenceViewerUI(TArray<FAssetIdentifier> SelectedIdentifiers)
{
	if (SelectedIdentifiers.Num() > 0)
	{
		TSharedRef<SDockTab> NewTab = FGlobalTabmanager::Get()->InvokeTab(ReferenceViewerTabName);
		TSharedRef<SReferenceViewer> ReferenceViewer = StaticCastSharedRef<SReferenceViewer>(NewTab->GetContent());
		ReferenceViewer->SetGraphRootIdentifiers(SelectedIdentifiers);
	}
}

void FAssetManagerEditorModule::OpenReferenceViewerUI(TArray<FName> SelectedPackages)
{
	TArray<FAssetIdentifier> Identifiers;
	for (FName Name : SelectedPackages)
	{
		Identifiers.Add(FAssetIdentifier(Name));
	}

	OpenReferenceViewerUI(Identifiers);
}

void FAssetManagerEditorModule::OpenSizeMapUI(TArray<FName> SelectedPackages)
{
	TArray<FAssetIdentifier> Identifiers;
	for (FName Name : SelectedPackages)
	{
		Identifiers.Add(FAssetIdentifier(Name));
	}

	OpenSizeMapUI(Identifiers);
}

void FAssetManagerEditorModule::OpenSizeMapUI(TArray<FAssetIdentifier> SelectedIdentifiers)
{
	if (SelectedIdentifiers.Num() > 0)
	{
		TSharedRef<SDockTab> NewTab = FGlobalTabmanager::Get()->InvokeTab(SizeMapTabName);
		TSharedRef<SSizeMap> SizeMap = StaticCastSharedRef<SSizeMap>(NewTab->GetContent());
		SizeMap->SetRootAssetIdentifiers(SelectedIdentifiers);
	}
}

void FAssetManagerEditorModule::GetAssetDataInPaths(const TArray<FString>& Paths, TArray<FAssetData>& OutAssetData)
{
	// Form a filter from the paths
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	for (const FString& Path : Paths)
	{
		new (Filter.PackagePaths) FName(*Path);
	}

	// Query for a list of assets in the selected paths
	AssetRegistry->GetAssets(Filter, OutAssetData);
}

bool FAssetManagerEditorModule::AreLevelEditorPackagesSelected()
{
	return GetLevelEditorSelectedAssetPackages().Num() > 0;
}

TArray<FName> FAssetManagerEditorModule::GetLevelEditorSelectedAssetPackages()
{
	TArray<FName> OutAssetPackages;
	TArray<UObject*> ReferencedAssets;
	GEditor->GetReferencedAssetsForEditorSelection(ReferencedAssets);

	for (UObject* EditedAsset : ReferencedAssets)
	{
		if (EditedAsset && EditedAsset->IsAsset() && !EditedAsset->IsPendingKill())
		{
			OutAssetPackages.AddUnique(EditedAsset->GetOutermost()->GetFName());
		}
	}
	return OutAssetPackages;
}

TArray<FName> FAssetManagerEditorModule::GetContentBrowserSelectedAssetPackages(FOnContentBrowserGetSelection GetSelectionDelegate)
{
	TArray<FName> OutAssetPackages;
	TArray<FAssetData> SelectedAssets;
	TArray<FString> SelectedPaths;

	if (GetSelectionDelegate.IsBound())
	{
		GetSelectionDelegate.Execute(SelectedAssets, SelectedPaths);
	}

	GetAssetDataInPaths(SelectedPaths, SelectedAssets);

	TArray<FName> PackageNames;
	for (const FAssetData& AssetData : SelectedAssets)
	{
		OutAssetPackages.AddUnique(AssetData.PackageName);
	}

	return OutAssetPackages;
}

void FAssetManagerEditorModule::CreateAssetContextMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(FAssetManagerEditorCommands::Get().ViewReferences);
	MenuBuilder.AddMenuEntry(FAssetManagerEditorCommands::Get().ViewSizeMap);
	MenuBuilder.AddMenuEntry(FAssetManagerEditorCommands::Get().ViewAssetAudit);
}

void FAssetManagerEditorModule::OnExtendContentBrowserCommands(TSharedRef<FUICommandList> CommandList, FOnContentBrowserGetSelection GetSelectionDelegate)
{
	// There is no can execute because the focus state is weird during calls to it
	CommandList->MapAction(FAssetManagerEditorCommands::Get().ViewReferences, 
		FExecuteAction::CreateLambda([this, GetSelectionDelegate]
		{
			OpenReferenceViewerUI(GetContentBrowserSelectedAssetPackages(GetSelectionDelegate));
		})
	);

	CommandList->MapAction(FAssetManagerEditorCommands::Get().ViewSizeMap,
		FExecuteAction::CreateLambda([this, GetSelectionDelegate]
		{
			OpenSizeMapUI(GetContentBrowserSelectedAssetPackages(GetSelectionDelegate));
		})
	);

	CommandList->MapAction(FAssetManagerEditorCommands::Get().ViewAssetAudit,
		FExecuteAction::CreateLambda([this, GetSelectionDelegate]
		{
			OpenAssetAuditUI(GetContentBrowserSelectedAssetPackages(GetSelectionDelegate));
		})
	);
}

void FAssetManagerEditorModule::OnExtendLevelEditorCommands(TSharedRef<FUICommandList> CommandList)
{
	CommandList->MapAction(FAssetManagerEditorCommands::Get().ViewReferences, 
		FExecuteAction::CreateLambda([this]
		{
			OpenReferenceViewerUI(GetLevelEditorSelectedAssetPackages());
		}),
		FCanExecuteAction::CreateRaw(this, &FAssetManagerEditorModule::AreLevelEditorPackagesSelected)
	);

	CommandList->MapAction(FAssetManagerEditorCommands::Get().ViewSizeMap,
		FExecuteAction::CreateLambda([this]
		{
			OpenSizeMapUI(GetLevelEditorSelectedAssetPackages());
		}),
		FCanExecuteAction::CreateRaw(this, &FAssetManagerEditorModule::AreLevelEditorPackagesSelected)
	);

	CommandList->MapAction(FAssetManagerEditorCommands::Get().ViewAssetAudit,
		FExecuteAction::CreateLambda([this]
		{
			OpenAssetAuditUI(GetLevelEditorSelectedAssetPackages());
		}),
		FCanExecuteAction::CreateRaw(this, &FAssetManagerEditorModule::AreLevelEditorPackagesSelected)
	);
}

TSharedRef<FExtender> FAssetManagerEditorModule::OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets)
{
	TSharedRef<FExtender> Extender(new FExtender());
	
	Extender->AddMenuExtension(
		"AssetContextReferences",
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateRaw(this, &FAssetManagerEditorModule::CreateAssetContextMenu));

	return Extender;
}

TSharedRef<FExtender> FAssetManagerEditorModule::OnExtendContentBrowserPathSelectionMenu(const TArray<FString>& SelectedPaths)
{
	TSharedRef<FExtender> Extender(new FExtender());

	Extender->AddMenuExtension(
		"PathContextBulkOperations",
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateRaw(this, &FAssetManagerEditorModule::CreateAssetContextMenu));

	return Extender;
}

TSharedRef<FExtender> FAssetManagerEditorModule::OnExtendAssetEditor(const TSharedRef<FUICommandList> CommandList, const TArray<UObject*> ContextSensitiveObjects)
{
	TArray<FName> PackageNames;
	for (UObject* EditedAsset : ContextSensitiveObjects)
	{
		if (EditedAsset && EditedAsset->IsAsset() && !EditedAsset->IsPendingKill())
		{
			PackageNames.AddUnique(EditedAsset->GetOutermost()->GetFName());
		}
	}

	TSharedRef<FExtender> Extender(new FExtender());

	if (PackageNames.Num() > 0)
	{
		// It's safe to modify the CommandList here because this is run as the editor UI is created and the payloads are safe
		CommandList->MapAction(
			FAssetManagerEditorCommands::Get().ViewReferences,
			FExecuteAction::CreateRaw(this, &FAssetManagerEditorModule::OpenReferenceViewerUI, PackageNames));

		CommandList->MapAction(
			FAssetManagerEditorCommands::Get().ViewSizeMap,
			FExecuteAction::CreateRaw(this, &FAssetManagerEditorModule::OpenSizeMapUI, PackageNames));

		CommandList->MapAction(
			FAssetManagerEditorCommands::Get().ViewAssetAudit,
			FExecuteAction::CreateRaw(this, &FAssetManagerEditorModule::OpenAssetAuditUI, PackageNames));

		Extender->AddMenuExtension(
			"FindInContentBrowser",
			EExtensionHook::After,
			CommandList,
			FMenuExtensionDelegate::CreateRaw(this, &FAssetManagerEditorModule::CreateAssetContextMenu));
	}

	return Extender;
}

TSharedRef<FExtender> FAssetManagerEditorModule::OnExtendLevelEditor(const TSharedRef<FUICommandList> CommandList, const TArray<AActor*> SelectedActors)
{
	TSharedRef<FExtender> Extender(new FExtender());

	// Most of this work is handled in the command list extension
	if (SelectedActors.Num() > 0)
	{
		Extender->AddMenuExtension(
			"ActorAsset",
			EExtensionHook::After,
			CommandList,
			FMenuExtensionDelegate::CreateRaw(this, &FAssetManagerEditorModule::CreateAssetContextMenu));
	}

	return Extender;
}

bool FAssetManagerEditorModule::GetManagedPackageListForAssetData(const FAssetData& AssetData, TSet<FName>& ManagedPackageSet)
{
	InitializeRegistrySources(true);

	TSet<FPrimaryAssetId> PrimaryAssetSet;
	bool bFoundAny = false;
	int32 ChunkId = IAssetManagerEditorModule::ExtractChunkIdFromFakeAssetData(AssetData);
	if (ChunkId != INDEX_NONE)
	{
		// This is a chunk meta asset
		const FAssetManagerChunkInfo* FoundChunkInfo = CurrentRegistrySource->ChunkAssignments.Find(ChunkId);

		if (!FoundChunkInfo)
		{
			return false;
		}
		
		for (const FAssetIdentifier& AssetIdentifier : FoundChunkInfo->AllAssets)
		{
			if (AssetIdentifier.PackageName != NAME_None)
			{
				bFoundAny = true;
				ManagedPackageSet.Add(AssetIdentifier.PackageName);
			}
		}
	}
	else
	{
		FPrimaryAssetId FoundPrimaryAssetId = IAssetManagerEditorModule::ExtractPrimaryAssetIdFromFakeAssetData(AssetData);
		if (FoundPrimaryAssetId.IsValid())
		{
			// Primary asset meta package
			PrimaryAssetSet.Add(FoundPrimaryAssetId);
		}
		else
		{
			// Normal asset package
			FoundPrimaryAssetId = UAssetManager::Get().GetPrimaryAssetIdForData(AssetData);

			if (!FoundPrimaryAssetId.IsValid())
			{
				return false;
			}

			PrimaryAssetSet.Add(FoundPrimaryAssetId);
		}
	}

	TArray<FAssetIdentifier> FoundDependencies;

	if (CurrentRegistrySource->RegistryState)
	{
		for (const FPrimaryAssetId& PrimaryAssetId : PrimaryAssetSet)
		{
			CurrentRegistrySource->RegistryState->GetDependencies(PrimaryAssetId, FoundDependencies, EAssetRegistryDependencyType::Manage);
		}
	}
	
	for (const FAssetIdentifier& Identifier : FoundDependencies)
	{
		if (Identifier.PackageName != NAME_None)
		{
			bFoundAny = true;
			ManagedPackageSet.Add(Identifier.PackageName);
		}
	}
	return bFoundAny;
}

bool FAssetManagerEditorModule::GetStringValueForCustomColumn(const FAssetData& AssetData, FName ColumnName, FString& OutValue)
{
	if (!CurrentRegistrySource || !CurrentRegistrySource->RegistryState)
	{
		return false;
	}

	UAssetManager& AssetManager = UAssetManager::Get();

	if (ColumnName == ManagedResourceSizeName || ColumnName == ManagedDiskSizeName || ColumnName == DiskSizeName || ColumnName == TotalUsageName)
	{
		// Get integer, convert to string
		int64 IntegerValue = 0;
		if (GetIntegerValueForCustomColumn(AssetData, ColumnName, IntegerValue))
		{
			OutValue = Lex::ToString(IntegerValue);
			return true;
		}
	}
	else if (ColumnName == CookRuleName)
	{
		EPrimaryAssetCookRule CookRule;

		CookRule = AssetManager.GetPackageCookRule(AssetData.PackageName);

		switch (CookRule)
		{
		case EPrimaryAssetCookRule::AlwaysCook: 
			OutValue = TEXT("Always");
			return true;
		case EPrimaryAssetCookRule::DevelopmentCook: 
			OutValue = TEXT("Development");
			return true;
		case EPrimaryAssetCookRule::NeverCook: 
			OutValue = TEXT("Never");
			return true;
		}
	}
	else if (ColumnName == ChunksName)
	{
		TArray<int32> FoundChunks;
		OutValue.Reset();

		if (CurrentRegistrySource->bIsEditor)
		{
			// The in-memory data is wrong, ask the asset manager
			AssetManager.GetPackageChunkIds(AssetData.PackageName, CurrentRegistrySource->TargetPlatform, AssetData.ChunkIDs, FoundChunks);
		}
		else
		{
			const FAssetData* PlatformData = CurrentRegistrySource->RegistryState->GetAssetByObjectPath(AssetData.ObjectPath);
			if (PlatformData)
			{
				FoundChunks = PlatformData->ChunkIDs;
			}
		}
		
		FoundChunks.Sort();

		for (int32 Chunk : FoundChunks)
		{
			if (!OutValue.IsEmpty())
			{
				OutValue += TEXT("+");
			}
			OutValue += Lex::ToString(Chunk);
		}
		return true;
	}
	else
	{
		// Get base value of asset tag
		return AssetData.GetTagValue(ColumnName, OutValue);
	}

	return false;
}

bool FAssetManagerEditorModule::GetDisplayTextForCustomColumn(const FAssetData& AssetData, FName ColumnName, FText& OutValue)
{
	if (!CurrentRegistrySource || !CurrentRegistrySource->RegistryState)
	{
		return false;
	}

	UAssetManager& AssetManager = UAssetManager::Get();

	if (ColumnName == ManagedResourceSizeName || ColumnName == ManagedDiskSizeName || ColumnName == DiskSizeName || ColumnName == TotalUsageName)
	{
		// Get integer, convert to string
		int64 IntegerValue = 0;
		if (GetIntegerValueForCustomColumn(AssetData, ColumnName, IntegerValue))
		{
			if (ColumnName == TotalUsageName)
			{
				OutValue = FText::AsNumber(IntegerValue);
			}
			else
			{
				// Display size properly
				OutValue = FText::AsMemory(IntegerValue);
			}
			return true;
		}
	}
	else if (ColumnName == CookRuleName)
	{
		EPrimaryAssetCookRule CookRule;

		CookRule = AssetManager.GetPackageCookRule(AssetData.PackageName);

		switch (CookRule)
		{
		case EPrimaryAssetCookRule::AlwaysCook:
			OutValue = LOCTEXT("AlwaysCook", "Always");
			return true;
		case EPrimaryAssetCookRule::DevelopmentCook:
			OutValue = LOCTEXT("DevelopmentCook", "Development");
			return true;
		case EPrimaryAssetCookRule::NeverCook:
			OutValue = LOCTEXT("NeverCook", "Never");
			return true;
		}
	}
	else if (ColumnName == ChunksName)
	{
		FString OutString;

		if (GetStringValueForCustomColumn(AssetData, ColumnName, OutString))
		{
			OutValue = FText::AsCultureInvariant(OutString);
			return true;
		}
	}
	else
	{
		// Get base value of asset tag
		return AssetData.GetTagValue(ColumnName, OutValue);
	}

	return false;
}

bool FAssetManagerEditorModule::GetIntegerValueForCustomColumn(const FAssetData& AssetData, FName ColumnName, int64& OutValue)
{
	if (!CurrentRegistrySource || !CurrentRegistrySource->RegistryState)
	{
		return false;
	}

	UAssetManager& AssetManager = UAssetManager::Get();

	if (ColumnName == ManagedResourceSizeName || ColumnName == ManagedDiskSizeName)
	{
		FName SizeTag = (ColumnName == ManagedResourceSizeName) ? ResourceSizeName : DiskSizeName;
		TSet<FName> AssetPackageSet;

		if (!GetManagedPackageListForAssetData(AssetData, AssetPackageSet))
		{
			// Just return exclusive
			return GetIntegerValueForCustomColumn(AssetData, SizeTag, OutValue);
		}

		int64 TotalSize = 0;
		bool bFoundAny = false;

		for (FName PackageName : AssetPackageSet)
		{
			TArray<FAssetData> FoundData;
			FARFilter AssetFilter;
			AssetFilter.PackageNames.Add(PackageName);
			AssetFilter.bIncludeOnlyOnDiskAssets = true;

			if (AssetRegistry->GetAssets(AssetFilter, FoundData) && FoundData.Num() > 0)
			{
				// Use first one
				FAssetData& ManagedAssetData = FoundData[0];

				int64 PackageSize = 0;
				if (GetIntegerValueForCustomColumn(ManagedAssetData, SizeTag, PackageSize))
				{
					bFoundAny = true;
					TotalSize += PackageSize;
				}
			}
		}

		if (bFoundAny)
		{
			OutValue = TotalSize;
			return true;
		}
	}
	else if (ColumnName == DiskSizeName)
	{
		const FAssetPackageData* FoundData = CurrentRegistrySource->RegistryState->GetAssetPackageData(AssetData.PackageName);

		if (FoundData && FoundData->DiskSize >= 0)
		{
			OutValue = FoundData->DiskSize;
			return true;
		}
	}
	else if (ColumnName == ResourceSizeName)
	{
		// Resource size can currently only be calculated for loaded assets, so load and check
		UObject* Asset = AssetData.GetAsset();

		if (Asset)
		{
			OutValue = Asset->GetResourceSizeBytes(EResourceSizeMode::EstimatedTotal);
			return true;
		}
	}
	else if (ColumnName == TotalUsageName)
	{
		int64 TotalWeight = 0;

		TSet<FPrimaryAssetId> ReferencingPrimaryAssets;

		AssetManager.GetPackageManagers(AssetData.PackageName, false, ReferencingPrimaryAssets);

		for (const FPrimaryAssetId& PrimaryAssetId : ReferencingPrimaryAssets)
		{
			FPrimaryAssetRules Rules = AssetManager.GetPrimaryAssetRules(PrimaryAssetId);

			if (!Rules.IsDefault())
			{
				TotalWeight += Rules.Priority;
			}
		}

		OutValue = TotalWeight;
		return true;
	}
	else
	{
		// Get base value of asset tag
		if (AssetData.GetTagValue(ColumnName, OutValue))
		{
			return true;
		}
	}

	return false;
}

FString FAssetManagerEditorModule::GetSavedAssetRegistryPath(ITargetPlatform* TargetPlatform)
{
	if (!TargetPlatform)
	{
		return FString();
	}

	FString PlatformName = TargetPlatform->PlatformName();

	// Initialize sandbox wrapper
	if (!CookedSandbox)
	{
		CookedSandbox = new FSandboxPlatformFile(false);

		FString OutputDirectory = FPaths::Combine(*FPaths::ProjectDir(), TEXT("Saved"), TEXT("Cooked"), TEXT("[Platform]"));
		FPaths::NormalizeDirectoryName(OutputDirectory);

		CookedSandbox->Initialize(&FPlatformFileManager::Get().GetPlatformFile(), *FString::Printf(TEXT("-sandbox=\"%s\""), *OutputDirectory));
	}

	if (!EditorCookedSandbox)
	{
		EditorCookedSandbox = new FSandboxPlatformFile(false);

		FString OutputDirectory = FPaths::Combine(*FPaths::ProjectDir(), TEXT("Saved"), TEXT("EditorCooked"), TEXT("[Platform]"));
		FPaths::NormalizeDirectoryName(OutputDirectory);

		EditorCookedSandbox->Initialize(&FPlatformFileManager::Get().GetPlatformFile(), *FString::Printf(TEXT("-sandbox=\"%s\""), *OutputDirectory));
	}

	FString CommandLinePath;
	FParse::Value(FCommandLine::Get(), TEXT("AssetRegistryFile="), CommandLinePath);
	CommandLinePath.ReplaceInline(TEXT("[Platform]"), *PlatformName);
	
	// We can only load DevelopmentAssetRegistry, the normal asset registry doesn't have enough data to be useful
	FString CookedDevelopmentAssetRegistry = FPaths::ProjectDir() / TEXT("Metadata") / TEXT("DevelopmentAssetRegistry.bin");

	FString DevCookedPath = CookedSandbox->ConvertToAbsolutePathForExternalAppForWrite(*CookedDevelopmentAssetRegistry).Replace(TEXT("[Platform]"), *PlatformName);
	FString DevEditorCookedPath = EditorCookedSandbox->ConvertToAbsolutePathForExternalAppForWrite(*CookedDevelopmentAssetRegistry).Replace(TEXT("[Platform]"), *PlatformName);
	FString DevSharedCookedPath = FPaths::Combine(*FPaths::ProjectSavedDir(), TEXT("SharedIterativeBuild"), PlatformName, TEXT("Metadata"), TEXT("DevelopmentAssetRegistry.bin"));

	// Try command line, then cooked, then shared build
	if (!CommandLinePath.IsEmpty() && IFileManager::Get().FileExists(*CommandLinePath))
	{
		return CommandLinePath;
	}

	if (IFileManager::Get().FileExists(*DevCookedPath))
	{
		return DevCookedPath;
	}

	if (IFileManager::Get().FileExists(*DevEditorCookedPath))
	{
		return DevEditorCookedPath;
	}

	if (IFileManager::Get().FileExists(*DevSharedCookedPath))
	{
		return DevSharedCookedPath;
	}

	return FString();
}

void FAssetManagerEditorModule::GetAvailableRegistrySources(TArray<const FAssetManagerEditorRegistrySource*>& AvailableSources)
{
	InitializeRegistrySources(false);

	for (const TPair<FString, FAssetManagerEditorRegistrySource>& Pair : RegistrySourceMap)
	{
		AvailableSources.Add(&Pair.Value);
	}
}

const FAssetManagerEditorRegistrySource* FAssetManagerEditorModule::GetCurrentRegistrySource(bool bNeedManagementData)
{
	InitializeRegistrySources(bNeedManagementData);

	return CurrentRegistrySource;
}

void FAssetManagerEditorModule::SetCurrentRegistrySource(const FString& SourceName)
{
	InitializeRegistrySources(false);

	FAssetManagerEditorRegistrySource* NewSource = RegistrySourceMap.Find(SourceName);

	if (NewSource)
	{
		CurrentRegistrySource = NewSource;

		if (CurrentRegistrySource->SourceName == FAssetManagerEditorRegistrySource::CustomSourceName)
		{
			if (CurrentRegistrySource->RegistryState)
			{
				check(!CurrentRegistrySource->bIsEditor);

				delete CurrentRegistrySource->RegistryState;
				CurrentRegistrySource->RegistryState = nullptr;
				CurrentRegistrySource->ChunkAssignments.Reset();
				CurrentRegistrySource->bManagementDataInitialized = false;
			}
			CurrentRegistrySource->SourceFilename.Reset();

			IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
			const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
			const FText Title = LOCTEXT("LoadAssetRegistry", "Load DevelopmentAssetRegistry.bin");
			const FString FileTypes = TEXT("DevelopmentAssetRegistry.bin|*.bin");

			TArray<FString> OutFilenames;
			DesktopPlatform->OpenFileDialog(
				ParentWindowWindowHandle,
				Title.ToString(),
				TEXT(""),
				TEXT("DevelopmentAssetRegistry.bin"),
				FileTypes,
				EFileDialogFlags::None,
				OutFilenames
			);

			if (OutFilenames.Num() == 1)
			{
				CurrentRegistrySource->SourceFilename = OutFilenames[0];
			}
			else
			{
				CurrentRegistrySource = RegistrySourceMap.Find(FAssetManagerEditorRegistrySource::EditorSourceName);
			}
		}

		if (CurrentRegistrySource->RegistryState == nullptr && !CurrentRegistrySource->SourceFilename.IsEmpty() && !CurrentRegistrySource->bIsEditor)
		{
			bool bLoaded = false;
			FArrayReader SerializedAssetData;
			if (FFileHelper::LoadFileToArray(SerializedAssetData, *CurrentRegistrySource->SourceFilename))
			{
				FAssetRegistryState* NewState = new FAssetRegistryState();
				FAssetRegistrySerializationOptions Options;

				Options.ModifyForDevelopment();
				NewState->Serialize(SerializedAssetData, Options);

				if (NewState->GetObjectPathToAssetDataMap().Num() > 0)
				{
					bLoaded = true;
					CurrentRegistrySource->RegistryState = NewState;
				}
			}
			if (!bLoaded)
			{
				FNotificationInfo Info(FText::Format(LOCTEXT("LoadRegistryFailed", "Failed to load asset registry from {0}!"), FText::FromString(CurrentRegistrySource->SourceFilename)));
				Info.ExpireDuration = 10.0f;
				FSlateNotificationManager::Get().AddNotification(Info);
				CurrentRegistrySource = RegistrySourceMap.Find(FAssetManagerEditorRegistrySource::EditorSourceName);
			}
		}

		if (!CurrentRegistrySource->bManagementDataInitialized && CurrentRegistrySource->RegistryState)
		{
			CurrentRegistrySource->ChunkAssignments.Reset();
			if (CurrentRegistrySource->bIsEditor)
			{
				// Load chunk list from asset manager
				if (UAssetManager::IsValid())
				{
					CurrentRegistrySource->ChunkAssignments = UAssetManager::Get().GetChunkManagementMap();
				}
			}
			else
			{
				// Iterate assets and look for chunks
				const TMap<FName, const FAssetData*>& AssetDataMap = CurrentRegistrySource->RegistryState->GetObjectPathToAssetDataMap();

				for (const TPair<FName, const FAssetData*>& Pair : AssetDataMap)
				{
					const FAssetData& AssetData = *Pair.Value;
					if (AssetData.ChunkIDs.Num() > 0)
					{
						TArray<FAssetIdentifier> ManagerAssets;
						CurrentRegistrySource->RegistryState->GetReferencers(AssetData.PackageName, ManagerAssets, EAssetRegistryDependencyType::Manage);

						for (int32 ChunkId : AssetData.ChunkIDs)
						{
							FPrimaryAssetId ChunkAssetId = UAssetManager::CreatePrimaryAssetIdFromChunkId(ChunkId);
							
							FAssetManagerChunkInfo* ChunkAssignmentSet = CurrentRegistrySource->ChunkAssignments.Find(ChunkId);

							if (!ChunkAssignmentSet)
							{
								// First time found, read the graph
								ChunkAssignmentSet = &CurrentRegistrySource->ChunkAssignments.Add(ChunkId);
								
								TArray<FAssetIdentifier> ManagedAssets;

								CurrentRegistrySource->RegistryState->GetDependencies(ChunkAssetId, ManagedAssets, EAssetRegistryDependencyType::Manage);

								for (const FAssetIdentifier& ManagedAsset : ManagedAssets)
								{
									ChunkAssignmentSet->ExplicitAssets.Add(ManagedAsset);
								}
							}

							ChunkAssignmentSet->AllAssets.Add(AssetData.PackageName);

							// Check to see if this was added by a management reference, if not register as an explicit reference
							bool bAddedFromManager = false;
							for (const FAssetIdentifier& Manager : ManagerAssets)
							{
								if (ChunkAssignmentSet->ExplicitAssets.Find(Manager))
								{
									bAddedFromManager = true;
								}
							}

							if (!bAddedFromManager)
							{
								ChunkAssignmentSet->ExplicitAssets.Add(AssetData.PackageName);
							}
						}
					}
				}
			}

			CurrentRegistrySource->bManagementDataInitialized = true;
		}
	}
	else
	{
		FNotificationInfo Info(FText::Format(LOCTEXT("LoadRegistryFailed", "Can't find registry source {0}! Reverting to Editor."), FText::FromString(SourceName)));
		Info.ExpireDuration = 10.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		CurrentRegistrySource = RegistrySourceMap.Find(FAssetManagerEditorRegistrySource::EditorSourceName);
	}

	check(CurrentRegistrySource);
	
	// Refresh UI
	if (AssetAuditUI.IsValid())
	{
		AssetAuditUI.Pin()->SetCurrentRegistrySource(CurrentRegistrySource);
	}
	if (SizeMapUI.IsValid())
	{
		SizeMapUI.Pin()->SetCurrentRegistrySource(CurrentRegistrySource);
	}
	if (ReferenceViewerUI.IsValid())
	{
		ReferenceViewerUI.Pin()->SetCurrentRegistrySource(CurrentRegistrySource);
	}
}

void FAssetManagerEditorModule::RefreshRegistryData()
{
	UAssetManager::Get().UpdateManagementDatabase();

	// Rescan registry sources, try to restore the current one
	FString OldSourceName = CurrentRegistrySource->SourceName;

	CurrentRegistrySource = nullptr;
	InitializeRegistrySources(false);

	SetCurrentRegistrySource(OldSourceName);
}

bool FAssetManagerEditorModule::IsPackageInCurrentRegistrySource(FName PackageName)
{
	if (CurrentRegistrySource && CurrentRegistrySource->RegistryState && !CurrentRegistrySource->bIsEditor)
	{
		const FAssetPackageData* FoundData = CurrentRegistrySource->RegistryState->GetAssetPackageData(PackageName);

		if (!FoundData || FoundData->DiskSize < 0)
		{
			return false;
		}
	}

	// In editor, no packages are filtered
	return true;
}

bool FAssetManagerEditorModule::FilterAssetIdentifiersForCurrentRegistrySource(TArray<FAssetIdentifier>& AssetIdentifiers, EAssetRegistryDependencyType::Type DependencyType, bool bForwardDependency)
{
	bool bMadeChange = false;
	if (!CurrentRegistrySource || !CurrentRegistrySource->RegistryState || CurrentRegistrySource->bIsEditor)
	{
		return bMadeChange;
	}

	for (int32 Index = 0; Index < AssetIdentifiers.Num(); Index++)
	{
		FName PackageName = AssetIdentifiers[Index].PackageName;

		if (PackageName != NAME_None)
		{
			if (!IsPackageInCurrentRegistrySource(PackageName))
			{
				// Remove bad package
				AssetIdentifiers.RemoveAt(Index);

				if (DependencyType != EAssetRegistryDependencyType::None)
				{
					// If this is a redirector replace with references
					TArray<FAssetData> Assets;
					AssetRegistry->GetAssetsByPackageName(PackageName, Assets, true);

					for (const FAssetData& Asset : Assets)
					{
						if (Asset.IsRedirector())
						{
							TArray<FAssetIdentifier> FoundReferences;

							if (bForwardDependency)
							{
								CurrentRegistrySource->RegistryState->GetDependencies(PackageName, FoundReferences, DependencyType);
							}
							else
							{
								CurrentRegistrySource->RegistryState->GetReferencers(PackageName, FoundReferences, DependencyType);
							}

							AssetIdentifiers.Insert(FoundReferences, Index);
							break;
						}
					}
				}

				// Need to redo this index, it was either removed or replaced
				Index--;
			}
		}
	}
	return bMadeChange;
}

void FAssetManagerEditorModule::InitializeRegistrySources(bool bNeedManagementData)
{
	if (CurrentRegistrySource == nullptr)
	{
		// Clear old list
		RegistrySourceMap.Reset();

		// Add Editor source
		FAssetManagerEditorRegistrySource EditorSource;
		EditorSource.SourceName = FAssetManagerEditorRegistrySource::EditorSourceName;
		EditorSource.bIsEditor = true;
		EditorSource.RegistryState = AssetRegistry->GetAssetRegistryState();

		RegistrySourceMap.Add(EditorSource.SourceName, EditorSource);

		TArray<ITargetPlatform*> Platforms = GetTargetPlatformManager()->GetTargetPlatforms();

		for (ITargetPlatform* CheckPlatform : Platforms)
		{
			FString RegistryPath = GetSavedAssetRegistryPath(CheckPlatform);

			if (!RegistryPath.IsEmpty())
			{
				FAssetManagerEditorRegistrySource PlatformSource;
				PlatformSource.SourceName = CheckPlatform->PlatformName();
				PlatformSource.SourceFilename = RegistryPath;
				PlatformSource.TargetPlatform = CheckPlatform;

				RegistrySourceMap.Add(PlatformSource.SourceName, PlatformSource);
			}
		}

		// Add Custom source
		FAssetManagerEditorRegistrySource CustomSource;
		CustomSource.SourceName = FAssetManagerEditorRegistrySource::CustomSourceName;

		RegistrySourceMap.Add(CustomSource.SourceName, CustomSource);

		// Select the Editor source by default
		CurrentRegistrySource = RegistrySourceMap.Find(EditorSource.SourceName);
	}
	check(CurrentRegistrySource);

	if (bNeedManagementData && !CurrentRegistrySource->bManagementDataInitialized)
	{
		SetCurrentRegistrySource(CurrentRegistrySource->SourceName);
	}
}

void FAssetManagerEditorModule::PerformAuditConsoleCommand(const TArray<FString>& Args)
{
	// Turn off as it makes diffing hard
	TGuardValue<ELogTimes::Type> DisableLogTimes(GPrintLogTimes, ELogTimes::None);

	UAssetManager::Get().UpdateManagementDatabase();

	// Now print assets with multiple labels
	LogAssetsWithMultipleLabels();
}

bool FAssetManagerEditorModule::GetDependencyTypeArg(const FString& Arg, EAssetRegistryDependencyType::Type& OutDepType)
{
	if (Arg.Compare(TEXT("-hardonly"), ESearchCase::IgnoreCase) == 0)
	{
		OutDepType = EAssetRegistryDependencyType::Hard;
		return true;
	}
	else if (Arg.Compare(TEXT("-softonly"), ESearchCase::IgnoreCase) == 0)
	{
		OutDepType = EAssetRegistryDependencyType::Soft;
		return true;
	}
	return false;
}

void FAssetManagerEditorModule::PerformDependencyChainConsoleCommand(const TArray<FString>& Args)
{
	if (Args.Num() < 2)
	{
		UE_LOG(LogAssetManagerEditor, Display, TEXT("FindDepChain given incorrect number of arguments.  Usage: %s"), FindDepChainHelpText);
		return;
	}

	FName TargetPath = FName(*Args[0].ToLower());
	FName SearchRoot = FName(*Args[1].ToLower());

	EAssetRegistryDependencyType::Type DependencyType = EAssetRegistryDependencyType::Packages;
	if (Args.Num() > 2)
	{
		GetDependencyTypeArg(Args[2], DependencyType);
	}

	FindReferenceChains(TargetPath, SearchRoot, DependencyType);
}

void FAssetManagerEditorModule::PerformDependencyClassConsoleCommand(const TArray<FString>& Args)
{
	if (Args.Num() < 2)
	{
		UE_LOG(LogAssetManagerEditor, Display, TEXT("FindDepClasses given incorrect number of arguments.  Usage: %s"), FindClassDepHelpText);
		return;
	}

	EAssetRegistryDependencyType::Type DependencyType = EAssetRegistryDependencyType::Packages;

	FName SourcePackagePath = FName(*Args[0].ToLower());
	TArray<FName> TargetClasses;
	for (int32 i = 1; i < Args.Num(); ++i)
	{
		if (!GetDependencyTypeArg(Args[i], DependencyType))
		{
			TargetClasses.AddUnique(FName(*Args[i]));
		}
	}

	TArray<FName> PackagesToSearch;

	//determine if the user passed us a package, or a directory
	TArray<FAssetData> PackageAssets;
	AssetRegistry->GetAssetsByPackageName(SourcePackagePath, PackageAssets);
	if (PackageAssets.Num() > 0)
	{
		PackagesToSearch.Add(SourcePackagePath);
	}
	else
	{
		TArray<FAssetData> AssetsInSearchPath;
		if (AssetRegistry->GetAssetsByPath(SourcePackagePath, /*inout*/ AssetsInSearchPath, /*bRecursive=*/ true))
		{
			for (const FAssetData& AssetData : AssetsInSearchPath)
			{
				PackagesToSearch.AddUnique(AssetData.PackageName);
			}
		}
	}

	for (FName SourcePackage : PackagesToSearch)
	{
		UE_LOG(LogAssetManagerEditor, Verbose, TEXT("FindDepClasses for: %s"), *SourcePackage.ToString());
		FindClassDependencies(SourcePackage, TargetClasses, DependencyType);
	}
}

bool FAssetManagerEditorModule::GetPackageDependencyChain(FName SourcePackage, FName TargetPackage, TArray<FName>& VisitedPackages, TArray<FName>& OutDependencyChain, EAssetRegistryDependencyType::Type DependencyType)
{
	//avoid crashing from circular dependencies.
	if (VisitedPackages.Contains(SourcePackage))
	{
		return false;
	}
	VisitedPackages.AddUnique(SourcePackage);

	if (SourcePackage == TargetPackage)
	{
		OutDependencyChain.Add(SourcePackage);
		return true;
	}

	TArray<FName> SourceDependencies;
	if (AssetRegistry->GetDependencies(SourcePackage, SourceDependencies, DependencyType) == false)
	{
		return false;
	}

	int32 DependencyCounter = 0;
	while (DependencyCounter < SourceDependencies.Num())
	{
		const FName& ChildPackageName = SourceDependencies[DependencyCounter];
		if (GetPackageDependencyChain(ChildPackageName, TargetPackage, VisitedPackages, OutDependencyChain, DependencyType))
		{
			OutDependencyChain.Add(SourcePackage);
			return true;
		}
		++DependencyCounter;
	}

	return false;
}

void FAssetManagerEditorModule::GetPackageDependenciesPerClass(FName SourcePackage, const TArray<FName>& TargetClasses, TArray<FName>& VisitedPackages, TArray<FName>& OutDependentPackages, EAssetRegistryDependencyType::Type DependencyType)
{
	//avoid crashing from circular dependencies.
	if (VisitedPackages.Contains(SourcePackage))
	{
		return;
	}
	VisitedPackages.AddUnique(SourcePackage);

	TArray<FName> SourceDependencies;
	if (AssetRegistry->GetDependencies(SourcePackage, SourceDependencies, DependencyType) == false)
	{
		return;
	}

	int32 DependencyCounter = 0;
	while (DependencyCounter < SourceDependencies.Num())
	{
		const FName& ChildPackageName = SourceDependencies[DependencyCounter];
		GetPackageDependenciesPerClass(ChildPackageName, TargetClasses, VisitedPackages, OutDependentPackages, DependencyType);
		++DependencyCounter;
	}

	FARFilter Filter;
	Filter.PackageNames.Add(SourcePackage);
	Filter.ClassNames = TargetClasses;
	Filter.bIncludeOnlyOnDiskAssets = true;

	TArray<FAssetData> PackageAssets;
	if (AssetRegistry->GetAssets(Filter, PackageAssets))
	{
		for (const FAssetData& AssetData : PackageAssets)
		{
			OutDependentPackages.AddUnique(SourcePackage);
			break;
		}
	}
}

void FAssetManagerEditorModule::FindReferenceChains(FName TargetPackageName, FName RootSearchPath, EAssetRegistryDependencyType::Type DependencyType)
{
	//find all the assets we think might depend on our target through some chain
	TArray<FAssetData> AssetsInSearchPath;
	AssetRegistry->GetAssetsByPath(RootSearchPath, /*inout*/ AssetsInSearchPath, /*bRecursive=*/ true);

	//consolidate assets into a unique set of packages for dependency searching. reduces redundant work.
	TArray<FName> SearchPackages;
	for (const FAssetData& AD : AssetsInSearchPath)
	{
		SearchPackages.AddUnique(AD.PackageName);
	}

	int32 CurrentFoundChain = 0;
	TArray<TArray<FName>> FoundChains;
	FoundChains.AddDefaulted(1);

	//try to find a dependency chain that links each of these packages to our target.
	TArray<FName> VisitedPackages;
	for (const FName& SearchPackage : SearchPackages)
	{
		VisitedPackages.Reset();
		if (GetPackageDependencyChain(SearchPackage, TargetPackageName, VisitedPackages, FoundChains[CurrentFoundChain], DependencyType))
		{
			++CurrentFoundChain;
			FoundChains.AddDefaulted(1);
		}
	}

	UE_LOG(LogAssetManagerEditor, Log, TEXT("Found %i, Dependency Chains to %s from directory %s"), CurrentFoundChain, *TargetPackageName.ToString(), *RootSearchPath.ToString());
	for (int32 ChainIndex = 0; ChainIndex < CurrentFoundChain; ++ChainIndex)
	{
		TArray<FName>& FoundChain = FoundChains[ChainIndex];
		UE_LOG(LogAssetManagerEditor, Log, TEXT("Chain %i"), ChainIndex);

		for (FName& Name : FoundChain)
		{
			UE_LOG(LogAssetManagerEditor, Log, TEXT("\t%s"), *Name.ToString());
		}
	}
}

void FAssetManagerEditorModule::FindClassDependencies(FName SourcePackageName, const TArray<FName>& TargetClasses, EAssetRegistryDependencyType::Type DependencyType)
{
	TArray<FAssetData> PackageAssets;
	if (!AssetRegistry->GetAssetsByPackageName(SourcePackageName, PackageAssets))
	{
		UE_LOG(LogAssetManagerEditor, Log, TEXT("Couldn't find source package %s. Abandoning class dep search.  "), *SourcePackageName.ToString());
		return;
	}

	TArray<FName> VisitedPackages;
	TArray<FName> DependencyPackages;
	GetPackageDependenciesPerClass(SourcePackageName, TargetClasses, VisitedPackages, DependencyPackages, DependencyType);

	if (DependencyPackages.Num() > 0)
	{
		UE_LOG(LogAssetManagerEditor, Log, TEXT("Found %i: dependencies for %s of the target classes"), DependencyPackages.Num(), *SourcePackageName.ToString());
		for (FName DependencyPackage : DependencyPackages)
		{
			UE_LOG(LogAssetManagerEditor, Log, TEXT("\t%s"), *DependencyPackage.ToString());
		}

		for (FName DependencyPackage : DependencyPackages)
		{
			TArray<FName> Chain;
			VisitedPackages.Reset();
			GetPackageDependencyChain(SourcePackageName, DependencyPackage, VisitedPackages, Chain, DependencyType);

			UE_LOG(LogAssetManagerEditor, Log, TEXT("Chain to package: %s"), *DependencyPackage.ToString());
			TArray<FAssetData> DepAssets;

			FARFilter Filter;
			Filter.PackageNames.Add(DependencyPackage);
			Filter.ClassNames = TargetClasses;
			Filter.bIncludeOnlyOnDiskAssets = true;

			if (AssetRegistry->GetAssets(Filter, DepAssets))
			{
				for (const FAssetData& DepAsset : DepAssets)
				{
					if (TargetClasses.Contains(DepAsset.AssetClass))
					{
						UE_LOG(LogAssetManagerEditor, Log, TEXT("Asset: %s class: %s"), *DepAsset.AssetName.ToString(), *DepAsset.AssetClass.ToString());
					}
				}
			}

			for (FName DepChainEntry : Chain)
			{
				UE_LOG(LogAssetManagerEditor, Log, TEXT("\t%s"), *DepChainEntry.ToString());
			}
		}
	}
}

void FAssetManagerEditorModule::WriteProfileFile(const FString& Extension, const FString& FileContents)
{
	const FString PathName = *(FPaths::ProfilingDir() + TEXT("AssetAudit/"));
	IFileManager::Get().MakeDirectory(*PathName);

	const FString Filename = CreateProfileFilename(Extension, true);
	const FString FilenameFull = PathName + Filename;

	UE_LOG(LogAssetManagerEditor, Log, TEXT("Saving %s"), *FPaths::ConvertRelativePathToFull(FilenameFull));
	FFileHelper::SaveStringToFile(FileContents, *FilenameFull);
}


void FAssetManagerEditorModule::LogAssetsWithMultipleLabels()
{
	UAssetManager& Manager = UAssetManager::Get();

	TMap<FName, TArray<FPrimaryAssetId>> PackageToLabelMap;
	TArray<FPrimaryAssetId> LabelNames;

	Manager.GetPrimaryAssetIdList(UAssetManager::PrimaryAssetLabelType, LabelNames);

	for (const FPrimaryAssetId& Label : LabelNames)
	{
		TArray<FName> LabeledPackages;

		Manager.GetManagedPackageList(Label, LabeledPackages);

		for (FName Package : LabeledPackages)
		{
			PackageToLabelMap.FindOrAdd(Package).AddUnique(Label);
		}
	}

	PackageToLabelMap.KeySort(TLess<FName>());

	UE_LOG(LogAssetManagerEditor, Log, TEXT("\nAssets with multiple labels follow"));

	// Print them out
	for (TPair<FName, TArray<FPrimaryAssetId>> Pair : PackageToLabelMap)
	{
		if (Pair.Value.Num() > 1)
		{
			FString TagString;
			for (const FPrimaryAssetId& Label : Pair.Value)
			{
				if (!TagString.IsEmpty())
				{
					TagString += TEXT(", ");
				}
				TagString += Label.ToString();
			}

			UE_LOG(LogAssetManagerEditor, Log, TEXT("%s has %s"), *Pair.Key.ToString(), *TagString);
		}		
	}
}

void FAssetManagerEditorModule::DumpAssetDependencies(const TArray<FString>& Args)
{
	if (!UAssetManager::IsValid())
	{
		return;
	}

	UAssetManager& Manager = UAssetManager::Get();
	TArray<FPrimaryAssetTypeInfo> TypeInfos;

	Manager.UpdateManagementDatabase();

	Manager.GetPrimaryAssetTypeInfoList(TypeInfos);

	TypeInfos.Sort([](const FPrimaryAssetTypeInfo& LHS, const FPrimaryAssetTypeInfo& RHS) { return LHS.PrimaryAssetType < RHS.PrimaryAssetType; });

	UE_LOG(LogAssetManagerEditor, Log, TEXT("=========== Asset Manager Dependencies ==========="));

	TArray<FString> ReportLines;

	ReportLines.Add(TEXT("digraph { "));

	for (const FPrimaryAssetTypeInfo& TypeInfo : TypeInfos)
	{
		struct FDependencyInfo
		{
			FName AssetName;
			FString AssetListString;

			FDependencyInfo(FName InAssetName, const FString& InAssetListString) : AssetName(InAssetName), AssetListString(InAssetListString) {}
		};

		TArray<FDependencyInfo> DependencyInfos;
		TArray<FPrimaryAssetId> PrimaryAssetIds;

		Manager.GetPrimaryAssetIdList(TypeInfo.PrimaryAssetType, PrimaryAssetIds);

		for (const FPrimaryAssetId& PrimaryAssetId : PrimaryAssetIds)
		{
			TArray<FAssetIdentifier> FoundDependencies;
			TArray<FString> DependencyStrings;

			AssetRegistry->GetDependencies(PrimaryAssetId, FoundDependencies, EAssetRegistryDependencyType::Manage);

			for (const FAssetIdentifier& Identifier : FoundDependencies)
			{
				FString ReferenceString = Identifier.ToString();
				DependencyStrings.Add(ReferenceString);

				ReportLines.Add(FString::Printf(TEXT("\t\"%s\" -> \"%s\";"), *PrimaryAssetId.ToString(), *ReferenceString));
			}

			DependencyStrings.Sort();

			DependencyInfos.Emplace(PrimaryAssetId.PrimaryAssetName, *FString::Join(DependencyStrings, TEXT(", ")));
		}

		if (DependencyInfos.Num() > 0)
		{
			UE_LOG(LogAssetManagerEditor, Log, TEXT("  Type %s:"), *TypeInfo.PrimaryAssetType.ToString());

			DependencyInfos.Sort([](const FDependencyInfo& LHS, const FDependencyInfo& RHS) { return LHS.AssetName < RHS.AssetName; });

			for (FDependencyInfo& DependencyInfo : DependencyInfos)
			{
				UE_LOG(LogAssetManagerEditor, Log, TEXT("    %s: depends on %s"), *DependencyInfo.AssetName.ToString(), *DependencyInfo.AssetListString);
			}
		}
	}

	ReportLines.Add(TEXT("}"));

	Manager.WriteCustomReport(FString::Printf(TEXT("PrimaryAssetReferences%s.gv"), *FDateTime::Now().ToString()), ReportLines);
}

bool FAssetManagerEditorModule::CreateOrEmptyCollection(FName CollectionName, ECollectionShareType::Type ShareType)
{
	ICollectionManager& CollectionManager = FCollectionManagerModule::GetModule().Get();

	if (CollectionManager.CollectionExists(CollectionName, ShareType))
	{
		return CollectionManager.EmptyCollection(CollectionName, ShareType);
	}
	else if (CollectionManager.CreateCollection(CollectionName, ShareType, ECollectionStorageMode::Static))
	{
		return true;
	}

	return false;
}

bool FAssetManagerEditorModule::WriteCollection(FName CollectionName, ECollectionShareType::Type ShareType, const TArray<FName>& PackageNames, bool bShowFeedback)
{
	ICollectionManager& CollectionManager = FCollectionManagerModule::GetModule().Get();
	FText ResultsMessage;
	bool bSuccess = false;

	TSet<FName> ObjectPathsToAddToCollection;
	for (FName PackageToAdd : PackageNames)
	{
		const FString PackageString = PackageToAdd.ToString();
		const FName ObjectPath = *FString::Printf(TEXT("%s.%s"), *PackageString, *FPackageName::GetLongPackageAssetName(PackageString));
		ObjectPathsToAddToCollection.Add(ObjectPath);
	}

	if (ObjectPathsToAddToCollection.Num() == 0)
	{
		UE_LOG(LogAssetManagerEditor, Log, TEXT("Nothing to add to collection %s"), *CollectionName.ToString());
		ResultsMessage = FText::Format(LOCTEXT("NothingToAddToCollection", "Nothing to add to collection {0}"), FText::FromName(CollectionName));
	}
	else if (CreateOrEmptyCollection(CollectionName, ShareType))
	{		
		if (CollectionManager.AddToCollection(CollectionName, ECollectionShareType::CST_Local, ObjectPathsToAddToCollection.Array()))
		{
			UE_LOG(LogAssetManagerEditor, Log, TEXT("Updated collection %s"), *CollectionName.ToString());
			ResultsMessage = FText::Format(LOCTEXT("CreateCollectionSucceeded", "Updated collection {0}"), FText::FromName(CollectionName));
			bSuccess = true;
		}
		else
		{
			UE_LOG(LogAssetManagerEditor, Warning, TEXT("Failed to update collection %s. %s"), *CollectionName.ToString(), *CollectionManager.GetLastError().ToString());
			ResultsMessage = FText::Format(LOCTEXT("AddToCollectionFailed", "Failed to add to collection {0}. {1}"), FText::FromName(CollectionName), CollectionManager.GetLastError());
		}
	}
	else
	{
		UE_LOG(LogAssetManagerEditor, Warning, TEXT("Failed to create collection %s. %s"), *CollectionName.ToString(), *CollectionManager.GetLastError().ToString());
		ResultsMessage = FText::Format(LOCTEXT("CreateCollectionFailed", "Failed to create collection {0}. {0}"), FText::FromName(CollectionName), CollectionManager.GetLastError());
	}

	if (bShowFeedback)
	{
		FMessageDialog::Open(EAppMsgType::Ok, ResultsMessage);
	}

	return bSuccess;
}
#undef LOCTEXT_NAMESPACE