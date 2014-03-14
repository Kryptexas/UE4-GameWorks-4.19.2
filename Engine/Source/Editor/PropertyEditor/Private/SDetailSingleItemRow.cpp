// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PropertyEditorPrivatePCH.h"
#include "SDetailSingleItemRow.h"
#include "DetailItemNode.h"
#include "PropertyEditorHelpers.h"

namespace DetailWidgetConstants
{
	const FMargin LeftRowPadding( 0.0f, 2.5f, 2.0f, 2.5f );
	const FMargin RightRowPadding( 3.0f, 2.5f, 2.0f, 2.5f );
}

class SConstrainedBox : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SConstrainedBox )
		: _MinWidth(0)
		, _MaxWidth(0)
	{}
		SLATE_DEFAULT_SLOT( FArguments, Content )
		SLATE_ATTRIBUTE( float, MinWidth )
		SLATE_ATTRIBUTE( float, MaxWidth )
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs )
	{
		MinWidth = InArgs._MinWidth;
		MaxWidth = InArgs._MaxWidth;

		ChildSlot
		[
			InArgs._Content.Widget
		];
	}

	virtual FVector2D ComputeDesiredSize() const OVERRIDE
	{
		const float MinWidthVal = MinWidth.Get();
		const float MaxWidthVal = MaxWidth.Get();

		if( MinWidthVal == 0.0f && MaxWidthVal == 0.0f )
		{
			return SCompoundWidget::ComputeDesiredSize();
		}
		else
		{
			FVector2D ChildSize = ChildSlot.Widget->GetDesiredSize();

			float XVal = FMath::Max( MinWidthVal, ChildSize.X );
			if( MaxWidthVal >= MinWidthVal )
			{
				XVal = FMath::Min( MaxWidthVal, XVal );
			}

			return FVector2D( XVal, ChildSize.Y );
		}
	}
private:
	TAttribute<float> MinWidth;
	TAttribute<float> MaxWidth;
};


void SDetailSingleItemRow::Construct( const FArguments& InArgs, FDetailLayoutCustomization* InCustomization, bool bHasMultipleColumns, TSharedRef<IDetailTreeNode> InOwnerTreeNode, const TSharedRef<STableViewBase>& InOwnerTableView )
{
	OwnerTreeNode = InOwnerTreeNode;

	ColumnSizeData = InArgs._ColumnSizeData;

	TSharedRef<SWidget> Widget = SNullWidget::NullWidget;
	Customization = InCustomization;

	EHorizontalAlignment HorizontalAlignment = HAlign_Fill;
	EVerticalAlignment VerticalAlignment = VAlign_Fill;

	if( InCustomization->IsValidCustomization() )
	{
		FDetailWidgetRow Row = InCustomization->GetWidgetRow();
			
		TSharedPtr<SWidget> NameWidget;
		TSharedPtr<SWidget> ValueWidget;
	
		NameWidget = Row.NameWidget.Widget;
		if( Row.IsEnabledAttr.IsBound() )
		{
			NameWidget->SetEnabled( Row.IsEnabledAttr );
		}
		
		ValueWidget =
			SNew( SConstrainedBox )
			.MinWidth( Row.ValueWidget.MinWidth )
			.MaxWidth( Row.ValueWidget.MaxWidth )
			[
				Row.ValueWidget.Widget
			];

		if( Row.IsEnabledAttr.IsBound() )
		{
			ValueWidget->SetEnabled( Row.IsEnabledAttr );
		}

		if( bHasMultipleColumns )
		{
			Widget = 
				SNew( SSplitter )
				.Style( FEditorStyle::Get(), "DetailsView.Splitter" )
				.PhysicalSplitterHandleSize( 1.0f )
				.HitDetectionSplitterHandleSize( 5.0f )
				+ SSplitter::Slot()
				.Value( ColumnSizeData.LeftColumnWidth )
				.OnSlotResized( SSplitter::FOnSlotResized::CreateSP( this, &SDetailSingleItemRow::OnLeftColumnResized ) )
				[
					SNew( SHorizontalBox )
					+ SHorizontalBox::Slot()
					.Padding( 3.0f, 0.0f )
					.HAlign( HAlign_Left )
					.VAlign( VAlign_Center )
					.AutoWidth()
					[
						SNew( SExpanderArrow, SharedThis(this) )
					]
					+ SHorizontalBox::Slot()
					.HAlign( Row.NameWidget.HorizontalAlignment )
					.VAlign( Row.NameWidget.VerticalAlignment )
					.Padding( DetailWidgetConstants::LeftRowPadding )
					[
						NameWidget.ToSharedRef()
					]
				]
				+ SSplitter::Slot()
				.Value( ColumnSizeData.RightColumnWidth )
				.OnSlotResized( ColumnSizeData.OnWidthChanged )
				[
					SNew( SHorizontalBox )
					+ SHorizontalBox::Slot()
					.Padding( DetailWidgetConstants::RightRowPadding )
					.HAlign( Row.ValueWidget.HorizontalAlignment )
					.VAlign( Row.ValueWidget.VerticalAlignment )
					[
						ValueWidget.ToSharedRef()
					]
				];
		}
		else
		{
			Widget =
				SNew( SHorizontalBox )
				+ SHorizontalBox::Slot()
				.Padding( 3.0f, 0.0f )
				.HAlign( HAlign_Left )
				.VAlign( VAlign_Center )
				.AutoWidth()
				[
					SNew( SExpanderArrow, SharedThis(this) )
				]
				+ SHorizontalBox::Slot()
				.HAlign( Row.WholeRowWidget.HorizontalAlignment )
				.VAlign( Row.WholeRowWidget.VerticalAlignment )
				.Padding( DetailWidgetConstants::LeftRowPadding )
				[
					Row.WholeRowWidget.Widget
				];
		}

	}

	this->ChildSlot
	[	
		SNew( SBorder )
		.BorderImage( FEditorStyle::GetBrush("DetailsView.CategoryMiddle") )
		.Padding( 0.0f )
		[
			Widget
		]
	];

	STableRow< TSharedPtr< IDetailTreeNode > >::ConstructInternal(
		STableRow::FArguments()
			.Style(FEditorStyle::Get(), "DetailsView.TreeView.TableRow")
			.ShowSelection(false),
		InOwnerTableView
	);
}

bool SDetailSingleItemRow::OnContextMenuOpening( FMenuBuilder& MenuBuilder )
{
	if( Customization->HasPropertyNode() )
	{
		FUIAction CopyAction( FExecuteAction::CreateSP( this, &SDetailSingleItemRow::OnCopyProperty ) );

		FUIAction PasteAction( FExecuteAction::CreateSP( this, &SDetailSingleItemRow::OnPasteProperty ), FCanExecuteAction::CreateSP( this, &SDetailSingleItemRow::CanPasteProperty ) );

		MenuBuilder.AddMenuSeparator();

		MenuBuilder.AddMenuEntry(	
			NSLOCTEXT("PropertyView", "CopyProperty", "Copy"),
			NSLOCTEXT("PropertyView", "CopyProperty_ToolTip", "Copy this property value"),
			FSlateIcon(),
			CopyAction );

		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("PropertyView", "PasteProperty", "Paste"),
			NSLOCTEXT("PropertyView", "PasteProperty_ToolTip", "Paste the copied value here"),
			FSlateIcon(),
			PasteAction);

		return true;
	}

	return false;
}

void SDetailSingleItemRow::OnLeftColumnResized( float InNewWidth )
{
	// This has to be bound or the splitter will take it upon itself to determine the size
	// We do nothing here because it is handled by the column size data
}

void SDetailSingleItemRow::OnCopyProperty()
{
	if( Customization->HasPropertyNode() && OwnerTreeNode.IsValid() )
	{
		TSharedPtr<IPropertyHandle> Handle = PropertyEditorHelpers::GetPropertyHandle( Customization->GetPropertyNode().ToSharedRef(), OwnerTreeNode.Pin()->GetDetailsView().GetNotifyHook(),  OwnerTreeNode.Pin()->GetDetailsView().GetPropertyUtilities() );

		FString Value;
		if( Handle->GetValueAsFormattedString(Value) == FPropertyAccess::Success )
		{
			FPlatformMisc::ClipboardCopy(*Value);
		}
	}
}

void SDetailSingleItemRow::OnPasteProperty()
{
	FString ClipboardContent;
	FPlatformMisc::ClipboardPaste(ClipboardContent);

	if( !ClipboardContent.IsEmpty() && Customization->HasPropertyNode() && OwnerTreeNode.IsValid() )
	{
		TSharedPtr<IPropertyHandle> Handle = PropertyEditorHelpers::GetPropertyHandle(Customization->GetPropertyNode().ToSharedRef(), OwnerTreeNode.Pin()->GetDetailsView().GetNotifyHook(), OwnerTreeNode.Pin()->GetDetailsView().GetPropertyUtilities());

		Handle->SetValueFromFormattedString( ClipboardContent );
	}

}

bool SDetailSingleItemRow::CanPasteProperty() const
{
	FString ClipboardContent;
	if( Customization->HasPropertyNode() && OwnerTreeNode.IsValid() )
	{
		FPlatformMisc::ClipboardPaste(ClipboardContent);
	}

	return !ClipboardContent.IsEmpty();
}