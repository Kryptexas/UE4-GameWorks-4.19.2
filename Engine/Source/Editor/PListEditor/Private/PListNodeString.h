// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#ifndef __PListNodeString_h__
#define __PlistNodeString_h__

#include "PListNode.h"

/** A Node representing a string value */
class FPListNodeString : public IPListNode
{
public:
	FPListNodeString(SPListEditorPanel* InEditorWidget)
		: IPListNode(InEditorWidget), bFiltered(false), bArrayMember(false), ArrayIndex(-1), bKeyValid(false), bValueValid(false)
	{}

public:

	/** Validation check */
	virtual bool IsValid() OVERRIDE;

	/** Returns the array of children */
	virtual TArray<TSharedPtr<IPListNode> >& GetChildren() OVERRIDE;

	/** Adds a child to the internal array of the class */
	virtual void AddChild(TSharedPtr<IPListNode> InChild) OVERRIDE;

	/** Gets the type of the node via enum */
	virtual EPLNTypes GetType() OVERRIDE;

	/** Determines whether the node needs to generate widgets for columns, or just use the whole row without columns */
	virtual bool UsesColumns() OVERRIDE;

	/** Generates a widget for a TableView row */
	virtual TSharedRef<ITableRow> GenerateWidget(const TSharedRef<STableViewBase>& OwnerTable) OVERRIDE;

	/** Generate a widget for the specified column name */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName, int32 Depth, ITableRow* RowPtr) OVERRIDE;

	/** Gets an XML representation of the node's internals */
	virtual FString ToXML(const int32 indent = 0, bool bOutputKey = true) OVERRIDE;

	/** Refreshes anything necessary in the node, such as a text used in information displaying */
	virtual void Refresh() OVERRIDE;

	/** Calculate how many key/value pairs exist for this node. */
	virtual int32 GetNumPairs() OVERRIDE;

	/** Called when the filter changes */
	virtual void OnFilterTextChanged(FString NewFilter) OVERRIDE;

	/** When parents are refreshed, they can set the index of their children for proper displaying */
	virtual void SetIndex(int32 NewIndex) OVERRIDE;

public:
	
	/** Gets the brush to use based on filter status */
	const FSlateBrush* GetOverlayBrush() OVERRIDE;
	/** Sets the KeyString of the node, needed for telling children to set their keys */
	virtual void SetKey(FString NewString) OVERRIDE;
	/** Sets the value of the boolean */
	virtual void SetValue(FString NewValue) OVERRIDE;
	/** Sets a flag denoting whether the element is in an array */
	virtual void SetArrayMember(bool bArrayMember) OVERRIDE;

private:

	/** Delegate: When key string changes */
	void OnKeyStringChanged(const FText& NewString);
	/** Delegate: When the value changes */
	void OnValueStringChanged(const FText& NewString);
	/** Delegate: Changes the color of the key string text box based on validity */
	FSlateColor GetKeyBackgroundColor() const;
	/** Delegate: Changes the color of the key string text box based on validity */
	FSlateColor GetKeyForegroundColor() const;
	/** Delegate: Changes the color of the value string text box based on validity */
	FSlateColor GetValueBackgroundColor() const;
	/** Delegate: Changes the color of the value string text box based on validity */
	FSlateColor GetValueForegroundColor() const;

private:

	/** The string for the key */
	FString KeyString;
	/** The editable text box for the key string */
	TSharedPtr<SEditableTextBox> KeyStringTextBox;

	/** The value */
	FString ValueString;
	/** The editable text box for the value string */
	TSharedPtr<SEditableTextBox> ValueStringTextBox;

	/** Index withing parent array. Ignore if not an array member */
	int32 ArrayIndex;
	/** Filtered flag */
	bool bFiltered;
	/** Array flag */
	bool bArrayMember;

	/** Flag for valid key string */
	bool bKeyValid;
	/** Flag for valid value string */
	bool bValueValid;
};

#endif
