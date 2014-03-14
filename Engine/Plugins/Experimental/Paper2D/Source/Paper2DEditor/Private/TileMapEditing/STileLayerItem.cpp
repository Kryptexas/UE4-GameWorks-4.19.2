// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "STileLayerItem.h"
#include "SContentReference.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "Paper2D"

//////////////////////////////////////////////////////////////////////////
// SRenamableEntry

class SRenamableEntry : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRenamableEntry) {}
		SLATE_ATTRIBUTE(FText, Text)
		SLATE_EVENT(FOnTextCommitted, OnTextCommitted)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// SWidget interface
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) OVERRIDE;
	virtual bool SupportsKeyboardFocus() const OVERRIDE;
	// End of SWidget interface
protected:
	bool bInEditMode;

	/** Editable text object used when node is being renamed */
	TSharedPtr<SEditableText> EditableTextWidget;

	TAttribute<FText> MyText;

	FOnTextCommitted OnTextCommittedEvent;
protected:
	/* Called to check the visibility of the editable text */
	EVisibility	GetEditModeVisibility() const;

	/* Called to check the visibility of the non-editable text */
	EVisibility	GetNonEditModeVisibility() const;

	FText GetEditableText() const;
	FString GetNonEditableText() const;

	void OnTextChanged(const FText& InNewText);
	void OnTextCommited(const FText& InText, ETextCommit::Type CommitInfo);
};

FText SRenamableEntry::GetEditableText() const
{
	return MyText.Get();
}

FString SRenamableEntry::GetNonEditableText() const
{
	return MyText.Get().ToString();
}

void SRenamableEntry::OnTextChanged(const FText& InNewText)
{
	//@TODO:	OnTextChanged.ExecuteIfBound(InNewText);

	//UpdateErrorInfo();
	//ErrorReporting->SetError(ErrorMsg);
}

void SRenamableEntry::OnTextCommited(const FText& InText, ETextCommit::Type CommitInfo) 
{
	OnTextCommittedEvent.ExecuteIfBound(InText, CommitInfo);

	//UpdateErrorInfo();
	//ErrorReporting->SetError(ErrorMsg);

	// lost focus so exit rename mode
	bInEditMode = false;
}

FReply SRenamableEntry::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bInEditMode)
	{
		bInEditMode = true;

		FWidgetPath TextBoxPath;
		FSlateApplication::Get().GeneratePathToWidgetUnchecked(EditableTextWidget.ToSharedRef(), TextBoxPath);
		FSlateApplication::Get().SetKeyboardFocus(EditableTextWidget, EKeyboardFocusCause::Mouse);

		return FReply::Handled();
	}
	else
	{
		return FReply::Unhandled();
	}
}

bool SRenamableEntry::SupportsKeyboardFocus() const
{
	return (bInEditMode && EditableTextWidget.IsValid())
		? StaticCastSharedPtr<SWidget>(EditableTextWidget)->SupportsKeyboardFocus() 
		: false;
}

void SRenamableEntry::Construct(const FArguments& InArgs)
{
	bInEditMode = false;

	MyText = InArgs._Text;
	OnTextCommittedEvent = InArgs._OnTextCommitted;

	//@TODO: Add an error reporting area / text validation

	ChildSlot
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SAssignNew(EditableTextWidget, SEditableText)
			.Text(this, &SRenamableEntry::GetEditableText)
			.OnTextChanged(this, &SRenamableEntry::OnTextChanged)
			.OnTextCommitted(this, &SRenamableEntry::OnTextCommited)
			.SelectAllTextWhenFocused(true)
			.ClearKeyboardFocusOnCommit(true)
			.RevertTextOnEscape(true)
			.Visibility(this, &SRenamableEntry::GetEditModeVisibility)
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
			.Visibility(this, &SRenamableEntry::GetNonEditModeVisibility)
			.Text(this, &SRenamableEntry::GetNonEditableText)
		]
	];
}

EVisibility	SRenamableEntry::GetEditModeVisibility() const
{
	return bInEditMode ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility	SRenamableEntry::GetNonEditModeVisibility() const
{
	return bInEditMode ? EVisibility::Collapsed : EVisibility::Visible;
}

//////////////////////////////////////////////////////////////////////////
// STileLayerItem

void STileLayerItem::Construct(const FArguments& InArgs, class UPaperTileLayer* InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	MyLayer = InItem;

	SMultiColumnTableRow<class UPaperTileLayer*>::Construct(FSuperRowType::FArguments(), OwnerTable);
}

TSharedRef<SWidget> STileLayerItem::GenerateWidgetForColumn(const FName& ColumnName) 
{
	if (ColumnName == TEXT("Name"))
	{
		return SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SAssignNew( VisibilityButton, SButton )
				.ContentPadding( 0 )
				.ButtonStyle( FEditorStyle::Get(), "ToolBarButton" ) //@TODO: Bogus?
				.OnClicked( this, &STileLayerItem::OnToggleVisibility )
				.ToolTipText( LOCTEXT("LayerVisibilityButtonToolTip", "Toggle Layer Visibility") )
				.ForegroundColor( FSlateColor::UseForeground() )
				.HAlign( HAlign_Center )
				.VAlign( VAlign_Center )
				.Content()
				[
					SNew(SImage)
					.Image(this, &STileLayerItem::GetVisibilityBrushForLayer)
					.ColorAndOpacity(this, &STileLayerItem::GetForegroundColorForVisibilityButton)
				]
			]
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			[
				SNew(SRenamableEntry)
				.Text(this, &STileLayerItem::GetLayerDisplayName)
				.OnTextCommitted(this, &STileLayerItem::OnLayerNameCommitted)
			];
	}
	else
	{
		return SNew(SContentReference)
			.AssetReference(this, &STileLayerItem::GetLayerTileset)
			.OnSetReference(this, &STileLayerItem::OnChangeLayerTileSet)
			.AllowedClass(UPaperTileSet::StaticClass())
			.AllowSelectingNewAsset(true)
			.AllowClearingReference(false);
	}
}

FText STileLayerItem::GetLayerDisplayName() const
{
	const FText UnnamedText = LOCTEXT("NoLayerName", "(unnamed)");

	return MyLayer->LayerName.IsEmpty() ? UnnamedText : MyLayer->LayerName;
}

void STileLayerItem::OnLayerNameCommitted(const FText& NewText, ETextCommit::Type CommitInfo)
{
	const FScopedTransaction Transaction( LOCTEXT("TileMapRenameLayer", "Rename Layer") );
	MyLayer->SetFlags(RF_Transactional);
	MyLayer->Modify();
	MyLayer->LayerName = NewText;
}

void STileLayerItem::OnChangeLayerTileSet(UObject* NewAsset)
{
	if (UPaperTileSet* TileSet = Cast<UPaperTileSet>(NewAsset))
	{
		MyLayer->TileSet = TileSet;
	}
}

UObject* STileLayerItem::GetLayerTileset() const
{
	return MyLayer->TileSet;
}

FReply STileLayerItem::OnToggleVisibility()
{
	const FScopedTransaction Transaction( LOCTEXT("ToggleVisibility", "Toggle Layer Visibility") );
	MyLayer->SetFlags(RF_Transactional);
	MyLayer->Modify();
	MyLayer->bHiddenInEditor = !MyLayer->bHiddenInEditor;
	return FReply::Handled();
}

const FSlateBrush* STileLayerItem::GetVisibilityBrushForLayer() const
{
	return MyLayer->bHiddenInEditor ? FEditorStyle::GetBrush("Layer.NotVisibleIcon16x") : FEditorStyle::GetBrush("Layer.VisibleIcon16x");
}

FSlateColor STileLayerItem::GetForegroundColorForVisibilityButton() const
{
	return (VisibilityButton.IsValid() && (VisibilityButton->IsHovered() || VisibilityButton->IsPressed())) ? FEditorStyle::GetSlateColor("InvertedForeground") : FSlateColor::UseForeground();
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE