// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "ISectionLayoutBuilder.h"
#include "Runtime/MovieSceneTracks/Public/Sections/MovieSceneCameraCutSection.h"
#include "Runtime/Engine/Public/Slate/SceneViewport.h"
#include "SInlineEditableTextBlock.h"


#define LOCTEXT_NAMESPACE "FCameraCutSection"


/* FCameraCutSection structors
 *****************************************************************************/

FCameraCutSection::FCameraCutSection(TSharedPtr<ISequencer> InSequencer, TSharedPtr<FTrackEditorThumbnailPool> InThumbnailPool, UMovieSceneSection& InSection) : FThumbnailSection(InSequencer, InThumbnailPool, InSection)
{
}

FCameraCutSection::~FCameraCutSection()
{
}


/* ISequencerSection interface
 *****************************************************************************/

void FCameraCutSection::BuildSectionContextMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();

	if (World == nullptr)
	{
		return;
	}

	// get list of available cameras
	TArray<AActor*> AllCameras;
	TArray<AActor*> SelectedCameras;

	for (FActorIterator ActorIt(World); ActorIt; ++ActorIt)
	{
		AActor* Actor = *ActorIt;

		if ((Actor != GetCameraObject()) && Actor->IsListedInSceneOutliner())
		{
			UCameraComponent* CameraComponent = MovieSceneHelpers::CameraComponentFromActor(Actor);
			if (CameraComponent)
			{
				AllCameras.Add(Actor);
			}
		}
	}

	if (AllCameras.Num() == 0)
	{
		return;
	}

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("ChangeCameraMenuText", "Change Camera"));
	{
		for (auto EachCamera : AllCameras)
		{
			FText ActorLabel = FText::FromString(EachCamera->GetActorLabel());

			MenuBuilder.AddMenuEntry(
				FText::Format(LOCTEXT("SetCameraMenuEntryTextFormat", "{0}"), ActorLabel),
				FText::Format(LOCTEXT("SetCameraMenuEntryTooltipFormat", "Assign {0} to this camera cut"), ActorLabel),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateRaw(this, &FCameraCutSection::HandleSetCameraMenuEntryExecute, EachCamera))
			);
		}
	}
	MenuBuilder.EndSection();
}


FText FCameraCutSection::GetDisplayName() const
{
	return NSLOCTEXT("FCameraCutSection", "", "CameraCuts");
}


/* FThumbnailSection interface
 *****************************************************************************/

AActor* FCameraCutSection::GetCameraObject() const
{
	UMovieSceneCameraCutSection* CameraCutSection = Cast<UMovieSceneCameraCutSection>(Section);

	TArray<UObject*> OutObjects;
	// @todo sequencer: the director track may be able to get cameras from sub-movie scenes
	SequencerPtr.Pin()->GetRuntimeObjects(SequencerPtr.Pin()->GetRootMovieSceneSequenceInstance(),  CameraCutSection->GetCameraGuid(), OutObjects);

	AActor* ReturnCam = nullptr;

	if (OutObjects.Num() > 0)
	{
		ReturnCam = Cast<AActor>(OutObjects[0]);

		if (ReturnCam != nullptr)
		{
			return ReturnCam;
		}
	}

	// Otherwise look in the current movie scene
	OutObjects.Empty();
	SequencerPtr.Pin()->GetRuntimeObjects(SequencerPtr.Pin()->GetFocusedMovieSceneSequenceInstance(), CameraCutSection->GetCameraGuid(), OutObjects);

	if (OutObjects.Num() > 0)
	{
		ReturnCam = Cast<AActor>(OutObjects[0]);

		if (ReturnCam != nullptr)
		{
			return ReturnCam;
		}
	}

	return ReturnCam;
}

float FCameraCutSection::GetSectionHeight() const
{
	return FThumbnailSection::GetSectionHeight() + 10.f;
}

FMargin FCameraCutSection::GetContentPadding() const
{
	return FMargin(6.f, 12.f);
}

int32 FCameraCutSection::OnPaintSection(FSequencerSectionPainter& InPainter) const
{
	static const FSlateBrush* FilmBorder = FEditorStyle::GetBrush("Sequencer.Section.FilmBorder");

	InPainter.LayerId = InPainter.PaintSectionBackground();
	return FThumbnailSection::OnPaintSection(InPainter);
}

FText FCameraCutSection::HandleThumbnailTextBlockText() const
{
	AActor* CameraActor = GetCameraObject();
	if (CameraActor)
	{
		return FText::FromString(CameraActor->GetActorLabel());
	}

	return FText::GetEmpty();
}


/* FCameraCutSection callbacks
 *****************************************************************************/

void FCameraCutSection::HandleSetCameraMenuEntryExecute(AActor* InCamera)
{
	auto Sequencer = SequencerPtr.Pin();

	if (Sequencer.IsValid())
	{
		FGuid ObjectGuid = Sequencer->GetHandleToObject(InCamera, true);

		UMovieSceneCameraCutSection* CameraCutSection = Cast<UMovieSceneCameraCutSection>(Section);

		CameraCutSection->SetFlags(RF_Transactional);

		const FScopedTransaction Transaction(LOCTEXT("SetCameraCut", "Set Camera Cut"));

		CameraCutSection->Modify();
	
		CameraCutSection->SetCameraGuid(ObjectGuid);
	
		Sequencer->NotifyMovieSceneDataChanged();
	}
}


#undef LOCTEXT_NAMESPACE
