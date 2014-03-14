// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================
	SDeviceProfileSelectionPanel.h: Declares the SDeviceProfileSelectionPanel class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "DeviceProfileEditorSelectionPanel"


/** Delegate that is executed when the user selects a device profile */
DECLARE_DELEGATE_OneParam( FOnDeviceProfileSelectionChanged, TWeakObjectPtr< UDeviceProfile > );

/** Delegate that is executed when a device profile is pinned */
DECLARE_DELEGATE_OneParam( FOnDeviceProfilePinned, TWeakObjectPtr< UDeviceProfile > );

/** Delegate that is executed when a device profile is unpinned */
DECLARE_DELEGATE_OneParam( FOnDeviceProfileUnpinned, TWeakObjectPtr< UDeviceProfile > );


/**
 * Slate widget to allow users to select device profiles
 */
class SDeviceProfileSelectionPanel : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS( SDeviceProfileSelectionPanel )
		: _OnDeviceProfilePinned()
		, _OnDeviceProfileUnpinned()
		{}
		SLATE_DEFAULT_SLOT( FArguments, Content )

		SLATE_EVENT( FOnDeviceProfileSelectionChanged, OnDeviceProfileSelectionChanged )
		SLATE_EVENT( FOnDeviceProfilePinned, OnDeviceProfilePinned )
		SLATE_EVENT( FOnDeviceProfileUnpinned, OnDeviceProfileUnpinned )
	SLATE_END_ARGS()


	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs, TWeakObjectPtr< UDeviceProfileManager > InDeviceProfileManager );


	/** Destructor */
	~SDeviceProfileSelectionPanel();


	/**
	 * Handle generating the device profile widget.
	 *
	 * @param InDeviceProfile - The selected device profile.
	 * @param OwnerTable - The Owner table.
	 *
	 * @return The generated widget
	 */
	TSharedRef<ITableRow> OnGenerateWidgetForDeviceProfile( TWeakObjectPtr<UDeviceProfile> InDeviceProfile, const TSharedRef< STableViewBase >& OwnerTable );


public:

	/**
	* Handle the profile selection changed
	*
	* @param InDeviceProfile - The selected item
	* @param SelectInfo - Provides context on how the selection changed
	*/
	void HandleProfileSelectionChanged( TWeakObjectPtr<UDeviceProfile> InDeviceProfile, ESelectInfo::Type SelectInfo );


	/**
	* Get the profile which is selected from the list of device profiles
	*
	* @return The profile currently selected in the list, if any
	*/
	TWeakObjectPtr< UDeviceProfile > GetSelectedProfile()
	{
		return SelectedProfile;
	}


protected:

	/**
	* Regenerate the list view widget when the device profiles are recreated
	*/
	void RegenerateProfileList();


private:

	// Holds the device profile manager
	TWeakObjectPtr< UDeviceProfileManager > DeviceProfileManager;

	// The collection of device profile ptrs for the selection process
	TArray< TWeakObjectPtr<UDeviceProfile> > DeviceProfiles;

	// Hold the widget that contains the list view of device profiles
	TSharedPtr< SVerticalBox > ListWidget;

	// Delegate for handling profile selection changed.
	FOnDeviceProfileSelectionChanged OnDeviceProfileSelectionChanged;

	// Delegate for handling a profile being pinned to the grid
	FOnDeviceProfilePinned OnDeviceProfilePinned;

	// Delegate for handling a profile being unpinned from the grid
	FOnDeviceProfileUnpinned OnDeviceProfileUnpinned;

	// The profile selected from the current list
	TWeakObjectPtr< UDeviceProfile > SelectedProfile;
};


#undef LOCTEXT_NAMESPACE