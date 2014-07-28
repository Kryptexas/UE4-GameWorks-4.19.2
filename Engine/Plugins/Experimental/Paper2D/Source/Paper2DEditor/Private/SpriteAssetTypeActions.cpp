// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"

#include "SpriteAssetTypeActions.h"
#include "SpriteEditor/SpriteEditor.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

//////////////////////////////////////////////////////////////////////////
// FSpriteAssetTypeActions

FText FSpriteAssetTypeActions::GetName() const
{
	return LOCTEXT("FSpriteAssetTypeActionsName", "Sprite");
}

FColor FSpriteAssetTypeActions::GetTypeColor() const
{
	return FColor(0, 255, 255);
}

UClass* FSpriteAssetTypeActions::GetSupportedClass() const
{
	return UPaperSprite::StaticClass();
}

void FSpriteAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (UPaperSprite* Sprite = Cast<UPaperSprite>(*ObjIt))
		{
			TSharedRef<FSpriteEditor> NewSpriteEditor(new FSpriteEditor());
			NewSpriteEditor->InitSpriteEditor(Mode, EditWithinLevelEditor, Sprite);
		}
	}
}

uint32 FSpriteAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

void FSpriteAssetTypeActions::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	auto Sprites = GetTypedWeakObjectPtrs<UPaperSprite>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Sprite_CreateFlipbook", "Create Flipbook"),
		LOCTEXT("Sprite_CreateFlipbookTooltip", "Creates a flipbook from the selected sprites."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP(this, &FSpriteAssetTypeActions::ExecuteCreateFlipbook, Sprites),
		FCanExecuteAction()
		)
		);
}

//////////////////////////////////////////////////////////////////////////


void FSpriteAssetTypeActions::ExecuteCreateFlipbook(TArray<TWeakObjectPtr<UPaperSprite>> Objects)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<UPaperSprite*> Sprites;

	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		UPaperSprite *Object = (*ObjIt).Get();
		if (Object && Object->IsValidLowLevel())
		{
			Sprites.Add(Object);
		}
	}

	// Attempt to natural sort if possible
	// Cache results if it becomes a bottleneck
	struct FSpriteSortPredicate
	{
		FSpriteSortPredicate() {}

		void ExtractNumber(const FString& String, FString& BareString, int& Number) const
		{
			int SplitPoint = 0;
			for (int i = 0; i < String.Len(); ++i)
			{
				const TCHAR CurrentChar = String[i];
				if (CurrentChar >= TEXT('0') && CurrentChar <= TEXT('9'))
				{
					SplitPoint = i;
					break;
				}
			}
			if (SplitPoint > 0)
			{
				FString NumberString = String.Mid(SplitPoint);
				if (NumberString.IsNumeric())
				{
					// Valid string with number
					BareString = String.Left(SplitPoint);
					Number = FCString::Atoi(*NumberString);
					return;
				}
			}
		
			BareString = String;
			Number = -1;
		}

		// Sort predicate operator
		bool operator()(UPaperSprite& LHS, UPaperSprite& RHS) const
		{
			FString LeftString;
			int LeftNumber;
			ExtractNumber(LHS.GetName(), /*out*/LeftString, /*out*/LeftNumber);

			FString RightString;
			int RightNumber;
			ExtractNumber(RHS.GetName(), /*out*/RightString, /*out*/RightNumber);

			return (LeftString == RightString) ? LeftNumber < RightNumber : LeftString < RightString;
		}
	};
	Sprites.Sort(FSpriteSortPredicate());
	
	// Create the flipbook
	if (Sprites.Num() > 0)
	{
		const FString SpritePathName = Sprites[0]->GetOutermost()->GetPathName();
		const FString LongPackagePath = FPackageName::GetLongPackagePath(Sprites[0]->GetOutermost()->GetPathName());

		const FString NewFlipBookDefaultPath = LongPackagePath + TEXT("/NewFlipbook");
		FString DefaultSuffix;
		FString AssetName;
		FString PackageName;
		AssetToolsModule.Get().CreateUniqueAssetName(NewFlipBookDefaultPath, /*out*/ DefaultSuffix, /*out*/ PackageName, /*out*/ AssetName);
		const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);

		UPaperFlipbookFactory* FlipbookFactory = ConstructObject<UPaperFlipbookFactory>(UPaperFlipbookFactory::StaticClass());
		for (int32 SpriteIndex = 0; SpriteIndex < Sprites.Num(); ++SpriteIndex)
		{
			UPaperSprite* Sprite = Sprites[SpriteIndex];
			FPaperFlipbookKeyFrame* KeyFrame = new (FlipbookFactory->KeyFrames) FPaperFlipbookKeyFrame();
			KeyFrame->Sprite = Sprite;
			KeyFrame->FrameRun = 1;
		}

		ContentBrowserModule.Get().CreateNewAsset(AssetName, PackagePath, UPaperFlipbook::StaticClass(), FlipbookFactory);
	}
}


#undef LOCTEXT_NAMESPACE
