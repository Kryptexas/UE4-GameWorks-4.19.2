// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MediaTrackEditor.h"

#include "ContentBrowserModule.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IContentBrowserSingleton.h"
#include "ISequencer.h"
#include "ISequencerObjectChangeListener.h"
#include "MediaSource.h"
#include "MediaTexture.h"
#include "MovieSceneBinding.h"
#include "MovieSceneMediaSection.h"
#include "MovieSceneMediaTrack.h"
#include "MovieSceneToolsUserSettings.h"
#include "MediaPlane.h"
#include "MediaPlaneComponent.h"
#include "SequencerUtilities.h"
#include "TrackEditorThumbnail/TrackEditorThumbnailPool.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"

#include "MediaThumbnailSection.h"


#define LOCTEXT_NAMESPACE "FMediaTrackEditor"


/* FMediaTrackEditor static functions
 *****************************************************************************/

TArray<FAnimatedPropertyKey, TInlineAllocator<1>> FMediaTrackEditor::GetAnimatedPropertyTypes()
{
	return TArray<FAnimatedPropertyKey, TInlineAllocator<1>>({ FAnimatedPropertyKey::FromObjectType(UMediaTexture::StaticClass()) });
}


/* FMediaTrackEditor structors
 *****************************************************************************/

FMediaTrackEditor::FMediaTrackEditor(TSharedRef<ISequencer> InSequencer)
	: FMovieSceneTrackEditor(InSequencer)
{
	ThumbnailPool = MakeShared<FTrackEditorThumbnailPool>(InSequencer);
}


FMediaTrackEditor::~FMediaTrackEditor()
{
}


/* FMovieSceneTrackEditor interface
 *****************************************************************************/

UMovieSceneTrack* FMediaTrackEditor::AddTrack(UMovieScene* FocusedMovieScene, const FGuid& ObjectHandle, TSubclassOf<class UMovieSceneTrack> TrackClass, FName UniqueTypeName)
{
	UMovieSceneTrack* Track = FocusedMovieScene->AddTrack(TrackClass, ObjectHandle);
	UMovieSceneMediaTrack* MediaTrack = Cast<UMovieSceneMediaTrack>(Track);

	return Track;
}


void FMediaTrackEditor::BuildAddTrackMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddTrack", "Media Track"),
		LOCTEXT("AddTooltip", "Adds a new master media track that can play media sources."),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "Sequencer.Tracks.Media"),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FMediaTrackEditor::HandleAddMediaTrackMenuEntryExecute)
		)
	);
}


TSharedPtr<SWidget> FMediaTrackEditor::BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params)
{
	UMovieSceneMediaTrack* MediaTrack = Cast<UMovieSceneMediaTrack>(Track);

	if (MediaTrack == nullptr)
	{
		return SNullWidget::NullWidget;
	}

	auto CreatePicker = [this, MediaTrack]
	{
		FAssetPickerConfig AssetPickerConfig;
		{
			AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateRaw(this, &FMediaTrackEditor::AddNewSection, MediaTrack);
			AssetPickerConfig.bAllowNullSelection = false;
			AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
			AssetPickerConfig.Filter.bRecursiveClasses = true;
			AssetPickerConfig.Filter.ClassNames.Add(UMediaSource::StaticClass()->GetFName());
		}

		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

		TSharedRef<SBox> Picker = SNew(SBox)
			.WidthOverride(300.0f)
			.HeightOverride(300.f)
			[
				ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
			];

		return Picker;
	};

	return SNew(SHorizontalBox)

	+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			FSequencerUtilities::MakeAddButton(LOCTEXT("AddMediaSection_Text", "Media"), FOnGetContent::CreateLambda(CreatePicker), Params.NodeIsHovered)
		];
}


bool FMediaTrackEditor::HandleAssetAdded(UObject* Asset, const FGuid& TargetObjectGuid)
{
	if ((Asset == nullptr) || !Asset->IsA<UMediaSource>())
	{
		return false;
	}

	auto MediaSource = Cast<UMediaSource>(Asset);

	if (TargetObjectGuid.IsValid())
	{
		TArray<TWeakObjectPtr<>> OutObjects;

		for (TWeakObjectPtr<> Object : GetSequencer()->FindObjectsInCurrentSequence(TargetObjectGuid))
		{
			OutObjects.Add(Object);
		}

		AnimatablePropertyChanged(FOnKeyProperty::CreateRaw(this, &FMediaTrackEditor::AddAttachedMediaSource, MediaSource, OutObjects));
	}
	else
	{
		AnimatablePropertyChanged(FOnKeyProperty::CreateRaw(this, &FMediaTrackEditor::AddMasterMediaSource, MediaSource));
	}

	return true;
}


TSharedRef<ISequencerSection> FMediaTrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
	check(SupportsType(SectionObject.GetOuter()->GetClass()));
	return MakeShared<FMediaThumbnailSection>(*CastChecked<UMovieSceneMediaSection>(&SectionObject), ThumbnailPool, GetSequencer());
}


bool FMediaTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> TrackClass) const
{
	return TrackClass.Get() && TrackClass.Get()->IsChildOf(UMovieSceneMediaTrack::StaticClass());
}


void FMediaTrackEditor::Tick(float DeltaTime)
{
	ThumbnailPool->DrawThumbnails();
}


const FSlateBrush* FMediaTrackEditor::GetIconBrush() const
{
	return FEditorStyle::GetBrush("Sequencer.Tracks.Media");
}


/* FMediaTrackEditor implementation
 *****************************************************************************/

FKeyPropertyResult FMediaTrackEditor::AddAttachedMediaSource(float KeyTime, UMediaSource* MediaSource, TArray<TWeakObjectPtr<UObject>> ObjectsToAttachTo)
{
	FKeyPropertyResult KeyPropertyResult;

	for (int32 ObjectIndex = 0; ObjectIndex < ObjectsToAttachTo.Num(); ++ObjectIndex)
	{
		UObject* Object = ObjectsToAttachTo[ObjectIndex].Get();

		FFindOrCreateHandleResult HandleResult = FindOrCreateHandleToObject(Object);
		FGuid ObjectHandle = HandleResult.Handle;
		KeyPropertyResult.bHandleCreated |= HandleResult.bWasCreated;

		if (ObjectHandle.IsValid())
		{
			FFindOrCreateTrackResult TrackResult = FindOrCreateTrackForObject(ObjectHandle, UMovieSceneMediaTrack::StaticClass());
			UMovieSceneTrack* Track = TrackResult.Track;
			KeyPropertyResult.bTrackCreated |= TrackResult.bWasCreated;

			if (ensure(Track))
			{
				auto MediaTrack = Cast<UMovieSceneMediaTrack>(Track);
				MediaTrack->AddNewMediaSource(*MediaSource, KeyTime);
				MediaTrack->SetDisplayName(LOCTEXT("MediaTrackName", "Media"));
				KeyPropertyResult.bTrackModified = true;
			}
		}
	}

	return KeyPropertyResult;
}


FKeyPropertyResult FMediaTrackEditor::AddMasterMediaSource(float KeyTime, UMediaSource* MediaSource)
{
	FKeyPropertyResult KeyPropertyResult;

	FFindOrCreateMasterTrackResult<UMovieSceneMediaTrack> TrackResult = FindOrCreateMasterTrack<UMovieSceneMediaTrack>();
	UMovieSceneTrack* Track = TrackResult.Track;
	auto MediaTrack = Cast<UMovieSceneMediaTrack>(Track);

	MediaTrack->AddNewMediaSource(*MediaSource, KeyTime);

	if (TrackResult.bWasCreated)
	{
		MediaTrack->SetDisplayName(LOCTEXT("MediaTrackName", "Media"));
	}

	KeyPropertyResult.bTrackModified = true;

	return KeyPropertyResult;
}


void FMediaTrackEditor::AddNewSection(const FAssetData& AssetData, UMovieSceneMediaTrack* Track)
{
	FSlateApplication::Get().DismissAllMenus();

	UObject* SelectedObject = AssetData.GetAsset();

	if (SelectedObject)
	{
		auto MediaSource = CastChecked<UMediaSource>(AssetData.GetAsset());

		if (MediaSource != nullptr)
		{
			const FScopedTransaction Transaction(NSLOCTEXT("Sequencer", "AddMedia_Transaction", "Add Media"));

			auto MediaTrack = Cast<UMovieSceneMediaTrack>(Track);
			MediaTrack->Modify();

			float KeyTime = GetSequencer()->GetLocalTime();
			MediaTrack->AddNewMediaSource(*MediaSource, KeyTime);

			GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
		}
	}
}


/* FMediaTrackEditor callbacks
 *****************************************************************************/

void FMediaTrackEditor::HandleAddMediaTrackMenuEntryExecute()
{
	UMovieScene* FocusedMovieScene = GetFocusedMovieScene();

	if (FocusedMovieScene == nullptr)
	{
		return;
	}

	const FScopedTransaction Transaction(NSLOCTEXT("Sequencer", "AddMediaTrack_Transaction", "Add Media Track"));
	FocusedMovieScene->Modify();
	
	auto NewTrack = FocusedMovieScene->AddMasterTrack<UMovieSceneMediaTrack>();
	ensure(NewTrack);

	NewTrack->SetDisplayName(LOCTEXT("MediaTrackName", "Media"));

	GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
}



#undef LOCTEXT_NAMESPACE
