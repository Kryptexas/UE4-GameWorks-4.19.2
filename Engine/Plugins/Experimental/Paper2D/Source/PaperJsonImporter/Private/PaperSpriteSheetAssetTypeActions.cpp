// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PaperJsonImporterPrivatePCH.h"
#include "Paper2DClasses.h"
#include "PaperSpriteSheet.h"
#include "PaperSpriteSheetAssetTypeActions.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "AssetRegistryModule.h"
#include "PaperFlipbookFactory.h"

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
	if (LastCharacter >= 0 && FChar::IsDigit(String[LastCharacter]))
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
			
			TMap<FString, UPaperSprite*> SpriteNameMap;
			TSet<FString> FlipbookNames;
			TMap<FString, TArray<UPaperSprite*> > FlipbookSprites;
			TArray<UPaperSprite*> AllValidSprites;

			check(SpriteSheet->SpriteNames.Num() == SpriteSheet->Sprites.Num());
			bool useSpriteNames = (SpriteSheet->SpriteNames.Num() == SpriteSheet->Sprites.Num());

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

					SpriteNameMap.Add(SpriteName, Sprite);
					
					int SpriteNumber = 0;
					FString SpriteBareString;
					if (ExtractSpriteNumber(SpriteName, /*out*/SpriteBareString, /*out*/SpriteNumber))
					{
						FlipbookNames.Add(SpriteBareString);
						if (FlipbookSprites.Contains(SpriteBareString))
						{
							FlipbookSprites[SpriteBareString].Add(Sprite);
						}
						else
						{
							FlipbookSprites.Add(SpriteBareString, TArray<UPaperSprite*>());
						}
					}

					AllValidSprites.Add(Sprite);
				}
			}

			// Natural sort using the same method as above
			struct FSpriteSortPredicate
			{
				FSpriteSortPredicate() {}

				// Sort predicate operator
				bool operator()(UPaperSprite& LHS, UPaperSprite& RHS) const
				{
					FString LeftString;
					int LeftNumber;
					ExtractSpriteNumber(LHS.GetName(), /*out*/LeftString, /*out*/LeftNumber);

					FString RightString;
					int RightNumber;
					ExtractSpriteNumber(RHS.GetName(), /*out*/RightString, /*out*/RightNumber);

					return (LeftString == RightString) ? LeftNumber < RightNumber : LeftString < RightString;
				}
			};

			// Haven't matched any existing names, so create one flipbook with all sprites in it
			if (FlipbookNames.Num() == 0 &&
				EAppReturnType::Yes == FMessageDialog::Open(EAppMsgType::YesNo, NSLOCTEXT("Paper2D", "Paper2D_SpriteSheetCreateOneFlipbook", "Unable to detect flipbooks by name in this sprite sheet. Proceed to create a single flipbook containing all sprites in this sprite sheet?")) )
			{
				const FString& DefaultFlipbookName = SpriteSheet->GetName() + TEXT(" Flipbook");
				FlipbookNames.Add(DefaultFlipbookName);
				FlipbookSprites.Add(DefaultFlipbookName, AllValidSprites);
			}

			// Create one flipbook for every grouped flipbook name
			if (FlipbookNames.Num() > 0)
			{
				UPaperFlipbookFactory* FlipbookFactory = ConstructObject<UPaperFlipbookFactory>(UPaperFlipbookFactory::StaticClass());

				GWarn->BeginSlowTask(NSLOCTEXT("Paper2D", "Paper2D_SpriteSheetCreateFlipbooks", "Creating flipbooks from Sprite Sheet"), true, true);

				TArray<FString> FlipbookNameArray = FlipbookNames.Array();
				TArray<UObject*> ObjectsToSync;
				for (int FlipbookNameIndex = 0; FlipbookNameIndex < FlipbookNames.Num(); ++FlipbookNameIndex)
				{
					GWarn->UpdateProgress(FlipbookNameIndex, FlipbookNames.Num());

					const FString& FlipbookName = FlipbookNameArray[FlipbookNameIndex];
					TArray<UPaperSprite*>& Sprites = FlipbookSprites[FlipbookName];
					Sprites.Sort(FSpriteSortPredicate());

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

