// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/ContentBrowser/Public/ContentBrowserDelegates.h"

// This widget is a content reference, which can optionally show buttons for interacting with the reference
class KISMET_API SContentReference : public SCompoundWidget
{
public:

	/** Allows for informing user when someone tries to set a new reference*/
	DECLARE_DELEGATE_OneParam(FOnSetReference, UObject*)

	SLATE_BEGIN_ARGS(SContentReference)
		: _Style(TEXT("ContentReference"))
		, _AssetReference((UObject*)NULL)
		, _ShowFindInBrowserButton(true)
		, _ShowToolsButton(false)
		, _AllowSelectingNewAsset(false)
		, _AllowClearingReference(false)
		, _AllowedClass((UClass*)NULL)
		, _WidthOverride(FOptionalSize())
	{}
		// The style of the content reference widget (optional)
		SLATE_ARGUMENT(FName, Style)

		// The asset being displayed by this widget
		SLATE_ATTRIBUTE(UObject*, AssetReference)

		// Should the find in content browser button be shown?
		SLATE_ATTRIBUTE(bool, ShowFindInBrowserButton)

		// Should the tools button be shown?
		SLATE_ATTRIBUTE(bool, ShowToolsButton)

		// Can the user pick a new asset to reference?
		SLATE_ATTRIBUTE(bool, AllowSelectingNewAsset)

		// Can the user clear the existing asset reference?
		SLATE_ATTRIBUTE(bool, AllowClearingReference)

		// If specified, the asset references are restricted to this class
		SLATE_ATTRIBUTE(UClass*, AllowedClass)

		// The event to call when the tools button is clicked
		SLATE_EVENT(FOnClicked, OnClickedTools)

		// If bound, this delegate will be executed for each asset in the asset picker to see if it should be filtered
		SLATE_EVENT(FOnShouldFilterAsset, OnShouldFilterAsset)

		//when user changes the referenced object
		SLATE_EVENT(FOnSetReference, OnSetReference)

		/** When specified, the path box will request this fixed size. */
		SLATE_ATTRIBUTE(FOptionalSize, WidthOverride)

		// The event to call when the user selects a new asset or clears the existing reference
		//@TODO: Support that stuff!
	SLATE_END_ARGS()

public:
	// Construct a SContentReference
	void Construct(const FArguments& InArgs);

protected:
	EVisibility GetUseButtonVisibility() const;
	EVisibility GetPickButtonVisibility() const;
	EVisibility GetFindButtonVisibility() const;
	EVisibility GetClearButtonVisibility() const;
	EVisibility GetToolsButtonVisibility() const;

	FReply OnClickUseButton();
	FReply OnClickFindButton();
	FReply OnClickClearButton();

	TSharedRef<SWidget> MakeAssetPickerMenu();
	void OnAssetSelectedFromPicker(const FAssetData& AssetData);

	FString GetAssetShortName() const;
	FString GetAssetFullName() const;

	const FSlateBrush* GetBorderImage() const;

	static void FindObjectInContentBrowser(UObject* Object);
protected:
	// Attributes
	TAttribute<UObject*> AssetReference;
	TAttribute<bool> ShowFindInBrowserButton;
	TAttribute<bool> ShowToolsButton;
	TAttribute<bool> AllowSelectingNewAsset;
	TAttribute<bool> AllowClearingReference;
	TAttribute<UClass*> AllowedClass;

	// Delegates
	FOnShouldFilterAsset OnShouldFilterAsset;
	FOnSetReference OnSetReference;

	// Widgets
	TSharedPtr<SBorder> AssetReferenceNameBorderWidget;
	TSharedPtr<SComboButton> PickerComboButton;

	// Resources
	const FSlateBrush* BorderImageNormal;
	const FSlateBrush* BorderImageHovered;
};
