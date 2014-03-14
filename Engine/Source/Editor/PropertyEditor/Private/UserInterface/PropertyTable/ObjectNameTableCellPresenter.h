// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IPropertyTableCellPresenter.h"
#include "IPropertyTableCell.h"
#include "PropertyTableConstants.h"

class FObjectNameTableCellPresenter : public TSharedFromThis< FObjectNameTableCellPresenter >, public IPropertyTableCellPresenter
{
public:

	FObjectNameTableCellPresenter( const TSharedRef< IPropertyTableCell >& InCell )
		: Cell( InCell )
		, FocusWidget( SNullWidget::NullWidget )
	{

	}

	virtual TSharedRef< class SWidget > ConstructDisplayWidget() OVERRIDE
	{
		return SNew( SBox )
			.VAlign( VAlign_Center)
			.Padding( FMargin( 2, 0, 2, 0 ) )
			[
				ConstructNameWidget( PropertyTableConstants::NormalFontStyle )
			]
			.ToolTip
			(
				SNew( SToolTip )[ ConstructNameWidget( PropertyTableConstants::NormalFontStyle ) ]
			);
	}

	virtual bool RequiresDropDown() OVERRIDE { return false; }

	virtual TSharedRef< class SWidget > ConstructEditModeCellWidget() OVERRIDE 
	{
		TSharedPtr< SEditableTextBox > NewFocusWidget;
		TSharedRef< class SWidget > Result = 
			SNew( SBox )
			.VAlign( VAlign_Center)
			.Padding( FMargin( 2, 0, 2, 0 )  )
			[
				SAssignNew( NewFocusWidget, SEditableTextBox )
				.Text( Cell, &IPropertyTableCell::GetValueAsText )
				.Font( FEditorStyle::GetFontStyle( PropertyTableConstants::NormalFontStyle ) )
				.IsReadOnly( true )
			];

		FocusWidget = NewFocusWidget;

		return Result;
	}

	virtual TSharedRef< class SWidget > ConstructEditModeDropDownWidget() OVERRIDE { return SNullWidget::NullWidget; }

	virtual TSharedRef< class SWidget > WidgetToFocusOnEdit() OVERRIDE { return FocusWidget.Pin().ToSharedRef(); }

	virtual FString GetValueAsString() OVERRIDE { return Cell->GetValueAsString(); }

	virtual FText GetValueAsText() OVERRIDE { return Cell->GetValueAsText(); }

	virtual bool HasReadOnlyEditMode() OVERRIDE { return true; }


private:

	TSharedRef< SWidget > ConstructNameWidget( const FName& TextFontStyle )
	{
		TSharedRef< SHorizontalBox > NameBox = SNew( SHorizontalBox );
		const FString& DisplayNameText = Cell->GetValueAsString();

		TArray< FString > DisplayNamePieces;
		DisplayNameText.ParseIntoArray( /*OUT*/ &DisplayNamePieces, TEXT("->"), true /*Cull Empty*/);

		for (int Index = 0; Index < DisplayNamePieces.Num(); Index++)
		{
			NameBox->AddSlot()
				.AutoWidth()
				[
					SNew( STextBlock )
					.Font( FEditorStyle::GetFontStyle( TextFontStyle ) )
					.Text( DisplayNamePieces[ Index ] )
				];

			if ( Index < DisplayNamePieces.Num() - 1 )
			{
				NameBox->AddSlot()
					.AutoWidth()
					.HAlign( HAlign_Center )
					.VAlign( VAlign_Center )
					[
						SNew( SImage )
						.ColorAndOpacity( FSlateColor::UseForeground() )
						.Image( FEditorStyle::GetBrush( "PropertyTable.HeaderRow.Column.PathDelimiter" ) )
					];
			}
		}

		return NameBox;
	}


private:

	TWeakPtr< class SWidget > FocusWidget;
	TSharedRef< IPropertyTableCell > Cell;
};