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

#define LOCTEXT_NAMESPACE "MovieSceneObjectBindingIDPicker"

DECLARE_DELEGATE_OneParam(FOnSelectionChanged, const FMovieSceneObjectBindingID&);

struct FSequenceBindingNode
{
	FSequenceBindingNode(FText InDisplayString, FMovieSceneObjectBindingID InBindingID)
		: BindingID(InBindingID)
		, DisplayString(InDisplayString)
	{}

	void AddChild(TSharedRef<FSequenceBindingNode> Child)
	{
		Child->ParentID = BindingID;
		Children.Add(Child);
	}

	FMovieSceneObjectBindingID BindingID, ParentID;

	FText DisplayString;

	TArray<TSharedRef<FSequenceBindingNode>> Children;
};

struct FSequenceBindingTree
{
	void Build(UMovieSceneSequence* InSequence)
	{
		Hierarchy.Reset();

		FMovieSceneObjectBindingID ID;

		TSharedRef<FSequenceBindingNode> NewNode = MakeShared<FSequenceBindingNode>(FText(), ID);
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

	FText GetTextForBinding(FMovieSceneObjectBindingID BindingID) const
	{
		TSharedPtr<FSequenceBindingNode> Object = BindingID.GetObjectBindingID().IsValid() ? Hierarchy.FindRef(BindingID) : nullptr;
		return Object.IsValid() ? Object->DisplayString : LOCTEXT("UnresolvedBinding", "Unresolved Binding");
	}

	FText GetToolTipTextForBinding(FMovieSceneObjectBindingID BindingID) const
	{
		TSharedPtr<FSequenceBindingNode> Object = BindingID.GetObjectBindingID().IsValid() ? Hierarchy.FindRef(BindingID) : nullptr;
		if (!Object.IsValid())
		{
			return LOCTEXT("UnresolvedBinding_ToolTip", "The specified binding could not be located in the sequence");
		}

		FText NestedPath;
		while (Object.IsValid() && Object->BindingID != FMovieSceneObjectBindingID())
		{
			NestedPath = NestedPath.IsEmpty() ? Object->DisplayString : FText::Format(LOCTEXT("NestedPathFormat", "{0} -> {1}"), Object->DisplayString, NestedPath);
			Object = Hierarchy.FindRef(Object->ParentID);
		}

		return NestedPath;
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

				TSharedRef<FSequenceBindingNode> NewNode = MakeShared<FSequenceBindingNode>(MovieScene->GetObjectDisplayName(Possessable.GetGuid()), ID);

				EnsureParent(Possessable.GetParent(), MovieScene, SequenceID)->AddChild(NewNode);
				Hierarchy.Add(ID, NewNode);
			}
		}

		int32 SpawnableCount = MovieScene->GetSpawnableCount();
		for (int32 Index = 0; Index < SpawnableCount; ++Index)
		{
			const FMovieSceneSpawnable& Spawnable = MovieScene->GetSpawnable(Index);

			FMovieSceneObjectBindingID ID(Spawnable.GetGuid(), SequenceID);

			TSharedRef<FSequenceBindingNode> NewNode = MakeShared<FSequenceBindingNode>(MovieScene->GetObjectDisplayName(Spawnable.GetGuid()), ID);

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
						FText DisplayString = Section->IsA<UMovieSceneCinematicShotSection>() ? Cast<UMovieSceneCinematicShotSection>(Section)->GetShotDisplayName() : FText::FromName(SubSection->GetFName());
						TSharedRef<FSequenceBindingNode> NewNode = MakeShared<FSequenceBindingNode>(DisplayString, ID);

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

		TSharedRef<FSequenceBindingNode> NewNode = MakeShared<FSequenceBindingNode>(InMovieScene->GetObjectDisplayName(InParentGuid), ParentPtr);
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

	CurrentText = DataTree->GetTextForBinding(GetCurrentValue());
	ToolTipText = DataTree->GetToolTipTextForBinding(GetCurrentValue());
}

void FMovieSceneObjectBindingIDPicker::OnGetMenuContent(FMenuBuilder& MenuBuilder, TSharedPtr<FSequenceBindingNode> Node)
{
	check(Node.IsValid());

	if (Node->BindingID.GetObjectBindingID().IsValid())
	{
		MenuBuilder.AddMenuEntry(
			Node->DisplayString,
			FText(),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateRaw(this, &FMovieSceneObjectBindingIDPicker::SetBindingId, Node->BindingID)
			)
		);
	}

	for (const TSharedPtr<FSequenceBindingNode>& Child : Node->Children)
	{
		check(Child.IsValid())

		if (!Child->BindingID.GetObjectBindingID().IsValid())
		{
			MenuBuilder.AddSubMenu(
				Child->DisplayString,
				FText(),
				FNewMenuDelegate::CreateRaw(this, &FMovieSceneObjectBindingIDPicker::OnGetMenuContent, Child)
				);
		}
		else
		{
			MenuBuilder.AddMenuEntry(
				Child->DisplayString,
				FText(),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateRaw(this, &FMovieSceneObjectBindingIDPicker::SetBindingId, Child->BindingID)
				)
			);
		}
	}
}

TSharedRef<SWidget> FMovieSceneObjectBindingIDPicker::GetPickerMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	OnGetMenuContent(MenuBuilder, DataTree->GetRootNode());

	return MenuBuilder.MakeWidget();
}

void FMovieSceneObjectBindingIDPicker::SetBindingId(FMovieSceneObjectBindingID InBindingId)
{
	SetCurrentValue(InBindingId);
	CurrentText = DataTree->GetTextForBinding(InBindingId);
	ToolTipText = DataTree->GetToolTipTextForBinding(InBindingId);
}

FText FMovieSceneObjectBindingIDPicker::GetToolTipText() const
{
	return ToolTipText;
}

FText FMovieSceneObjectBindingIDPicker::GetCurrentText() const
{
	return CurrentText;
}

#undef LOCTEXT_NAMESPACE
