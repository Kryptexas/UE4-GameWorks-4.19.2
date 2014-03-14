// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SPropertyMenuAssetPicker : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SPropertyMenuAssetPicker )
		: _InitialObject(NULL)
		, _AllowClear(true)
		, _AllowedClasses(NULL)
	{}
		SLATE_ARGUMENT( UObject*, InitialObject )
		SLATE_ARGUMENT( bool, AllowClear )
		SLATE_ARGUMENT( const TArray<const UClass*>*, AllowedClasses )
		SLATE_EVENT( FOnShouldFilterAsset, OnShouldFilterAsset )
		SLATE_EVENT( FOnAssetSelected, OnSet )
		SLATE_EVENT( FSimpleDelegate, OnClose )
	SLATE_END_ARGS()

	/**
	 * Construct the widget.
	 * @param	InArgs				Arguments for widget construction
	 * @param	InPropertyHandle	The property handle that this widget will operate on.
	 */
	void Construct( const FArguments& InArgs );

private:
	/** 
	 * Edit the object referenced by this widget
	 */
	void OnEdit();

	/** 
	 * Delegate handling ctrl+c
	 */
	void OnCopy();

	/** 
	 * Delegate handling ctrl+v
	 */
	void OnPaste();

	/**
	 * @return True of the current clipboard contents can be pasted
	 */
	bool CanPaste();

	/** 
	 * Clear the referenced object
	 */
	void OnClear();

	/** 
	 * Delegate for handling selection in the asset browser.
	 * @param	AssetData	The chosen asset data
	 */
	void OnAssetSelected( const FAssetData& AssetData );

	/** 
	 * Set the value of the asset referenced by this property editor.
	 * Will set the underlying property handle if there is one.
	 * @param	InObject	The object to set the reference to
	 */
	void SetValue( UObject* InObject );

private:
	UObject* CurrentObject;

	/** Whether the asset can be 'None' in this case */
	bool bAllowClear;

	TArray<const UClass*> AllowedClasses;

	/** Delegate for filtering valid assets */
	FOnShouldFilterAsset OnShouldFilterAsset;

	/** Delegate to call when our object value should be set */
	FOnAssetSelected OnSet;

	/** Delegate for closing the containing menu */
	FSimpleDelegate OnClose;

};