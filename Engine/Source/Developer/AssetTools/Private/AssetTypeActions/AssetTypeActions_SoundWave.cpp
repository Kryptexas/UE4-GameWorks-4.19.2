// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "AssetToolsModule.h"
#include "SoundDefinitions.h"
#include "ContentBrowserModule.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundCue.h"
#include "Sound/DialogueWave.h"
#include "ContentBrowserDelegates.h"
#include "PropertyCustomizationHelpers.h"
#include "Factories/DialogueWaveFactory.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

UClass* FAssetTypeActions_SoundWave::GetSupportedClass() const
{
	return USoundWave::StaticClass();
}

void FAssetTypeActions_SoundWave::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	FAssetTypeActions_SoundBase::GetActions(InObjects, MenuBuilder);

	auto SoundNodes = GetTypedWeakObjectPtrs<USoundWave>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SoundWave_CreateCue", "Create Cue"),
		LOCTEXT("SoundWave_CreateCueTooltip", "Creates a sound cue using this sound wave."),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.SoundCue"),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_SoundWave::ExecuteCreateSoundCue, SoundNodes ),
		FCanExecuteAction()
			)
		);

	MenuBuilder.AddSubMenu(
		LOCTEXT("SoundWave_CreateDialogue", "Create Dialogue"),
		LOCTEXT("SoundWave_CreateDialogueTooltip", "Creates a dialogue wave using this sound wave."),
		FNewMenuDelegate::CreateSP(this, &FAssetTypeActions_SoundWave::FillVoiceMenu, SoundNodes));
}

void FAssetTypeActions_SoundWave::GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const
{
	for (auto& Asset : TypeAssets)
	{
		const auto SoundWave = CastChecked<USoundWave>(Asset);
		SoundWave->AssetImportData->ExtractFilenames(OutSourceFilePaths);
	}
}

void FAssetTypeActions_SoundWave::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);
}

void FAssetTypeActions_SoundWave::ExecuteCreateSoundCue(TArray<TWeakObjectPtr<USoundWave>> Objects)
{
	const FString DefaultSuffix = TEXT("_Cue");

	if ( Objects.Num() == 1 )
	{
		auto Object = Objects[0].Get();

		if ( Object )
		{
			// Determine an appropriate name
			FString Name;
			FString PackagePath;
			CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackagePath, Name);

			// Create the factory used to generate the asset
			USoundCueFactoryNew* Factory = NewObject<USoundCueFactoryNew>();
			Factory->InitialSoundWave = Object;

			FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			ContentBrowserModule.Get().CreateNewAsset(Name, FPackageName::GetLongPackagePath(PackagePath), USoundCue::StaticClass(), Factory);
		}
	}
	else
	{
		TArray<UObject*> ObjectsToSync;

		for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
		{
			auto Object = (*ObjIt).Get();
			if ( Object )
			{
				FString Name;
				FString PackageName;
				CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackageName, Name);

				// Create the factory used to generate the asset
				USoundCueFactoryNew* Factory = NewObject<USoundCueFactoryNew>();
				Factory->InitialSoundWave = Object;

				FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
				UObject* NewAsset = AssetToolsModule.Get().CreateAsset(Name, FPackageName::GetLongPackagePath(PackageName), USoundCue::StaticClass(), Factory);

				if ( NewAsset )
				{
					ObjectsToSync.Add(NewAsset);
				}
			}
		}

		if ( ObjectsToSync.Num() > 0 )
		{
			FAssetTools::Get().SyncBrowserToAssets(ObjectsToSync);
		}
	}
}

void FAssetTypeActions_SoundWave::ExecuteCreateDialogueWave(const class FAssetData& AssetData, TArray<TWeakObjectPtr<USoundWave>> Objects)
{
	const FString DefaultSuffix = TEXT("_Dialogue");

	UDialogueVoice* DialogueVoice = Cast<UDialogueVoice>(AssetData.GetAsset());

	if (Objects.Num() == 1)
	{
		auto Object = Objects[0].Get();

		if (Object)
		{
			// Determine an appropriate name
			FString Name;
			FString PackagePath;
			CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackagePath, Name);

			// Create the factory used to generate the asset
			UDialogueWaveFactory* Factory = NewObject<UDialogueWaveFactory>();
			Factory->InitialSoundWave = Object;
			Factory->InitialSpeakerVoice = DialogueVoice;
			Factory->HasSetInitialTargetVoice = true;

			FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			ContentBrowserModule.Get().CreateNewAsset(Name, FPackageName::GetLongPackagePath(PackagePath), UDialogueWave::StaticClass(), Factory);
		}
	}
	else
	{
		TArray<UObject*> ObjectsToSync;

		for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
		{
			auto Object = (*ObjIt).Get();
			if (Object)
			{
				FString Name;
				FString PackageName;
				CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackageName, Name);

				// Create the factory used to generate the asset
				UDialogueWaveFactory* Factory = NewObject<UDialogueWaveFactory>();
				Factory->InitialSoundWave = Object;
				Factory->InitialSpeakerVoice = DialogueVoice;
				Factory->HasSetInitialTargetVoice = true;

				FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
				UObject* NewAsset = AssetToolsModule.Get().CreateAsset(Name, FPackageName::GetLongPackagePath(PackageName), UDialogueWave::StaticClass(), Factory);

				if (NewAsset)
				{
					ObjectsToSync.Add(NewAsset);
				}
			}
		}

		if (ObjectsToSync.Num() > 0)
		{
			FAssetTools::Get().SyncBrowserToAssets(ObjectsToSync);
		}
	}
}

void FAssetTypeActions_SoundWave::FillVoiceMenu(FMenuBuilder& MenuBuilder, TArray<TWeakObjectPtr<USoundWave>> Objects)
{
	TArray<const UClass*> AllowedClasses;
	AllowedClasses.Add(UDialogueVoice::StaticClass());

	TSharedRef<SWidget> VoicePicker = PropertyCustomizationHelpers::MakeAssetPickerWithMenu(
		FAssetData(),
		false,
		AllowedClasses,
		PropertyCustomizationHelpers::GetNewAssetFactoriesForClasses(AllowedClasses),
		FOnShouldFilterAsset(),
		FOnAssetSelected::CreateSP(this, &FAssetTypeActions_SoundWave::ExecuteCreateDialogueWave, Objects),
		FSimpleDelegate());

	MenuBuilder.AddWidget(VoicePicker, FText::GetEmpty(), false);
}

TSharedPtr<SWidget> FAssetTypeActions_SoundWave::GetThumbnailOverlay(const FAssetData& AssetData) const
{
	TArray<TWeakObjectPtr<USoundBase>> SoundList;
	SoundList.Add(TWeakObjectPtr<USoundBase>((USoundBase*)AssetData.GetAsset()));

	auto OnGetDisplayBrushLambda = [this, SoundList]() -> const FSlateBrush*
	{
		if (IsSoundPlaying(SoundList))
		{
			return FEditorStyle::GetBrush("MediaAsset.AssetActions.Stop");
		}

		return FEditorStyle::GetBrush("MediaAsset.AssetActions.Play");
	};

	auto IsEnabledLambda = [this, SoundList]() -> bool
	{
		return CanExecutePlayCommand(SoundList);
	};

	auto OnClickedLambda = [this, SoundList]() -> FReply
	{
		if (IsSoundPlaying(SoundList))
		{
			ExecuteStopSound(SoundList);
		}
		else
		{
			ExecutePlaySound(SoundList);
		}
		return FReply::Handled();
	};

	auto OnToolTipTextLambda = [this, SoundList]() -> FText
	{
		if (IsSoundPlaying(SoundList))
		{
			return LOCTEXT("Blueprint_StopSoundToolTip", "Stop selected Sound Wave");
		}

		return LOCTEXT("Blueprint_PlaySoundToolTip", "Play selected Sound Wave");
	};

	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(FMargin(2))
		[
			SNew(SButton)
			.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
			.ToolTipText_Lambda(OnToolTipTextLambda)
			.Cursor(EMouseCursor::Default) // The outer widget can specify a DragHand cursor, so we need to override that here
			.ForegroundColor(FSlateColor::UseForeground())		
			.IsEnabled_Lambda(IsEnabledLambda)
			.OnClicked_Lambda(OnClickedLambda)
			[
				SNew(SBox)
				.MinDesiredWidth(16)
				.MinDesiredHeight(16)
				[
					SNew(SImage)
					.Image_Lambda(OnGetDisplayBrushLambda)
				]
			]
		];
}


#undef LOCTEXT_NAMESPACE
