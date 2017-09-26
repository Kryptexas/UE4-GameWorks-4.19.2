// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ContentBrowserExtensions/ContentBrowserExtensions.h"
#include "Modules/ModuleManager.h"
#include "Misc/PackageName.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyleSet.h"
#include "AssetData.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"

#include "Engine/StaticMesh.h"
#include "FlexStaticMesh.h"

#define LOCTEXT_NAMESPACE "Flex"

DECLARE_LOG_CATEGORY_EXTERN(LogFlexCBExtensions, Log, All);
DEFINE_LOG_CATEGORY(LogFlexCBExtensions);

//////////////////////////////////////////////////////////////////////////

FContentBrowserMenuExtender_SelectedAssets ContentBrowserExtenderDelegate;
FDelegateHandle ContentBrowserExtenderDelegateHandle;

//////////////////////////////////////////////////////////////////////////
// FContentBrowserSelectedAssetExtensionBase

struct FContentBrowserSelectedAssetExtensionBase
{
public:
	TArray<struct FAssetData> SelectedAssets;

public:
	virtual void Execute() {}
	virtual ~FContentBrowserSelectedAssetExtensionBase() {}
};

//////////////////////////////////////////////////////////////////////////
// FCreateSpriteFromTextureExtension

#include "IAssetTools.h"
#include "AssetToolsModule.h"

struct FConvertStaticToFlexStaticExtension : public FContentBrowserSelectedAssetExtensionBase
{
	void ConvertStaticMeshesToFlex(TArray<UStaticMesh*>& StaticMeshes)
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

		TArray<UObject*> Assets;

		for (UStaticMesh* StaticMesh : StaticMeshes)
		{
			FString Name;
			FString PackageName;

			const FString DefaultSuffix = TEXT("_Flex");
			AssetToolsModule.Get().CreateUniqueAssetName(StaticMesh->GetOutermost()->GetName(), DefaultSuffix, /*out*/ PackageName, /*out*/ Name);

			const FString PackageNameC = FPackageName::GetLongPackagePath(PackageName) + TEXT("/") + Name;

			UPackage* Pkg = CreatePackage(nullptr, *PackageNameC);

			UFlexStaticMesh* FSM = Cast<UFlexStaticMesh>(StaticDuplicateObject(StaticMesh, Pkg, *Name, RF_AllFlags, UFlexStaticMesh::StaticClass()));

			Assets.Add(FSM);
		}

		if (Assets.Num())
		{
			ContentBrowserModule.Get().SyncBrowserToAssets(Assets, /*bAllowLockedBrowsers=*/true);
		}
	}

	virtual void Execute() override
	{
		UE_LOG(LogFlexCBExtensions, Log, TEXT("Convert static to flex static"));

		// Create sprites for any selected textures
		TArray<UStaticMesh*> StaticMeshes;

		for (auto AssetIt = SelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
		{
			const FAssetData& AssetData = *AssetIt;
			if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetData.GetAsset()))
			{
				StaticMeshes.Add(StaticMesh);
			}
		}

		ConvertStaticMeshesToFlex(StaticMeshes);
	}
};

//////////////////////////////////////////////////////////////////////////
// FFlexContentBrowserExtensions_Impl

class FFlexContentBrowserExtensions_Impl
{
public:
	static void ExecuteSelectedContentFunctor(TSharedPtr<FContentBrowserSelectedAssetExtensionBase> SelectedAssetFunctor)
	{
		SelectedAssetFunctor->Execute();
	}

	static void CreateStaticMeshActionsMenuEntries(FMenuBuilder& MenuBuilder, TArray<FAssetData> SelectedAssets)
	{
		TSharedPtr<FConvertStaticToFlexStaticExtension> ConvertFunctor = MakeShareable(new FConvertStaticToFlexStaticExtension());
		ConvertFunctor->SelectedAssets = SelectedAssets;

		FUIAction Action_ConvertStaticToFlex(
			FExecuteAction::CreateStatic(&FFlexContentBrowserExtensions_Impl::ExecuteSelectedContentFunctor, StaticCastSharedPtr<FContentBrowserSelectedAssetExtensionBase>(ConvertFunctor))
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("CB_Extension_ConvertStaticToFlexStatic", "Create Flex Static Mesh"),
			LOCTEXT("CB_Extension_ConvertStaticToFlexStatic_Tooltip", "Create Flex Static Meshes from selected Static Meshes"),
			FSlateIcon(),
			Action_ConvertStaticToFlex,
			NAME_None,
			EUserInterfaceActionType::Button
		);
	}

	static TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets)
	{
		TSharedRef<FExtender> Extender(new FExtender());

		// Run thru the assets to determine if any meet our criteria
		bool bAnyStaticMesh = false;
		for (auto AssetIt = SelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
		{
			const FAssetData& Asset = *AssetIt;
			bAnyStaticMesh = bAnyStaticMesh || (Asset.AssetClass == UStaticMesh::StaticClass()->GetFName());
		}

		if (bAnyStaticMesh)
		{
			// Add the StaticMesh actions sub-menu extender
			Extender->AddMenuExtension(
				"GetAssetActions",
				EExtensionHook::After,
				nullptr,
				FMenuExtensionDelegate::CreateStatic(&FFlexContentBrowserExtensions_Impl::CreateStaticMeshActionsMenuEntries, SelectedAssets));
		}

		return Extender;
	}

	static TArray<FContentBrowserMenuExtender_SelectedAssets>& GetExtenderDelegates()
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
		return ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	}
};

//////////////////////////////////////////////////////////////////////////
// FPaperContentBrowserExtensions

void FFlexContentBrowserExtensions::InstallHooks()
{
	ContentBrowserExtenderDelegate = FContentBrowserMenuExtender_SelectedAssets::CreateStatic(&FFlexContentBrowserExtensions_Impl::OnExtendContentBrowserAssetSelectionMenu);

	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = FFlexContentBrowserExtensions_Impl::GetExtenderDelegates();
	CBMenuExtenderDelegates.Add(ContentBrowserExtenderDelegate);
	ContentBrowserExtenderDelegateHandle = CBMenuExtenderDelegates.Last().GetHandle();
}

void FFlexContentBrowserExtensions::RemoveHooks()
{
	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = FFlexContentBrowserExtensions_Impl::GetExtenderDelegates();
	CBMenuExtenderDelegates.RemoveAll([](const FContentBrowserMenuExtender_SelectedAssets& Delegate){ return Delegate.GetHandle() == ContentBrowserExtenderDelegateHandle; });
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
