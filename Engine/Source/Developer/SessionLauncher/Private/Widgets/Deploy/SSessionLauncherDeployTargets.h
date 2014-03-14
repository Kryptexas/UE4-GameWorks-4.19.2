// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherDeployTargets.h: Declares the SSessionLauncherDeployTargets class.
=============================================================================*/

#pragma once


namespace EShowDevicesChoices
{
	enum Type
	{
		/**
		 * Show all available devices.
		 */
		ShowAllDevices,

		/**
		 * Only show devices that are included in the selected device group.
		 */
		ShowGroupDevices
	};
}


/**
 * Implements the deployment targets panel.
 */
class SSessionLauncherDeployTargets
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherDeployTargets) { }
	SLATE_END_ARGS()


public:

	/**
	 * Destructor.
	 */
	~SSessionLauncherDeployTargets( );


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InModel - The data model.
	 */
	void Construct(	const FArguments& InArgs, const FSessionLauncherModelRef& InModel, bool InIsAlwaysEnabled = false );


public:

	// Begin SCompoundWidget overrides

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;

	// End SCompoundWidget overrides


protected:

	/**
	 * Refreshes the list of device proxies.
	 */
	void RefreshDeviceProxyList( );


private:

	// Callback for selecting a different device group in the device group selector.
	void HandleDeviceGroupSelectorGroupSelected( const ILauncherDeviceGroupPtr& SelectedGroup );

	// Callback for getting the currently selected device group for a device list row.
	ILauncherDeviceGroupPtr HandleDeviceListRowDeviceGroup( ) const;

	// Callback for getting the enabled state of a device proxy list row.
	bool HandleDeviceListRowIsEnabled( ITargetDeviceProxyPtr DeviceProxy ) const;

	// Callback for getting the tool tip text of a device proxy list row.
	FString HandleDeviceListRowToolTipText( ITargetDeviceProxyPtr DeviceProxy ) const;

	// Callback for generating a row in the device list view.
	TSharedRef<ITableRow> HandleDeviceProxyListViewGenerateRow( ITargetDeviceProxyPtr InItem, const TSharedRef<STableViewBase>& OwnerTable ) const;

	// Callback for determining the visibility of the device list view.
	EVisibility HandleDeviceProxyListViewVisibility( ) const;

	// Callback for when a device proxy has been added to the device proxy manager.
	void HandleDeviceProxyManagerProxyAdded( const ITargetDeviceProxyRef& AddedProxy );

	// Callback for getting a flag indicating whether a valid device group is currently selected.
	EVisibility HandleDeviceSelectorVisibility( ) const;

	// Callback for getting the visibility of the 'No devices detected' text block.
	EVisibility HandleNoDevicesBoxVisibility( ) const;

	// Callback for getting the text in the 'no devices' text block.
	FString HandleNoDevicesTextBlockText( ) const;

	// Callback for changing the selected profile in the profile manager.
	void HandleProfileManagerProfileSelected( const ILauncherProfilePtr& SelectedProfile, const ILauncherProfilePtr& PreviousProfile );

	// Callback for determining the visibility of the 'Select a device group' text block.
	EVisibility HandleSelectDeviceGroupWarningVisibility( ) const;

	// Callback for changing the specified 'Show devices' check box.
	void HandleShowCheckBoxCheckStateChanged( ESlateCheckBoxState::Type NewState, EShowDevicesChoices::Type Choice );

	// Callback for determining the checked state of the specified 'Show devices' check box.
	ESlateCheckBoxState::Type HandleShowCheckBoxIsChecked( EShowDevicesChoices::Type Choice ) const;


private:

	// Holds a pointer to the active device group.
	ILauncherDeviceGroupPtr DeployedDeviceGroup;

	// Holds the device group selector.
	TSharedPtr<SSessionLauncherDeviceGroupSelector> DeviceGroupSelector;

	// Holds the list of available device proxies.
	TArray<ITargetDeviceProxyPtr> DeviceProxyList;

	// Holds the device proxy list view .
	TSharedPtr<SListView<ITargetDeviceProxyPtr> > DeviceProxyListView;

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;

	// Holds the current 'Show devices' check box choice.
	EShowDevicesChoices::Type ShowDevicesChoice;

	// Specifies that devices are always enabled
	bool IsAlwaysEnabled;
};
