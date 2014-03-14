// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SDeviceDetails.h: Declares the SDeviceDetails class.
=============================================================================*/

#pragma once


/**
 * Implements the device details widget.
 */
class SDeviceDetails
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SDeviceDetails) { }
	SLATE_END_ARGS()

public:

	/**
	 * Destructor.
	 */
	~SDeviceDetails( );

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The construction arguments.
	 * @param InModel - The view model to use.
	 */
	void Construct( const FArguments& InArgs, const FDeviceManagerModelRef& InModel );

private:

	// Callback for getting the enabled state of the details panel.
	bool HandleDetailsIsEnabled() const;

	// Callback for getting the visibility of the details panel.
	EVisibility HandleDetailsVisibility( ) const;

	// Callback for handling device service selection changes.
	void HandleModelSelectedDeviceServiceChanged( );

	// Callback for getting the visibility of the 'Select a device' message.
	EVisibility HandleSelectDeviceOverlayVisibility( ) const;

private:

	// Holds the details view.
//	TSharedPtr<IDetailsView> DetailsView;

	// Holds a pointer the device manager's view model.
	FDeviceManagerModelPtr Model;
};
