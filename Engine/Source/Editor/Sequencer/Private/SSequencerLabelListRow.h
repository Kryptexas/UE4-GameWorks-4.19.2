// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


struct FSequencerLabelTreeNode
{
public:

	/** Holds the child label nodes. */
	TArray<TSharedPtr<FSequencerLabelTreeNode>> Children;

	/** Holds the display name text. */
	FText DisplayName;

	/** Holds the label. */
	FString Label;

public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InLabel The node's label.
	 */
	FSequencerLabelTreeNode(const FString& InLabel, const FText& InDisplayName)
		: DisplayName(InDisplayName)
		, Label(InLabel)
	{ }
};


#define LOCTEXT_NAMESPACE "SSequencerLabelListRow"

/**
 * Implements a row widget for the label browser tree view.
 */
class SSequencerLabelListRow
	: public STableRow<TSharedPtr<FSequencerLabelTreeNode>>
{
public:

	SLATE_BEGIN_ARGS(SSequencerLabelListRow) { }
		SLATE_ARGUMENT(TSharedPtr<FSequencerLabelTreeNode>, Node)
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The arguments.
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
	{
		Node = InArgs._Node;

		STableRow<TSharedPtr<FSequencerLabelTreeNode>>::Construct(
			STableRow<TSharedPtr<FSequencerLabelTreeNode>>::FArguments()
				.Padding(FMargin(0.0f, 2.0f, 0.0f, 0.0f))
				.Content()
				[
					SNew(SHorizontalBox)

					// folder icon
					+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2.0f, 2.0f)
						.VAlign(VAlign_Center)
						[
							SNew(SImage) 
								.Image(this, &SSequencerLabelListRow::HandleFolderIconImage)
								.ColorAndOpacity(this, &SSequencerLabelListRow::HandleFolderIconColor)
						]

					// folder name
					+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(0.0f, 2.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
								.Text(
									Node->Label.IsEmpty()
										? LOCTEXT("AllTracksLabel", "All Tracks")
										: Node->DisplayName
								)
						]

					// edit icon
					+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SImage)
								.Image(FCoreStyle::Get().GetBrush(TEXT("EditableLabel.EditIcon")))
								.Visibility(this, &SSequencerLabelListRow::HandleEditIconVisibility)
						]
				],
			InOwnerTableView
		);
	}

private:

	EVisibility HandleEditIconVisibility() const
	{
		return IsHovered()
			? EVisibility::Visible
			: EVisibility::Collapsed;
	}

	const FSlateBrush* HandleFolderIconImage() const
	{
		static const FSlateBrush* FolderOpenBrush = FEditorStyle::GetBrush("ContentBrowser.AssetTreeFolderOpen");
		static const FSlateBrush* FolderClosedBrush = FEditorStyle::GetBrush("ContentBrowser.AssetTreeFolderClosed");

		return IsItemExpanded()
			? FolderOpenBrush
			: FolderClosedBrush;
	}

	FSlateColor HandleFolderIconColor() const
	{
		// TODO sequencer: gmp: allow folder color customization
		return FLinearColor::Gray;
	}

private:

	/** Holds the label node. */
	TSharedPtr<FSequencerLabelTreeNode> Node;
};


#undef LOCTEXT_NAMESPACE
