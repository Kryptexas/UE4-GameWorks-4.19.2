// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Small content browser designed to allow for asset picking
 */
class SAssetPicker : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SAssetPicker ){}

		/** A struct containing details about how the asset picker should behave */
		SLATE_ARGUMENT(FAssetPickerConfig, AssetPickerConfig)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs );

	// SWidget implementation
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;
	// End of SWidget implementation

private:
	/** Called by the editable text control when the search text is changed by the user */
	void OnSearchBoxChanged(const FText& InSearchText);

	/** Called by the editable text control when the user commits a text change */
	void OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo);

	/** Handler for when the "None" button is clicked */
	FReply OnNoneButtonClicked();

	/** Handler for when the user double clicks, presses enter, or presses space on an asset */
	void HandleAssetsActivated(const TArray<FAssetData>& ActivatedAssets, EAssetTypeActivationMethod::Type ActivationMethod);

	/** Clears the current asset selection in the view */
	void ClearSelection();

	/** @return The currently selected asset */
	TArray< FAssetData > GetCurrentSelection();

	/** Adjust the asset data selection, +1 to increment or -1 to decrement */
	void AdjustSelection(const int32 direction);

	/** @return The text to highlight on the assets  */
	FText GetHighlightedText() const;

	/** @return The tooltip for the other developers filter button depending on checked state.  */
	FText GetShowOtherDevelopersToolTip() const;

	/** Toggles the filter for showing other developers assets */
	void HandleShowOtherDevelopersCheckStateChanged( const ESlateCheckBoxState::Type InCheckboxState );

	/** Gets if showing other developers assets */
	ESlateCheckBoxState::Type GetShowOtherDevelopersCheckState() const;

private:
	/** The asset view widget */
	TSharedPtr<SAssetView> AssetViewPtr;

	/** The search box */
	TSharedPtr<SAssetSearchBox> SearchBoxPtr;

	/** Called to when an asset is selected or the none button is pressed */
	FOnAssetSelected OnAssetSelected;

	/** Called to when an asset is clicked */
	FOnAssetClicked OnAssetClicked;

	/** Called to when an asset is double clicked */
	FOnAssetDoubleClicked OnAssetDoubleClicked;

	/** Called when enter is pressed while an asset is selected */
	FOnAssetEnterPressed OnAssetEnterPressed;

	/** Called when any number of assets are activated */
	FOnAssetsActivated OnAssetsActivated;

	/** True if the search box will take keyboard focus next frame */
	bool bPendingFocusNextFrame;

	/** Filters needed for filtering the assets */
	TSharedPtr< AssetFilterCollectionType > FilterCollection;
	TSharedPtr< TTextFilter< AssetFilterType > > TextFilter;
	TSharedPtr< FOtherDevelopersAssetFilter > OtherDevelopersFilter;
};