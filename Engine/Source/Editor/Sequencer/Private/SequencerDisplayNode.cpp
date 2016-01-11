// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SectionLayoutBuilder.h"
#include "ISequencerSection.h"
#include "Sequencer.h"
#include "MovieScene.h"
#include "SSequencer.h"
#include "SSequencerSectionAreaView.h"
#include "MovieSceneSection.h"
#include "MovieSceneSequence.h"
#include "MovieSceneTrack.h"
#include "MovieSceneTrackEditor.h"
#include "CommonMovieSceneTools.h"
#include "IKeyArea.h"
#include "GroupedKeyArea.h"
#include "ObjectEditorUtils.h"

#define LOCTEXT_NAMESPACE "SequencerDisplayNode"

namespace SequencerNodeConstants
{
	float CommonPadding = 4.f;
}

class SSequencerObjectTrack : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SSequencerObjectTrack) {}
		/** The view range of the section area */
		SLATE_ATTRIBUTE( TRange<float>, ViewRange )
	SLATE_END_ARGS()

	/** SLeafWidget Interface */
	virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;
	
	void Construct( const FArguments& InArgs, TSharedRef<FSequencerDisplayNode> InRootNode )
	{
		RootNode = InRootNode;
		
		ViewRange = InArgs._ViewRange;

		check(RootNode->GetType() == ESequencerNode::Object);
	}

private:
	/** Collects all key times from the root node */
	void CollectAllKeyTimes(TArray<float>& OutKeyTimes) const;

	/** Adds a key time uniquely to an array of key times */
	void AddKeyTime(float NewTime, TArray<float>& OutKeyTimes) const;

private:
	/** Root node of this track view panel */
	TSharedPtr<FSequencerDisplayNode> RootNode;
	/** The current view range */
	TAttribute< TRange<float> > ViewRange;
};

int32 SSequencerObjectTrack::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	TArray<float> OutKeyTimes;
	CollectAllKeyTimes(OutKeyTimes);
	
	FTimeToPixel TimeToPixelConverter(AllottedGeometry, ViewRange.Get());

	for (int32 i = 0; i < OutKeyTimes.Num(); ++i)
	{
		float KeyPosition = TimeToPixelConverter.TimeToPixel(OutKeyTimes[i]);

		static const FVector2D KeyMarkSize = FVector2D(3.f, 21.f);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId+1,
			AllottedGeometry.ToPaintGeometry(FVector2D(KeyPosition - FMath::CeilToFloat(KeyMarkSize.X/2.f), FMath::CeilToFloat(AllottedGeometry.Size.Y/2.f - KeyMarkSize.Y/2.f)), KeyMarkSize),
			FEditorStyle::GetBrush("Sequencer.KeyMark"),
			MyClippingRect,
			ESlateDrawEffect::None,
			FLinearColor(1.f, 1.f, 1.f, 1.f)
		);
	}

	return LayerId+1;
}

FVector2D SSequencerObjectTrack::ComputeDesiredSize( float ) const
{
	// Note: X Size is not used
	return FVector2D( 100.0f, RootNode->GetNodeHeight() );
}

void SSequencerObjectTrack::CollectAllKeyTimes(TArray<float>& OutKeyTimes) const
{
	TArray< TSharedRef<FSectionKeyAreaNode> > OutNodes;
	RootNode->GetChildKeyAreaNodesRecursively(OutNodes);

	for (int32 i = 0; i < OutNodes.Num(); ++i)
	{
		TArray< TSharedRef<IKeyArea> > KeyAreas = OutNodes[i]->GetAllKeyAreas();
		for (int32 j = 0; j < KeyAreas.Num(); ++j)
		{
			TArray<FKeyHandle> KeyHandles = KeyAreas[j]->GetUnsortedKeyHandles();
			for (int32 k = 0; k < KeyHandles.Num(); ++k)
			{
				AddKeyTime(KeyAreas[j]->GetKeyTime(KeyHandles[k]), OutKeyTimes);
			}
		}
	}
}

void SSequencerObjectTrack::AddKeyTime(float NewTime, TArray<float>& OutKeyTimes) const
{
	// @todo Sequencer It might be more efficient to add each key and do the pruning at the end
	for (int32 i = 0; i < OutKeyTimes.Num(); ++i)
	{
		if (FMath::IsNearlyEqual(OutKeyTimes[i], NewTime))
		{
			return;
		}
	}
	OutKeyTimes.Add(NewTime);
}



FSequencerDisplayNode::FSequencerDisplayNode( FName InNodeName, TSharedPtr<FSequencerDisplayNode> InParentNode, FSequencerNodeTree& InParentTree )
	: ParentNode( InParentNode )
	, ParentTree( InParentTree )
	, NodeName( InNodeName )
	, bExpanded( false )
{
	
}

void FSequencerDisplayNode::Initialize(float InVirtualTop, float InVirtualBottom)
{
	bExpanded = ParentTree.GetSavedExpansionState( *this );

	VirtualTop = InVirtualTop;
	VirtualBottom = InVirtualBottom;
}

void FSequencerDisplayNode::AddObjectBindingNode(TSharedRef<FObjectBindingNode> ObjectBindingNode)
{
	ChildNodes.Add(ObjectBindingNode);
}


bool FSequencerDisplayNode::Traverse_ChildFirst(const TFunctionRef<bool(FSequencerDisplayNode&)>& InPredicate, bool bIncludeThisNode)
{
	for (auto& Child : GetChildNodes())
	{
		if (!Child->Traverse_ChildFirst(InPredicate, true))
		{
			return false;
		}
	}

	return bIncludeThisNode ? InPredicate(*this) : true;
}

bool FSequencerDisplayNode::Traverse_ParentFirst(const TFunctionRef<bool(FSequencerDisplayNode&)>& InPredicate, bool bIncludeThisNode)
{
	if (bIncludeThisNode && !InPredicate(*this))
	{
		return false;
	}

	for (auto& Child : GetChildNodes())
	{
		if (!Child->Traverse_ParentFirst(InPredicate, true))
		{
			return false;
		}
	}

	return true;
}

bool FSequencerDisplayNode::TraverseVisible_ChildFirst(const TFunctionRef<bool(FSequencerDisplayNode&)>& InPredicate, bool bIncludeThisNode)
{
	// If the item is not expanded, its children ain't visible
	if (IsExpanded())
	{
		for (auto& Child : GetChildNodes())
		{
			if (!Child->IsHidden() && !Child->TraverseVisible_ChildFirst(InPredicate, true))
			{
				return false;
			}
		}
	}

	if (bIncludeThisNode && !IsHidden())
	{
		return InPredicate(*this);
	}

	// Continue iterating regardless of visibility
	return true;
}


bool FSequencerDisplayNode::TraverseVisible_ParentFirst(const TFunctionRef<bool(FSequencerDisplayNode&)>& InPredicate, bool bIncludeThisNode)
{
	if (bIncludeThisNode && !IsHidden() && !InPredicate(*this))
	{
		return false;
	}

	// If the item is not expanded, its children ain't visible
	if (IsExpanded())
	{
		for (auto& Child : GetChildNodes())
		{
			if (!Child->IsHidden() && !Child->TraverseVisible_ParentFirst(InPredicate, true))
			{
				return false;
			}
		}
	}

	return true;
}

TSharedRef<FSectionCategoryNode> FSequencerDisplayNode::AddCategoryNode( FName CategoryName, const FText& DisplayLabel )
{
	TSharedPtr<FSectionCategoryNode> CategoryNode;

	// See if there is an already existing category node to use
	for( int32 ChildIndex = 0; ChildIndex < ChildNodes.Num(); ++ChildIndex )
	{
		TSharedRef<FSequencerDisplayNode>& ChildNode = ChildNodes[ChildIndex];
		if( ChildNode->GetNodeName() == CategoryName && ChildNode->GetType() == ESequencerNode::Category )
		{
			CategoryNode = StaticCastSharedRef<FSectionCategoryNode>( ChildNode );
		}
	}

	if( !CategoryNode.IsValid() )
	{
		// No existing category found, make a new one
		CategoryNode = MakeShareable( new FSectionCategoryNode( CategoryName, DisplayLabel, SharedThis( this ), ParentTree ) );
		ChildNodes.Add( CategoryNode.ToSharedRef() );
	}

	return CategoryNode.ToSharedRef();
}

TSharedRef<FTrackNode> FSequencerDisplayNode::AddSectionAreaNode( FName SectionName, UMovieSceneTrack& AssociatedTrack, ISequencerTrackEditor& AssociatedEditor )
{
	TSharedPtr<FTrackNode> SectionNode;

	// See if there is an already existing section node to use
	for( int32 ChildIndex = 0; ChildIndex < ChildNodes.Num(); ++ChildIndex )
	{
		TSharedRef<FSequencerDisplayNode>& ChildNode = ChildNodes[ChildIndex];
		if( ChildNode->GetNodeName() == SectionName && ChildNode->GetType() == ESequencerNode::Track )
		{
			SectionNode = StaticCastSharedRef<FTrackNode>( ChildNode );
		}
	}

	if( !SectionNode.IsValid() )
	{
		// No existing node found make a new one
		SectionNode = MakeShareable( new FTrackNode( SectionName, AssociatedTrack, AssociatedEditor, SharedThis( this ), ParentTree ) );
		ChildNodes.Add( SectionNode.ToSharedRef() );
	}

	// The section node type has to match
	check( SectionNode->GetTrack() == &AssociatedTrack );

	return SectionNode.ToSharedRef();
}

void FSequencerDisplayNode::AddKeyAreaNode( FName KeyAreaName, const FText& DisplayName, TSharedRef<IKeyArea> KeyArea )
{
	TSharedPtr<FSectionKeyAreaNode> KeyAreaNode;

	// See if there is an already existing key area node to use
	for( int32 ChildIndex = 0; ChildIndex < ChildNodes.Num(); ++ChildIndex )
	{
		TSharedRef<FSequencerDisplayNode>& ChildNode = ChildNodes[ChildIndex];
		if( ChildNode->GetNodeName() == KeyAreaName && ChildNode->GetType() == ESequencerNode::KeyArea )
		{
			KeyAreaNode = StaticCastSharedRef<FSectionKeyAreaNode>( ChildNode );
		}
	}

	if( !KeyAreaNode.IsValid() )
	{
		// No existing node found make a new one
		KeyAreaNode = MakeShareable( new FSectionKeyAreaNode( KeyAreaName, DisplayName, SharedThis( this ), ParentTree ) );
		ChildNodes.Add( KeyAreaNode.ToSharedRef() );
	}

	KeyAreaNode->AddKeyArea( KeyArea );
}

TSharedRef<SWidget> FSequencerDisplayNode::GenerateContainerWidgetForOutliner(const TSharedRef<SSequencerTreeViewRow>& InRow)
{
	return SNew( SAnimationOutlinerTreeNode, SharedThis( this ), InRow );
}

TSharedRef<SWidget> FSequencerDisplayNode::GenerateEditWidgetForOutliner()
{
	return SNew(SSpacer);
}

TSharedRef<SWidget> FSequencerDisplayNode::GenerateWidgetForSectionArea( const TAttribute< TRange<float> >& ViewRange )
{
	if( GetType() == ESequencerNode::Track )
	{
		return 
			SNew( SSequencerSectionAreaView, SharedThis( this ) )
			.ViewRange( ViewRange );
	}
	else if (GetType() == ESequencerNode::Object)
	{
		return SNew(SSequencerObjectTrack, SharedThis(this))
			.ViewRange( ViewRange );
	}
	else
	{
		// Currently only section areas display widgets
		return SNullWidget::NullWidget;
	}
}

TSharedPtr<FSequencerDisplayNode> FSequencerDisplayNode::GetSectionAreaAuthority() const
{
	TSharedPtr<FSequencerDisplayNode> Authority = SharedThis(const_cast<FSequencerDisplayNode*>(this));

	while (Authority.IsValid())
	{
		if (Authority->GetType() == ESequencerNode::Object || Authority->GetType() == ESequencerNode::Track)
		{
			return Authority;
		}
		else
		{
			Authority = Authority->GetParent();
		}
	}
	return Authority;
}

FString FSequencerDisplayNode::GetPathName() const
{
	// First get our parent's path
	FString PathName;
	if( ParentNode.IsValid() )
	{
		PathName = ParentNode.Pin()->GetPathName() + TEXT(".");
	}

	//then append our path
	PathName += GetNodeName().ToString();

	return PathName;
}

TSharedPtr<SWidget> FSequencerDisplayNode::OnSummonContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// @todo sequencer replace with UI Commands instead of faking it
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, NULL);

	BuildContextMenu(MenuBuilder);

	return MenuBuilder.MakeWidget();
}

void FSequencerDisplayNode::BuildContextMenu(FMenuBuilder& MenuBuilder)
{
	TSharedRef<FSequencerDisplayNode> NodeToBeDeleted = SharedThis(this);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("DeleteNode", "Delete"),
		LOCTEXT("DeleteNodeTooltip", "Delete this or selected nodes"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(&GetSequencer(), &FSequencer::DeleteNode, NodeToBeDeleted)));
}

void FSequencerDisplayNode::GetChildKeyAreaNodesRecursively(TArray< TSharedRef<FSectionKeyAreaNode> >& OutNodes) const
{
	for (int32 i = 0; i < ChildNodes.Num(); ++i)
	{
		if (ChildNodes[i]->GetType() == ESequencerNode::KeyArea)
		{
			OutNodes.Add(StaticCastSharedRef<FSectionKeyAreaNode>(ChildNodes[i]));
		}
		ChildNodes[i]->GetChildKeyAreaNodesRecursively(OutNodes);
	}
}

void FSequencerDisplayNode::SetExpansionState(bool bInExpanded)
{
	bExpanded = bInExpanded;

	// Expansion state has changed, save it to the movie scene now
	ParentTree.SaveExpansionState( *this, bExpanded );
}

bool FSequencerDisplayNode::IsExpanded() const
{
	return ParentTree.HasActiveFilter() ? true : bExpanded;
}

bool FSequencerDisplayNode::IsHidden() const
{
	return ParentTree.HasActiveFilter() && !ParentTree.IsNodeFiltered(AsShared());
}

TSharedRef<FGroupedKeyArea> FSequencerDisplayNode::GetKeyGrouping(int32 InSectionIndex)
{
	if (!KeyGroupings.IsValidIndex(InSectionIndex))
	{
		KeyGroupings.SetNum(InSectionIndex + 1);
	}

	if (!KeyGroupings[InSectionIndex].IsValid())
	{
		KeyGroupings[InSectionIndex] = MakeShareable(new FGroupedKeyArea(*this, InSectionIndex));
	}

	return KeyGroupings[InSectionIndex].ToSharedRef();
}

TSharedRef<FGroupedKeyArea> FSequencerDisplayNode::UpdateKeyGrouping(int32 InSectionIndex)
{
	if (!KeyGroupings.IsValidIndex(InSectionIndex))
	{
		KeyGroupings.SetNum(InSectionIndex + 1);
	}

	if (!KeyGroupings[InSectionIndex].IsValid())
	{
		KeyGroupings[InSectionIndex] = MakeShareable(new FGroupedKeyArea(*this, InSectionIndex));
	}
	else
	{
		*KeyGroupings[InSectionIndex] = FGroupedKeyArea(*this, InSectionIndex);
	}

	return KeyGroupings[InSectionIndex].ToSharedRef();
}


float FSectionKeyAreaNode::GetNodeHeight() const
{
	//@todo Sequencer - Should be defined by the key area probably
	return SequencerLayoutConstants::KeyAreaHeight;
}

FNodePadding FSectionKeyAreaNode::GetNodePadding() const
{
	return FNodePadding(0.f, 1.f);
}

TSharedRef<SWidget> FSectionKeyAreaNode::GenerateEditWidgetForOutliner()
{
	// @todo - Sequencer - Support multiple sections/key areas?
	TArray<TSharedRef<IKeyArea>> AllKeyAreas = GetAllKeyAreas();
	if (AllKeyAreas.Num() > 0)
	{
		if (AllKeyAreas[0]->CanCreateKeyEditor())
		{
			return AllKeyAreas[0]->CreateKeyEditor(&GetSequencer());
		}
	}
	return FSequencerDisplayNode::GenerateEditWidgetForOutliner();
}

void FSectionKeyAreaNode::AddKeyArea( TSharedRef< IKeyArea> KeyArea )
{
	KeyAreas.Add( KeyArea );
}

FTrackNode::FTrackNode( FName NodeName, UMovieSceneTrack& InAssociatedType, ISequencerTrackEditor& InAssociatedEditor, TSharedPtr<FSequencerDisplayNode> InParentNode, FSequencerNodeTree& InParentTree )
	: FSequencerDisplayNode( NodeName, InParentNode, InParentTree )
	, AssociatedType( &InAssociatedType )
	, AssociatedEditor( InAssociatedEditor )
{
}

float FTrackNode::GetNodeHeight() const
{
	return Sections.Num() > 0 ? Sections[0]->GetSectionHeight() * (GetMaxRowIndex()+1) : SequencerLayoutConstants::SectionAreaDefaultHeight;
}

FNodePadding FTrackNode::GetNodePadding() const
{
	return FNodePadding(SequencerNodeConstants::CommonPadding);
}

FText FTrackNode::GetDisplayName() const
{
	// @todo Sequencer - IS there a better way to get the section interface name for the animation outliner?
	return TopLevelKeyNode.IsValid() && Sections.Num() > 0 ? Sections[0]->GetDisplayName() : FText::FromName(NodeName);
}

void FTrackNode::SetSectionAsKeyArea( TSharedRef<IKeyArea>& KeyArea )
{
	if( !TopLevelKeyNode.IsValid() )
	{
		bool bTopLevel = true;
		TopLevelKeyNode = MakeShareable( new FSectionKeyAreaNode( GetNodeName(), FText::GetEmpty(), NULL, ParentTree, bTopLevel ) );
	}

	TopLevelKeyNode->AddKeyArea( KeyArea );
}

void FTrackNode::GetChildKeyAreaNodesRecursively(TArray< TSharedRef<class FSectionKeyAreaNode> >& OutNodes) const
{
	FSequencerDisplayNode::GetChildKeyAreaNodesRecursively(OutNodes);
	if (TopLevelKeyNode.IsValid())
	{
		OutNodes.Add(TopLevelKeyNode.ToSharedRef());
	}
}

TSharedRef<SWidget> FTrackNode::GenerateEditWidgetForOutliner()
{
	TSharedPtr<FSectionKeyAreaNode> KeyAreaNode = GetTopLevelKeyNode();
	if (KeyAreaNode.IsValid())
	{
		// @todo - Sequencer - Support multiple sections/key areas?
		TArray<TSharedRef<IKeyArea>> KeyAreas = KeyAreaNode->GetAllKeyAreas();
		if (KeyAreas.Num() > 0)
		{
			if (KeyAreas[0]->CanCreateKeyEditor())
			{
				return KeyAreas[0]->CreateKeyEditor(&GetSequencer());
			}
		}
	}
	else
	{
		FGuid ObjectBinding;
		TSharedPtr<FSequencerDisplayNode> ParentNode = GetParent();
		if ( ParentNode.IsValid() && ParentNode->GetType() == ESequencerNode::Object )
		{
			ObjectBinding = StaticCastSharedPtr<FObjectBindingNode>(ParentNode)->GetObjectBinding();
		}

		TSharedPtr<SWidget> EditWidget = AssociatedEditor.BuildOutlinerEditWidget( ObjectBinding, AssociatedType.Get() );
		if ( EditWidget.IsValid() )
		{
			return EditWidget.ToSharedRef();
		}
	}
	return FSequencerDisplayNode::GenerateEditWidgetForOutliner();
}

int32 FTrackNode::GetMaxRowIndex() const
{
	int32 MaxRowIndex = 0;
	for (int32 i = 0; i < Sections.Num(); ++i)
	{
		MaxRowIndex = FMath::Max(MaxRowIndex, Sections[i]->GetSectionObject()->GetRowIndex());
	}
	return MaxRowIndex;
}

void FTrackNode::FixRowIndices()
{
	if (AssociatedType->SupportsMultipleRows())
	{
		// remove any empty track rows by waterfalling down sections to be as compact as possible
		TArray< TArray< TSharedRef<ISequencerSection> > > TrackIndices;
		TrackIndices.AddZeroed(GetMaxRowIndex() + 1);
		for (int32 i = 0; i < Sections.Num(); ++i)
		{
			TrackIndices[Sections[i]->GetSectionObject()->GetRowIndex()].Add(Sections[i]);
		}

		int32 NewIndex = 0;
		for (int32 i = 0; i < TrackIndices.Num(); ++i)
		{
			const TArray< TSharedRef<ISequencerSection> >& SectionsForThisIndex = TrackIndices[i];
			if (SectionsForThisIndex.Num() > 0)
			{
				for (int32 j = 0; j < SectionsForThisIndex.Num(); ++j)
				{
					SectionsForThisIndex[j]->GetSectionObject()->SetRowIndex(NewIndex);
				}
				++NewIndex;
			}
		}
	}
	else
	{
		// non master tracks can only have a single row
		for (int32 i = 0; i < Sections.Num(); ++i)
		{
			Sections[i]->GetSectionObject()->SetRowIndex(0);
		}
	}
}

FText FObjectBindingNode::GetDisplayName() const
{
	FText DisplayName;
	return GetSequencer().GetFocusedMovieSceneSequence()->TryGetObjectDisplayName(ObjectBinding, DisplayName) ?
		DisplayName : DefaultDisplayName;
}

float FObjectBindingNode::GetNodeHeight() const
{
	return SequencerLayoutConstants::ObjectNodeHeight;
}

FNodePadding FObjectBindingNode::GetNodePadding() const
{
	return FNodePadding(SequencerNodeConstants::CommonPadding * 2, 0.f);
}

const UClass* FObjectBindingNode::GetClassForObjectBinding()
{
	FSequencer& ParentSequencer = GetSequencer();

	UMovieScene* MovieScene = GetSequencer().GetFocusedMovieSceneSequence()->GetMovieScene();

	FMovieSceneSpawnable* Spawnable = MovieScene->FindSpawnable(ObjectBinding);
	FMovieScenePossessable* Possessable = MovieScene->FindPossessable(ObjectBinding);
	
	// should exist, but also shouldn't be both a spawnable and a possessable
	check((Spawnable != NULL) ^ (Possessable != NULL));
	check((NULL == Spawnable) || (NULL != Spawnable->GetClass())  );
	const UClass* ObjectClass = Spawnable ? Spawnable->GetClass()->GetSuperClass() : Possessable->GetPossessedObjectClass();

	return ObjectClass;
}

TSharedRef<SWidget> FObjectBindingNode::GenerateEditWidgetForOutliner()
{
	// Create a container edit box
	TSharedPtr<class SHorizontalBox> EditBox;
	SAssignNew(EditBox, SHorizontalBox);	

	// Add the property combo box
	EditBox.Get()->AddSlot()
	[
		SNew(SComboButton)
		.ButtonStyle(FEditorStyle::Get(), "FlatButton.Light")
		.OnGetMenuContent(this, &FObjectBindingNode::OnGetAddTrackMenuContent)
		.ContentPadding(FMargin(2, 0))
		.ButtonContent()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.8"))
				.Text(FText::FromString(FString(TEXT("\xf067"))) /*fa-plus*/)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4, 0, 0, 0)
			[
				SNew(STextBlock)
				.Font(FEditorStyle::GetFontStyle("Sequencer.AnimationOutliner.RegularFont"))
				.Text(LOCTEXT("AddTrackButton", "Track"))
			]
		]
	];

	const UClass* ObjectClass = GetClassForObjectBinding();

	GetSequencer().BuildObjectBindingEditButtons(EditBox, ObjectBinding, ObjectClass);

	return EditBox.ToSharedRef();
}

void FObjectBindingNode::BuildContextMenu(FMenuBuilder& MenuBuilder)
{
	if (GetSequencer().IsLevelEditorSequencer())
	{
		const UClass* ObjectClass = GetClassForObjectBinding();
		
		if (ObjectClass->IsChildOf(AActor::StaticClass()))
		{
			FFormatNamedArguments Args;
			MenuBuilder.AddMenuEntry(
				FText::Format( LOCTEXT("Assign Actor ", "Assign Actor"), Args),
				FText::Format( LOCTEXT("AssignActorTooltip", "Assign the selected actor to this track"), Args ),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(&GetSequencer(), &FSequencer::AssignActor, ObjectBinding, this),
							FCanExecuteAction::CreateSP(&GetSequencer(), &FSequencer::CanAssignActor, ObjectBinding)) );
		}
	}

	FSequencerDisplayNode::BuildContextMenu(MenuBuilder);
}

void GetKeyablePropertyPaths(UClass* Class, UStruct* PropertySource, TArray<UProperty*>& PropertyPath, FSequencer& Sequencer, TArray<TArray<UProperty*>>& KeyablePropertyPaths)
{
	//@todo need to resolve this between UMG and the level editor sequencer
	const bool bRecurseAllProperties = Sequencer.IsLevelEditorSequencer();

	for (TFieldIterator<UProperty> PropertyIterator(PropertySource); PropertyIterator; ++PropertyIterator)
	{
		UProperty* Property = *PropertyIterator;

		if (Property && !Property->HasAnyPropertyFlags(CPF_Deprecated))
		{
			PropertyPath.Add(Property);

			bool bIsPropertyKeyable = Sequencer.CanKeyProperty(FCanKeyPropertyParams(Class, PropertyPath));
			if (bIsPropertyKeyable)
			{
				KeyablePropertyPaths.Add(PropertyPath);
			}

			if (!bIsPropertyKeyable || bRecurseAllProperties)
			{
				UStructProperty* StructProperty = Cast<UStructProperty>(Property);
				if (StructProperty != nullptr)
				{
					GetKeyablePropertyPaths(Class, StructProperty->Struct, PropertyPath, Sequencer, KeyablePropertyPaths);
				}
			}

			PropertyPath.RemoveAt(PropertyPath.Num() - 1);
		}
	}
}

struct PropertyMenuData
{
	FString MenuName;
	TArray<UProperty*> PropertyPath;
};
	
TSharedRef<SWidget> FObjectBindingNode::OnGetAddTrackMenuContent()
{
	FSequencer& Sequencer = GetSequencer();

	//@todo need to resolve this between UMG and the level editor sequencer
	const bool bUseSubMenus = Sequencer.IsLevelEditorSequencer();

	UObject* BoundObject = Sequencer.GetFocusedMovieSceneSequence()->FindObject(ObjectBinding);

	ISequencerModule& SequencerModule = FModuleManager::GetModuleChecked<ISequencerModule>( "Sequencer" );
	TSharedRef<FUICommandList> CommandList(new FUICommandList);
	FMenuBuilder AddTrackMenuBuilder(true, nullptr, SequencerModule.GetMenuExtensibilityManager()->GetAllExtenders(CommandList, TArrayBuilder<UObject*>().Add(BoundObject)));

	const UClass* ObjectClass = GetClassForObjectBinding();
	AddTrackMenuBuilder.BeginSection( NAME_None, LOCTEXT("TracksMenuHeader" , "Tracks"));
	GetSequencer().BuildObjectBindingTrackMenu(AddTrackMenuBuilder, ObjectBinding, ObjectClass);
	AddTrackMenuBuilder.EndSection();

	TArray<TArray<UProperty*>> KeyablePropertyPaths;

	if (BoundObject != nullptr)
	{
		TArray<UProperty*> PropertyPath;
		GetKeyablePropertyPaths(BoundObject->GetClass(), BoundObject->GetClass(), PropertyPath, Sequencer, KeyablePropertyPaths);
	}

	// [Aspect Ratio]
	// [PostProcess Settings] [Bloom1Tint] [X]
	// [PostProcess Settings] [Bloom1Tint] [Y]
	// [PostProcess Settings] [ColorGrading]
	// [Ortho View]

	// Create property menu data based on keyable property paths
	TArray<PropertyMenuData> KeyablePropertyMenuData;
	for (auto KeyablePropertyPath : KeyablePropertyPaths)
	{
		PropertyMenuData KeyableMenuData;
		KeyableMenuData.PropertyPath = KeyablePropertyPath;
		KeyableMenuData.MenuName = KeyablePropertyPath[0]->GetDisplayNameText().ToString();
		KeyablePropertyMenuData.Add(KeyableMenuData);
	}

	// Sort on the menu name
	KeyablePropertyMenuData.Sort([](const PropertyMenuData& A, const PropertyMenuData& B)
	{
		int32 CompareResult = A.MenuName.Compare(B.MenuName);
		return CompareResult < 0;
	});
	

	// Add menu items
	AddTrackMenuBuilder.BeginSection( SequencerMenuExtensionPoints::AddTrackMenu_PropertiesSection, LOCTEXT("PropertiesMenuHeader" , "Properties"));
	for (int32 MenuDataIndex = 0; MenuDataIndex < KeyablePropertyMenuData.Num(); )
	{
		TArray<TArray<UProperty*> > KeyableSubMenuPropertyPaths;

		KeyableSubMenuPropertyPaths.Add(KeyablePropertyMenuData[MenuDataIndex].PropertyPath);

		// If this menu data only has one property name, add the menu item
		if (KeyablePropertyMenuData[MenuDataIndex].PropertyPath.Num() == 1 || !bUseSubMenus)
		{
			AddPropertyMenuItems(AddTrackMenuBuilder, KeyableSubMenuPropertyPaths);
			++MenuDataIndex;
		}
		// Otherwise, look to the next menu data to gather up new data
		else
		{
			for (; MenuDataIndex < KeyablePropertyMenuData.Num()-1; )
			{
				if (KeyablePropertyMenuData[MenuDataIndex].MenuName == KeyablePropertyMenuData[MenuDataIndex+1].MenuName)
				{	
					++MenuDataIndex;
					KeyableSubMenuPropertyPaths.Add(KeyablePropertyMenuData[MenuDataIndex].PropertyPath);
				}
				else
				{
					break;
				}
			}

			AddTrackMenuBuilder.AddSubMenu(
				FText::FromString(KeyablePropertyMenuData[MenuDataIndex].MenuName),
				FText::GetEmpty(), 
				FNewMenuDelegate::CreateSP(this, &FObjectBindingNode::AddPropertyMenuItemsWithCategories, KeyableSubMenuPropertyPaths));

			++MenuDataIndex;
		}
	}
	AddTrackMenuBuilder.EndSection();

	return AddTrackMenuBuilder.MakeWidget();
}

void FObjectBindingNode::AddPropertyMenuItemsWithCategories(FMenuBuilder& AddTrackMenuBuilder, TArray<TArray<UProperty*> > KeyablePropertyPaths)
{
	// [PostProcessSettings] [Bloom1Tint] [X]
	// [PostProcessSettings] [Bloom1Tint] [Y]
	// [PostProcessSettings] [ColorGrading]

	// Create property menu data based on keyable property paths
	TSet<UProperty*> PropertiesTraversed;
	TArray<PropertyMenuData> KeyablePropertyMenuData;
	for (auto KeyablePropertyPath : KeyablePropertyPaths)
	{		
		PropertyMenuData KeyableMenuData;
		KeyableMenuData.PropertyPath = KeyablePropertyPath;

		// If the path is greater than 1, keep track of the actual properties (not channels) and only add these properties once since we can't do single channel keying of a property yet.
		if (KeyablePropertyPath.Num() > 1) //@todo
		{
			if (PropertiesTraversed.Find(KeyablePropertyPath[1]) != nullptr)
			{
				continue;
			}

			KeyableMenuData.MenuName = FObjectEditorUtils::GetCategoryFName(KeyablePropertyPath[1]).ToString();
			PropertiesTraversed.Add(KeyablePropertyPath[1]);
		}
		else
		{
			// No sub menus items, so skip
			continue; 
		}
		KeyablePropertyMenuData.Add(KeyableMenuData);
	}

	// Sort on the menu name
	KeyablePropertyMenuData.Sort([](const PropertyMenuData& A, const PropertyMenuData& B)
	{
		int32 CompareResult = A.MenuName.Compare(B.MenuName);
		return CompareResult < 0;
	});

	// Add menu items
	for (int32 MenuDataIndex = 0; MenuDataIndex < KeyablePropertyMenuData.Num(); )
	{
		TArray<TArray<UProperty*> > KeyableSubMenuPropertyPaths;
		KeyableSubMenuPropertyPaths.Add(KeyablePropertyMenuData[MenuDataIndex].PropertyPath);

		for (; MenuDataIndex < KeyablePropertyMenuData.Num()-1; )
		{
			if (KeyablePropertyMenuData[MenuDataIndex].MenuName == KeyablePropertyMenuData[MenuDataIndex+1].MenuName)
			{
				++MenuDataIndex;
				KeyableSubMenuPropertyPaths.Add(KeyablePropertyMenuData[MenuDataIndex].PropertyPath);
			}
			else
			{
				break;
			}
		}

		const int32 PropertyNameIndexStart = 1; // Strip off the struct property name
		const int32 PropertyNameIndexEnd = 2; // Stop at the property name, don't descend into the channels

		AddTrackMenuBuilder.AddSubMenu(
			FText::FromString(KeyablePropertyMenuData[MenuDataIndex].MenuName),
			FText::GetEmpty(), 
			FNewMenuDelegate::CreateSP(this, &FObjectBindingNode::AddPropertyMenuItems, KeyableSubMenuPropertyPaths, PropertyNameIndexStart, PropertyNameIndexEnd));

		++MenuDataIndex;
	}
}

void FObjectBindingNode::AddPropertyMenuItems(FMenuBuilder& AddTrackMenuBuilder, TArray<TArray<UProperty*> > KeyableProperties, int32 PropertyNameIndexStart, int32 PropertyNameIndexEnd)
{
	TArray<PropertyMenuData> KeyablePropertyMenuData;

	for (auto KeyableProperty : KeyableProperties)
	{
		TArray<FString> PropertyNames;
		if (PropertyNameIndexEnd == -1)
		{
			PropertyNameIndexEnd = KeyableProperty.Num();
		}

		//@todo
		if (PropertyNameIndexStart >= KeyableProperty.Num())
		{
			continue;
		}

		for (int32 PropertyNameIndex = PropertyNameIndexStart; PropertyNameIndex < PropertyNameIndexEnd; ++PropertyNameIndex)
		{
			PropertyNames.Add(KeyableProperty[PropertyNameIndex]->GetDisplayNameText().ToString());
		}

		PropertyMenuData KeyableMenuData;
		KeyableMenuData.PropertyPath = KeyableProperty;
		KeyableMenuData.MenuName = FString::Join( PropertyNames, TEXT( "." ) );
		KeyablePropertyMenuData.Add(KeyableMenuData);
	}

	// Sort on the menu name
	KeyablePropertyMenuData.Sort([](const PropertyMenuData& A, const PropertyMenuData& B)
	{
		int32 CompareResult = A.MenuName.Compare(B.MenuName);
		return CompareResult < 0;
	});

	// Add menu items
	for (int32 MenuDataIndex = 0; MenuDataIndex < KeyablePropertyMenuData.Num(); ++MenuDataIndex)
	{
		FUIAction AddTrackMenuAction( FExecuteAction::CreateSP( this, &FObjectBindingNode::AddTrackForProperty, KeyablePropertyMenuData[MenuDataIndex].PropertyPath ) );
		AddTrackMenuBuilder.AddMenuEntry( FText::FromString(KeyablePropertyMenuData[MenuDataIndex].MenuName), FText(), FSlateIcon(), AddTrackMenuAction );
	}
}

void FObjectBindingNode::AddTrackForProperty(TArray<UProperty*> PropertyPath)
{
	FSequencer& Sequencer = GetSequencer();
	UObject* BoundObject = Sequencer.GetFocusedMovieSceneSequence()->FindObject(ObjectBinding);

	TArray<UObject*> KeyableBoundObjects;
	if (BoundObject != nullptr)
	{
		if (Sequencer.CanKeyProperty(FCanKeyPropertyParams(BoundObject->GetClass(), PropertyPath)))
		{
			KeyableBoundObjects.Add(BoundObject);
		}
	}

	FKeyPropertyParams KeyPropertyParams(KeyableBoundObjects, PropertyPath);
	KeyPropertyParams.KeyParams.bCreateTrackIfMissing = true;
	KeyPropertyParams.KeyParams.bCreateHandleIfMissing = false;
	KeyPropertyParams.KeyParams.bAddKeyEvenIfUnchanged = false;
	KeyPropertyParams.KeyParams.KeyInterpolation = Sequencer.GetKeyInterpolation();

	Sequencer.KeyProperty(KeyPropertyParams);
}

float FSectionCategoryNode::GetNodeHeight() const 
{
	return SequencerLayoutConstants::CategoryNodeHeight;
}

FNodePadding FSectionCategoryNode::GetNodePadding() const
{
	return FNodePadding(SequencerNodeConstants::CommonPadding/2);
}

#undef LOCTEXT_NAMESPACE
