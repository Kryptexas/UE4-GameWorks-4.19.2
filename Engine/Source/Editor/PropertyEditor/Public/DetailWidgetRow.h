// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "Layout/Visibility.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Framework/Commands/UIAction.h"
#include "Widgets/Layout/SSpacer.h"
#include "PropertyHandle.h"

class FDetailWidgetRow;

/** Widget declaration for custom widgets in a widget row */
class FDetailWidgetDecl
{
public:
	
	FDetailWidgetDecl( class FDetailWidgetRow& InParentDecl, float InMinWidth, float InMaxWidth, EHorizontalAlignment InHAlign, EVerticalAlignment InVAlign )
		: Widget( SNew( SInvalidDetailWidget ) )
		, HorizontalAlignment( InHAlign )
		, VerticalAlignment( InVAlign )
		, MinWidth( InMinWidth )
		, MaxWidth( InMaxWidth )
		, ParentDecl( InParentDecl )
	{

	}

	FDetailWidgetRow& operator[]( TSharedRef<SWidget> InWidget )
	{
		Widget = InWidget;
		return ParentDecl;
	}

	FDetailWidgetDecl& VAlign( EVerticalAlignment InAlignment )
	{
		VerticalAlignment = InAlignment;
		return *this;
	}

	FDetailWidgetDecl& HAlign( EHorizontalAlignment InAlignment )
	{
		HorizontalAlignment = InAlignment;
		return *this;
	}

	FDetailWidgetDecl& MinDesiredWidth( TOptional<float> InMinWidth )
	{
		MinWidth = InMinWidth;
		return *this;
	}

	FDetailWidgetDecl& MaxDesiredWidth( TOptional<float> InMaxWidth )
	{
		MaxWidth = InMaxWidth;
		return *this;
	}
private:
	class SInvalidDetailWidget : public SSpacer
	{
		SLATE_BEGIN_ARGS( SInvalidDetailWidget )
		{}
		SLATE_END_ARGS()

		void Construct( const FArguments& InArgs )
		{
			Visibility = EVisibility::Collapsed;
		}

	};
public:
	TSharedRef<SWidget> Widget;
	EHorizontalAlignment HorizontalAlignment;
	EVerticalAlignment VerticalAlignment;
	TOptional<float> MinWidth;
	TOptional<float> MaxWidth;
private:
	class FDetailWidgetRow& ParentDecl;
};


static FName InvalidDetailWidgetName = TEXT("SInvalidDetailWidget");

/**
 * Represents a single row of custom widgets in a details panel 
 */
class FDetailWidgetRow
{
public:
	PROPERTYEDITOR_API const static float DefaultValueMinWidth;
	PROPERTYEDITOR_API const static float DefaultValueMaxWidth;

	FDetailWidgetRow()
		: NameWidget( *this, 0.0f, 0.0f, HAlign_Fill, VAlign_Center )
		, ValueWidget( *this, DefaultValueMinWidth, DefaultValueMaxWidth, HAlign_Left, VAlign_Fill )
		, WholeRowWidget( *this, 0.0f, 0.0f, HAlign_Fill, VAlign_Fill )
		, VisibilityAttr( EVisibility::Visible )
		, IsEnabledAttr( true )
		, FilterTextString()
		, CopyMenuAction()
		, PasteMenuAction()
		, RowTagName()
		, DiffersFromDefaultAttr( false )
	{
	}
	
	/**
	 * Assigns content to the entire row
	 */
	FDetailWidgetRow& operator[]( TSharedRef<SWidget> InWidget )
	{
		WholeRowWidget.Widget = InWidget;
		return *this;
	}

	/**
	* Assigns content to the whole slot, this is an explicit alternative to using []
	*/
	FDetailWidgetDecl& WholeRowContent()
	{
		return WholeRowWidget;
	}

	/**
	 * Assigns content to just the name slot
	 */
	FDetailWidgetDecl& NameContent()
	{
		return NameWidget;
	}

	/**
	 * Assigns content to the value slot
	 */
	FDetailWidgetDecl& ValueContent()
	{
		return ValueWidget;
	}

	/**
	 * Sets a string which should be used to filter the content when a user searches
	 */
	FDetailWidgetRow& FilterString( const FText& InFilterString )
	{
		FilterTextString = InFilterString;
		return *this;
	}

	/**
	 * Sets the visibility of the entire row
	 */
	FDetailWidgetRow& Visibility( const TAttribute<EVisibility>& InVisibility )
	{
		VisibilityAttr = InVisibility;
		return *this;
	}

	/**
	 * Sets the visibility of the entire row
	 */
	FDetailWidgetRow& IsEnabled( const TAttribute<bool>& InIsEnabled )
	{
		IsEnabledAttr = InIsEnabled;
		return *this;
	}

	/**
	 * Sets a custom copy action to take when copying the data from this row
	 */
	FDetailWidgetRow& CopyAction( const FUIAction& InCopyAction )
	{
		CopyMenuAction = InCopyAction;
		return *this;
	}

	/**
	 * Sets a custom paste action to take when copying the data from this row
	 */
	FDetailWidgetRow& PasteAction( const FUIAction& InPasteAction  )
	{
		PasteMenuAction = InPasteAction;
		return *this;
	}

	/**
	* Add a custom action to the row context menu
	*/
	FDetailWidgetRow& AddCustomContextMenuAction(const FUIAction& Action, const FText& Name, const FText& Tooltip = FText(), const FSlateIcon& SlateIcon = FSlateIcon())
	{
		CustomMenuItems.Add(FCustomMenuData(Action, Name, Tooltip, SlateIcon));
		return *this;
	}

	/**
	 * @return true if the row has columns, false if it spans the entire row
	 */
	bool HasColumns() const
	{
		return NameWidget.Widget->GetType() != InvalidDetailWidgetName || ValueWidget.Widget->GetType() != InvalidDetailWidgetName;
	}

	/**
	 * @return true of the row has any content
	 */
	bool HasAnyContent() const
	{
		return WholeRowWidget.Widget->GetType() != InvalidDetailWidgetName || HasColumns();
	}

	/** @return true if a custom copy/paste is bound on this row */
	bool IsCopyPasteBound() const
	{
		return CopyMenuAction.ExecuteAction.IsBound() && PasteMenuAction.ExecuteAction.IsBound();
	}

	/**
	* Sets a tag which can be used to identify this row 
	*/
	FDetailWidgetRow& RowTag(const FName& InRowTagName)
	{
		RowTagName = InRowTagName;
		return *this;
	}

	/**
	* Sets flag to indicate if property value differs from the default
	*/
	FDetailWidgetRow& DiffersFromDefault(const TAttribute<bool>& InDiffersFromDefaultAttr)
	{
		DiffersFromDefaultAttr = InDiffersFromDefaultAttr;
		return *this;
	}

	/**
	* Used to provide all the property handles this WidgetRow represent
	*/
	FDetailWidgetRow& PropertyHandleList(const TArray<TSharedPtr<IPropertyHandle>>& InPropertyHandles)
	{
		PropertyHandles = InPropertyHandles;
		return *this;
	}

	/**
	* Return all the property handles this WidgetRow represent
	*/
	const TArray<TSharedPtr<IPropertyHandle>>& GetPropertyHandles() const { return PropertyHandles;  }

public:
	/** Name column content */
	FDetailWidgetDecl NameWidget;
	/** Value column content */
	FDetailWidgetDecl ValueWidget;
	/** Whole row content */
	FDetailWidgetDecl WholeRowWidget;
	/** Visibility of the row */
	TAttribute<EVisibility> VisibilityAttr;
	/** IsEnabled of the row */
	TAttribute<bool> IsEnabledAttr;
	/** String to filter with */
	FText FilterTextString;
	/** Action for coping data on this row */
	FUIAction CopyMenuAction;
	/** Action for pasting data on this row */
	FUIAction PasteMenuAction;

	struct FCustomMenuData
	{
		FCustomMenuData(const FUIAction& InAction, const FText& InName, const FText& InTooltip, const FSlateIcon& InSlateIcon)
			: Action(InAction)
			, Name(InName)
			, Tooltip(InTooltip)
			, SlateIcon(InSlateIcon)
		{}

		const FUIAction Action;
		const FText Name;
		const FText Tooltip;
		const FSlateIcon SlateIcon;
	};
	/** Custom Action on this row */
	TArray<FCustomMenuData> CustomMenuItems;

	/* Tag to identify this row */
	FName RowTagName;
	/* Flag to track if property has been modified from default */
	TAttribute<bool> DiffersFromDefaultAttr;
	/* All property handle that this custom widget represent */
	TArray<TSharedPtr<IPropertyHandle>> PropertyHandles;
};

