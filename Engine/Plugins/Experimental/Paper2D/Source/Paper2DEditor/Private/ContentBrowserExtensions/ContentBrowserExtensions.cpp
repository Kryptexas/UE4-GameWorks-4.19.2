// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "ContentBrowserModule.h"
#include "ContentBrowserExtensions.h"

#define LOCTEXT_NAMESPACE "Paper2D"

DECLARE_LOG_CATEGORY_EXTERN(LogPaperCBExtensions, Log, All);
DEFINE_LOG_CATEGORY(LogPaperCBExtensions);

//////////////////////////////////////////////////////////////////////////

FContentBrowserMenuExtender_SelectedAssets ContentBrowserExtenderDelegate;

//////////////////////////////////////////////////////////////////////////
// FContentBrowserSelectedAssetExtensionBase

struct FContentBrowserSelectedAssetExtensionBase
{
public:
	TArray<class FAssetData> SelectedAssets;

public:
	virtual void Execute() {}
	virtual ~FContentBrowserSelectedAssetExtensionBase() {}
};

//////////////////////////////////////////////////////////////////////////
// FCreateSpriteFromTextureExtension

#include "AssetToolsModule.h"
#include "AssetRegistryModule.h"

struct FCreateSpriteFromTextureExtension : public FContentBrowserSelectedAssetExtensionBase
{
	void CreateSpritesFromTextures(TArray<UTexture2D*>& Textures)
	{
		const FString DefaultSuffix = TEXT("_Sprite");

		FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

		const bool bOneTextureSelected = Textures.Num() == 1;
		TArray<UObject*> ObjectsToSync;

		for (auto TextureIt = Textures.CreateConstIterator(); TextureIt; ++TextureIt)
		{
			UTexture2D* Texture = *TextureIt;

			// Get a unique name for the sprite
			FString Name;
			FString PackageName;
			AssetToolsModule.Get().CreateUniqueAssetName(Texture->GetOutermost()->GetName(), DefaultSuffix, /*out*/ PackageName, /*out*/ Name);
			const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);


			// Create the factory used to generate the sprite
			UPaperSpriteFactory* SpriteFactory = ConstructObject<UPaperSpriteFactory>(UPaperSpriteFactory::StaticClass());
			SpriteFactory->InitialTexture = Texture;

			// Create the sprite
			if (bOneTextureSelected)
			{
				ContentBrowserModule.Get().CreateNewAsset(Name, PackagePath, UPaperSprite::StaticClass(), SpriteFactory);
			}
			else
			{
				if (UObject* NewAsset = AssetToolsModule.Get().CreateAsset(Name, PackagePath, UPaperSprite::StaticClass(), SpriteFactory))
				{
					ObjectsToSync.Add(NewAsset);
				}
			}
		}

		if (ObjectsToSync.Num() > 0)
		{
			ContentBrowserModule.Get().SyncBrowserToAssets(ObjectsToSync);
		}
	}

	virtual void Execute() OVERRIDE
	{
		// Create sprites for any selected textures
		TArray<UTexture2D*> Textures;
		for (auto AssetIt = SelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
		{
			const FAssetData& AssetData = *AssetIt;
			if (UTexture2D* Texture = Cast<UTexture2D>(AssetData.GetAsset()))
			{
				Textures.Add(Texture);
			}
		}

		CreateSpritesFromTextures(Textures);
	}
};

struct FConfigureTexturesForSpriteUsageExtension : public FContentBrowserSelectedAssetExtensionBase
{
	virtual void Execute() OVERRIDE
	{
		// Change the compression settings and trigger a recompress
		for (auto AssetIt = SelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
		{
			const FAssetData& AssetData = *AssetIt;
			if (UTexture2D* Texture = Cast<UTexture2D>(AssetData.GetAsset()))
			{
				Texture->Modify();
				Texture->LODGroup = TEXTUREGROUP_UI;
				Texture->CompressionSettings = TC_EditorIcon;
				Texture->Filter = TF_Nearest;
				Texture->PostEditChange();
			}
		}
	}
};

//////////////////////////////////////////////////////////////////////////
// FPaperContentBrowserExtensions_Impl

class FPaperContentBrowserExtensions_Impl
{
public:
	static void ExecuteSelectedContentFunctor(TSharedPtr<FContentBrowserSelectedAssetExtensionBase> SelectedAssetFunctor)
	{
		SelectedAssetFunctor->Execute();
	}

	static void CreateSpriteTextureMenuOptions(FMenuBuilder& Builder, TSharedPtr<FCreateSpriteFromTextureExtension> SpriteCreatorFunctor)
	{
		FUIAction Action_CreateSpritesFromTextures(
			FExecuteAction::CreateStatic(&FPaperContentBrowserExtensions_Impl::ExecuteSelectedContentFunctor, StaticCastSharedPtr<FContentBrowserSelectedAssetExtensionBase>(SpriteCreatorFunctor)));
		Builder.AddMenuEntry(
			LOCTEXT("CB_Extension_Texture_CreateSprite", "Create Sprite"), 
			LOCTEXT("CB_Extension_Texture_CreateSprite_Tooltip", "Create sprites from selected textures"),
			FSlateIcon(),
			Action_CreateSpritesFromTextures,
			NAME_None,
			EUserInterfaceActionType::Button);
	}

	static void CreateTextureSetupMenuOption(FMenuBuilder& Builder, TSharedPtr<FConfigureTexturesForSpriteUsageExtension> ConfigureFunctor)
	{
		FUIAction Action_ConfigureTexturesForSprites(
			FExecuteAction::CreateStatic(&FPaperContentBrowserExtensions_Impl::ExecuteSelectedContentFunctor, StaticCastSharedPtr<FContentBrowserSelectedAssetExtensionBase>(ConfigureFunctor)));
		Builder.AddMenuEntry(
			LOCTEXT("CB_Extension_Texture_ConfigureTextureForSprites", "Configure For Sprites"), 
			LOCTEXT("CB_Extension_Texture_ConfigureTextureForSprites_Tooltip", "Sets compression settings and sampling modes to good defaults for sprites"),
			FSlateIcon(),
			Action_ConfigureTexturesForSprites,
			NAME_None,
			EUserInterfaceActionType::Button);
	}

	

	static TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets)
	{
		TSharedRef<FExtender> Extender(new FExtender());

		// Run thru the assets to determine if any meet our criteria
		bool bAnyTextures = false;
		for (auto AssetIt = SelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
		{
			const FAssetData& Asset = *AssetIt;
			bAnyTextures = bAnyTextures || (Asset.AssetClass == UTexture2D::StaticClass()->GetFName());
		}

		if (bAnyTextures)
		{
			TSharedPtr<FCreateSpriteFromTextureExtension> SpriteCreatorFunctor = MakeShareable(new FCreateSpriteFromTextureExtension());
			SpriteCreatorFunctor->SelectedAssets = SelectedAssets;

			Extender->AddMenuExtension(
				"GetAssetActions",
				EExtensionHook::After,
				NULL,
				FMenuExtensionDelegate::CreateStatic(&FPaperContentBrowserExtensions_Impl::CreateSpriteTextureMenuOptions, SpriteCreatorFunctor));

			//@TODO: Already getting nasty, need to refactor the data to be independent of the functor
			TSharedPtr<FConfigureTexturesForSpriteUsageExtension> TextureConfigFunctor = MakeShareable(new FConfigureTexturesForSpriteUsageExtension());
			TextureConfigFunctor->SelectedAssets = SelectedAssets;

			Extender->AddMenuExtension(
				"GetAssetActions",
				EExtensionHook::After,
				NULL,
				FMenuExtensionDelegate::CreateStatic(&FPaperContentBrowserExtensions_Impl::CreateTextureSetupMenuOption, TextureConfigFunctor));
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

void FPaperContentBrowserExtensions::InstallHooks()
{
	ContentBrowserExtenderDelegate = FContentBrowserMenuExtender_SelectedAssets::CreateStatic(&FPaperContentBrowserExtensions_Impl::OnExtendContentBrowserAssetSelectionMenu);

	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = FPaperContentBrowserExtensions_Impl::GetExtenderDelegates();
	CBMenuExtenderDelegates.Add(ContentBrowserExtenderDelegate);
}

void FPaperContentBrowserExtensions::RemoveHooks()
{
	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = FPaperContentBrowserExtensions_Impl::GetExtenderDelegates();
	CBMenuExtenderDelegates.Remove(ContentBrowserExtenderDelegate);
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE