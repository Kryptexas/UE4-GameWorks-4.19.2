// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneObjectBindingIDPicker.h"
#include "IPropertyUtilities.h"
#include "MovieSceneBindingOwnerInterface.h"
#include "MovieSceneSequence.h"
#include "MovieScene.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Tracks/MovieSceneSubTrack.h"
#include "Sections/MovieSceneSubSection.h"
#include "Sections/MovieSceneCinematicShotSection.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Textures/SlateIcon.h"
#include "SlateIconFinder.h"
#include "EditorStyleSet.h"
#include "SImage.h"
#include "SOverlay.h"

#define LOCTEXT_NAMESPACE "MovieSceneObjectBindingIDPicker"

DECLARE_DELEGATE_OneParam(FOnSelectionChanged, const FMovieSceneObjectBindingID&);

struct FSequenceBindingNode
{
	FSequenceBindingNode(FText InDisplayString, FMovieSceneObjectBindingID InBindingID, FSlateIcon InIcon)
		: BindingID(InBindingID)
		, DisplayString(InDisplayString)
		, Icon(InIcon)
		, bIsSpawnable(false)
	{}

	void AddChild(TSharedRef<FSequenceBindingNode> Child)
	{
		Child->ParentID = BindingID;
		Children.Add(Child);
	}

	FMovieSceneObjectBindingID BindingID, ParentID;

	FText DisplayString;
	FSlateIcon Icon;
	bool bIsSpawnable;

	TArray<TSharedRef<FSequenceBindingNode>> Children;
};

struct FSequenceBindingTree
{
	void Build(UMovieSceneSequence* InSequence)
	{
		Hierarchy.Reset();

		FMovieSceneObjectBindingID ID;

		TSharedRef<FSequenceBindingNode> NewNode = MakeShared<FSequenceBindingNode>(FText(), ID, FSlateIcon());
		Hierarchy.Add(ID, NewNode);

		if (InSequence)
		{
			Build(InSequence, MovieSceneSequenceID::Root);
		}
	}

	TSharedRef<FSequenceBindingNode> GetRootNode() const
	{
		return Hierarchy.FindChecked(FMovieSceneObjectBindingID()).ToSharedRef();
	}

	TSharedPtr<FSequenceBindingNode> FindNode(FMovieSceneObjectBindingID BindingID) const
	{
		return Hierarchy.FindRef(BindingID);
	}

private:

	void Build(UMovieSceneSequence* InSequence, FMovieSceneSequenceID SequenceID)
	{
		check(InSequence);

		UMovieScene* MovieScene = InSequence->GetMovieScene();
		if (!MovieScene)
		{
			return;
		}

		const int32 PossessableCount = MovieScene->GetPossessableCount();
		for (int32 Index = 0; Index < PossessableCount; ++Index)
		{
			const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(Index);
			if (InSequence->CanRebindPossessable(Possessable))
			{
				FMovieSceneObjectBindingID ID(Possessable.GetGuid(), SequenceID);

				FSlateIcon Icon = FSlateIconFinder::FindIconForClass(Possessable.GetPossessedObjectClass());
				TSharedRef<FSequenceBindingNode> NewNode = MakeShared<FSequenceBindingNode>(MovieScene->GetObjectDisplayName(Possessable.GetGuid()), ID, Icon);

				EnsureParent(Possessable.GetParent(), MovieScene, SequenceID)->AddChild(NewNode);
				Hierarchy.Add(ID, NewNode);
			}
		}

		int32 SpawnableCount = MovieScene->GetSpawnableCount();
		for (int32 Index = 0; Index < SpawnableCount; ++Index)
		{
			const FMovieSceneSpawnable& Spawnable = MovieScene->GetSpawnable(Index);

			// Spawnables may have been added in the above loop
			FMovieSceneObjectBindingID ID(Spawnable.GetGuid(), SequenceID);
			if (Hierarchy.FindRef(ID).IsValid())
			{
				continue;
			}

			FSlateIcon Icon = FSlateIconFinder::FindIconForClass(Spawnable.GetObjectTemplate()->GetClass());
			TSharedRef<FSequenceBindingNode> NewNode = MakeShared<FSequenceBindingNode>(MovieScene->GetObjectDisplayName(Spawnable.GetGuid()), ID, Icon);
			NewNode->bIsSpawnable = true;

			EnsureParent(FGuid(), MovieScene, SequenceID)->AddChild(NewNode);
			Hierarchy.Add(ID, NewNode);
		}

		// Iterate all sub sections
		for (const UMovieSceneTrack* MasterTrack : MovieScene->GetMasterTracks())
		{
			const UMovieSceneSubTrack* SubTrack = Cast<const UMovieSceneSubTrack>(MasterTrack);
			if (SubTrack)
			{
				for (UMovieSceneSection* Section : SubTrack->GetAllSections())
				{
					UMovieSceneSubSection* SubSection = Cast<UMovieSceneSubSection>(Section);
					UMovieSceneSequence* SubSequence = SubSection ? SubSection->GetSequence() : nullptr;
					if (SubSequence)
					{
						FMovieSceneSequenceID SubSequenceID = SubSection->GetSequenceID();
						if (SequenceID != MovieSceneSequenceID::Root)
						{
							SubSequenceID = SubSequenceID.AccumulateParentID(SequenceID);
						}

						
						FMovieSceneObjectBindingID ID(FGuid(), SubSequenceID);

						UMovieSceneCinematicShotSection* ShotSection = Cast<UMovieSceneCinematicShotSection>(Section);
						FText DisplayString = ShotSection ? ShotSection->GetShotDisplayName() : FText::FromName(SubSection->GetFName());
						FSlateIcon Icon(FEditorStyle::GetStyleSetName(), ShotSection ? "Sequencer.Tracks.CinematicShot" : "Sequencer.Tracks.Sub");
						
						TSharedRef<FSequenceBindingNode> NewNode = MakeShared<FSequenceBindingNode>(DisplayString, ID, Icon);

						EnsureParent(FGuid(), MovieScene, SequenceID)->AddChild(NewNode);
						Hierarchy.Add(ID, NewNode);

						Build(SubSequence, SubSequenceID);
					}
				}
			}
		}
	}

	TSharedRef<FSequenceBindingNode> EnsureParent(const FGuid& InParentGuid, UMovieScene* InMovieScene, FMovieSceneSequenceID SequenceID)
	{
		FMovieSceneObjectBindingID ParentPtr(InParentGuid, SequenceID);

		// If the node already exists
		TSharedPtr<FSequenceBindingNode> Parent = Hierarchy.FindRef(ParentPtr);
		if (Parent.IsValid())
		{
			return Parent.ToSharedRef();
		}

		// Non-object binding nodes should have already been added externally to EnsureParent
		check(InParentGuid.IsValid());

		// Need to add it
		FGuid AddToGuid;
		if (FMovieScenePossessable* GrandParentPossessable = InMovieScene->FindPossessable(InParentGuid))
		{
			AddToGuid = GrandParentPossessable->GetGuid();
		}

		const FMovieScenePossessable* Possessable = InMovieScene->FindPossessable(InParentGuid);
		const FMovieSceneSpawnable* Spawnable = Possessable ? nullptr : InMovieScene->FindSpawnable(InParentGuid);

		FSlateIcon Icon;
		if (Possessable || Spawnable)
		{
			Icon = FSlateIconFinder::FindIconForClass(Possessable ? Possessable->GetPossessedObjectClass() : Spawnable->GetObjectTemplate()->GetClass());
		}

		TSharedRef<FSequenceBindingNode> NewNode = MakeShared<FSequenceBindingNode>(InMovieScene->GetObjectDisplayName(InParentGuid), ParentPtr, Icon);
		NewNode->bIsSpawnable = Spawnable != nullptr;
		EnsureParent(AddToGuid, InMovieScene, SequenceID)->AddChild(NewNode);

		Hierarchy.Add(ParentPtr, NewNode);

		return NewNode;
	}

private:

	TMap<FMovieSceneObjectBindingID, TSharedPtr<FSequenceBindingNode>> Hierarchy;
};

void FMovieSceneObjectBindingIDPicker::Initialize()
{
	if (!DataTree.IsValid())
	{
		DataTree = MakeShared<FSequenceBindingTree>();
	}

	DataTree->Build(GetSequence());
	UpdateCachedData();
}

void FMovieSceneObjectBindingIDPicker::OnGetMenuContent(FMenuBuilder& MenuBuilder, TSharedPtr<FSequenceBindingNode> Node)
{
	check(Node.IsValid());

	bool bHadAnyEntries = false;

	if (Node->BindingID.GetGuid().IsValid())
	{
		bHadAnyEntries = true;
		MenuBuilder.AddMenuEntry(
			Node->DisplayString,
			FText(),
			Node->Icon,
			FUIAction(
				FExecuteAction::CreateRaw(this, &FMovieSceneObjectBindingIDPicker::SetBindingId, Node->BindingID)
			)
		);
	}

	for (const TSharedPtr<FSequenceBindingNode>& Child : Node->Children)
	{
		check(Child.IsValid())

		bHadAnyEntries = true;

		if (!Child->BindingID.GetGuid().IsValid())
		{
			MenuBuilder.AddSubMenu(
				Child->DisplayString,
				FText(),
				FNewMenuDelegate::CreateRaw(this, &FMovieSceneObjectBindingIDPicker::OnGetMenuContent, Child),
				false,
				Child->Icon
				);
		}
		else
		{
			MenuBuilder.AddMenuEntry(
				Child->DisplayString,
				FText(),
				Child->Icon,
				FUIAction(
					FExecuteAction::CreateRaw(this, &FMovieSceneObjectBindingIDPicker::SetBindingId, Child->BindingID)
				)
			);
		}
	}

	if (!bHadAnyEntries)
	{
		MenuBuilder.AddMenuEntry(LOCTEXT("NoEntries", "No Object Bindings"), FText(), FSlateIcon(), FUIAction());
	}
}

TSharedRef<SWidget> FMovieSceneObjectBindingIDPicker::GetPickerMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	Initialize();
	OnGetMenuContent(MenuBuilder, DataTree->GetRootNode());

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FMovieSceneObjectBindingIDPicker::GetCurrentItemWidget(TSharedRef<STextBlock> TextContent)
{
	TextContent->SetText(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateRaw(this, &FMovieSceneObjectBindingIDPicker::GetCurrentText)));
	
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SOverlay)

			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image_Raw(this, &FMovieSceneObjectBindingIDPicker::GetCurrentIconBrush)
			]

			+ SOverlay::Slot()
			.VAlign(VAlign_Top)
			.HAlign(HAlign_Right)
			[
				SNew(SImage)
				.Visibility_Raw(this, &FMovieSceneObjectBindingIDPicker::GetSpawnableIconOverlayVisibility)
				.Image(FEditorStyle::GetBrush("Sequencer.SpawnableIconOverlay"))
			]
		]

		+ SHorizontalBox::Slot()
		.Padding(4.f,0,0,0)
		.VAlign(VAlign_Center)
		[
			TextContent
		];
}

void FMovieSceneObjectBindingIDPicker::SetBindingId(FMovieSceneObjectBindingID InBindingId)
{
	SetCurrentValue(InBindingId);
	UpdateCachedData();
}

void FMovieSceneObjectBindingIDPicker::UpdateCachedData()
{
	FMovieSceneObjectBindingID CurrentValue = GetCurrentValue();
	TSharedPtr<FSequenceBindingNode> Object = CurrentValue.IsValid() ? DataTree->FindNode(CurrentValue) : nullptr;

	if (!Object.IsValid())
	{
		CurrentIcon = FSlateIcon();
		CurrentText = LOCTEXT("UnresolvedBinding", "Unresolved Binding");
		ToolTipText = LOCTEXT("UnresolvedBinding_ToolTip", "The specified binding could not be located in the sequence");
		bIsCurrentItemSpawnable = false;
	}
	else
	{
		CurrentText = Object->DisplayString;
		CurrentIcon = Object->Icon;
		bIsCurrentItemSpawnable = Object->bIsSpawnable;

		ToolTipText = FText();
		while (Object.IsValid() && Object->BindingID != FMovieSceneObjectBindingID())
		{
			ToolTipText = ToolTipText.IsEmpty() ? Object->DisplayString : FText::Format(LOCTEXT("ToolTipFormat", "{0} -> {1}"), Object->DisplayString, ToolTipText);
			Object = DataTree->FindNode(Object->ParentID);
		}
	}
}

FText FMovieSceneObjectBindingIDPicker::GetToolTipText() const
{
	return ToolTipText;
}

FText FMovieSceneObjectBindingIDPicker::GetCurrentText() const
{
	return CurrentText;
}

FSlateIcon FMovieSceneObjectBindingIDPicker::GetCurrentIcon() const
{
	return CurrentIcon;
}

const FSlateBrush* FMovieSceneObjectBindingIDPicker::GetCurrentIconBrush() const
{
	return CurrentIcon.GetOptionalIcon();
}

EVisibility FMovieSceneObjectBindingIDPicker::GetSpawnableIconOverlayVisibility() const
{
	return bIsCurrentItemSpawnable ? EVisibility::Visible : EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE
