// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PaperJsonImporterPrivatePCH.h"
#include "Paper2DClasses.h"
#include "PaperSpriteSheet.h"
#include "PaperSpriteSheetAssetTypeActions.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "AssetRegistryModule.h"
#include "PaperFlipbookFactory.h"
#include "PackageTools.h"
#include "PaperFlipbookHelpers.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

//////////////////////////////////////////////////////////////////////////
// FPaperSpriteSheetImportAssetTypeActions

FText FPaperSpriteSheetAssetTypeActions::GetName() const
{
	return LOCTEXT("FSpriteSheetAssetTypeActionsName", "Sprite Sheet");
}

FColor FPaperSpriteSheetAssetTypeActions::GetTypeColor() const
{
	return FColor(0, 255, 255);
}

UClass* FPaperSpriteSheetAssetTypeActions::GetSupportedClass() const
{
	return UPaperSpriteSheet::StaticClass();
}

uint32 FPaperSpriteSheetAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

void FPaperSpriteSheetAssetTypeActions::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	auto SpriteSheetImports = GetTypedWeakObjectPtrs<UPaperSpriteSheet>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SpriteSheet_Reimport", "Reimport"),
		LOCTEXT("SpriteSheet_ReimportTooltip", "Reimports the sprite sheet."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP(this, &FPaperSpriteSheetAssetTypeActions::ExecuteReimport, SpriteSheetImports),
		FCanExecuteAction()
		)
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SpriteSheet_CreateFlipbooks", "Create Flipbooks"),
		LOCTEXT("SpriteSheet_CreateFlipbooksTooltip", "Creates flipbooks from sprites in this sprite sheet."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP(this, &FPaperSpriteSheetAssetTypeActions::ExecuteCreateFlipbooks, SpriteSheetImports),
		FCanExecuteAction()
		)
	);
}

//////////////////////////////////////////////////////////////////////////

void FPaperSpriteSheetAssetTypeActions::ExecuteReimport(TArray<TWeakObjectPtr<UPaperSpriteSheet>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if (Object)
		{
			FReimportManager::Instance()->Reimport(Object, /*bAskForNewFileIfMissing=*/true);
		}
	}
}

static bool ExtractSpriteNumber(const FString& String, FString& BareString, int& Number)
{
	bool bExtracted = false;

	int LastCharacter = String.Len() - 1;
	if (LastCharacter >= 0)
	{
		// Find the last character that isn't a digit (Handle sprite names with numbers inside inverted commas / parentheses)
		while (LastCharacter >= 0 && !FChar::IsDigit(String[LastCharacter]))
		{
			LastCharacter--;
		}

		// Only proceed if we found a number in the sprite name
		if (LastCharacter >= 0)
		{
			while (LastCharacter > 0 && FChar::IsDigit(String[LastCharacter - 1]))
			{
				LastCharacter--;
			}

			if (LastCharacter >= 0)
			{
				int FirstDigit = LastCharacter;
				int EndCharacter = FirstDigit;
				while (EndCharacter > 0 && !FChar::IsAlnum(String[EndCharacter - 1]))
				{
					EndCharacter--;
				}

				if (EndCharacter == 0)
				{
					// This string consists of non alnum + number, eg. _42
					// The flipbook / category name in this case will be _
					// Otherwise, we strip out all trailing non-alnum chars
					EndCharacter = FirstDigit;
				}

				FString NumberString = String.Mid(FirstDigit);
				BareString = String.Left(EndCharacter);
				Number = FCString::Atoi(*NumberString);
				bExtracted = true;
			}
		}
	}

	if (!bExtracted)
	{
		BareString = String;
		Number = -1;
	}
	return bExtracted;
}

void FPaperSpriteSheetAssetTypeActions::ExecuteCreateFlipbooks(TArray<TWeakObjectPtr<UPaperSpriteSheet>> Objects)
{	
	for (int SpriteSheetIndex = 0; SpriteSheetIndex < Objects.Num(); ++SpriteSheetIndex)
	{
		UPaperSpriteSheet* SpriteSheet = Objects[SpriteSheetIndex].Get();
		if (SpriteSheet != nullptr)
		{
			const FString PackagePath = FPackageName::GetLongPackagePath(SpriteSheet->GetOutermost()->GetPathName());

			FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
			FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			
			check(SpriteSheet->SpriteNames.Num() == SpriteSheet->Sprites.Num());
			bool useSpriteNames = (SpriteSheet->SpriteNames.Num() == SpriteSheet->Sprites.Num());

			// Create a list of sprites and sprite names to feed into paper flipbook helpers
			TArray<UPaperSprite*> Sprites;
			TArray<FString> SpriteNames;

			for (int SpriteIndex = 0; SpriteIndex < SpriteSheet->Sprites.Num(); ++SpriteIndex)
			{
				auto SpriteAssetPtr = SpriteSheet->Sprites[SpriteIndex];
				FStringAssetReference SpriteStringRef = SpriteAssetPtr.ToStringReference();
				UPaperSprite* Sprite = nullptr;
				if (!SpriteStringRef.ToString().IsEmpty())
				{
					Sprite = Cast<UPaperSprite>(StaticLoadObject(UPaperSprite::StaticClass(), NULL, *SpriteStringRef.ToString(), NULL, LOAD_None, NULL));
				}
				if (Sprite != nullptr)
				{
					const FString SpriteName = useSpriteNames ? SpriteSheet->SpriteNames[SpriteIndex] : Sprite->GetName();
					Sprites.Add(Sprite);
					SpriteNames.Add(SpriteName);
				}
			}

			TMap<FString, TArray<UPaperSprite*> > SpriteFlipbookMap;
			FPaperFlipbookHelpers::ExtractFlipbooksFromSprites(/*out*/SpriteFlipbookMap, Sprites, SpriteNames);

			// Create one flipbook for every grouped flipbook name
			if (SpriteFlipbookMap.Num() > 0)
			{
				UPaperFlipbookFactory* FlipbookFactory = ConstructObject<UPaperFlipbookFactory>(UPaperFlipbookFactory::StaticClass());

				GWarn->BeginSlowTask(NSLOCTEXT("Paper2D", "Paper2D_CreateFlipbooks", "Creating flipbooks from selection"), true, true);

				int Progress = 0;
				int TotalProgress = SpriteFlipbookMap.Num();
				TArray<UObject*> ObjectsToSync;
				for (auto It : SpriteFlipbookMap)
				{
					GWarn->UpdateProgress(Progress++, TotalProgress);

					const FString& FlipbookName = It.Key;
					TArray<UPaperSprite*>& Sprites = It.Value;

					const FString TentativePackagePath = PackageTools::SanitizePackageName(PackagePath + TEXT("/") + FlipbookName);
					FString DefaultSuffix;
					FString AssetName;
					FString PackageName;
					AssetToolsModule.Get().CreateUniqueAssetName(TentativePackagePath, /*out*/ DefaultSuffix, /*out*/ PackageName, /*out*/ AssetName);

					FlipbookFactory->KeyFrames.Empty();
					for (int32 SpriteIndex = 0; SpriteIndex < Sprites.Num(); ++SpriteIndex)
					{
						FPaperFlipbookKeyFrame* KeyFrame = new (FlipbookFactory->KeyFrames) FPaperFlipbookKeyFrame();
						KeyFrame->Sprite = Sprites[SpriteIndex];
						KeyFrame->FrameRun = 1;
					}

					if (UObject* NewAsset = AssetToolsModule.Get().CreateAsset(AssetName, PackagePath, UPaperFlipbook::StaticClass(), FlipbookFactory))
					{
						ObjectsToSync.Add(NewAsset);
					}

					if (GWarn->ReceivedUserCancel())
					{
						break;
					}
				}

				GWarn->EndSlowTask();

				if (ObjectsToSync.Num() > 0)
				{
					ContentBrowserModule.Get().SyncBrowserToAssets(ObjectsToSync);
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE

