// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherPreviewPage.h: Declares the SSessionLauncherPreviewPage class.
=============================================================================*/

#pragma once


/**
 * Implements the launcher's preview page.
 */
class SSessionLauncherPreviewPage
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherPreviewPage) { }
	SLATE_END_ARGS()


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InModel - The data model.
	 */
	void Construct(	const FArguments& InArgs, const FSessionLauncherModelRef& InModel );


public:

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;


protected:

	/**
	 * Refreshes the list of device proxies.
	 */
	void RefreshDeviceProxyList( );


private:

	// Callback for getting the text in the 'Build Configuration' text block.
	FString HandleBuildConfigurationTextBlockText( ) const;

	// Callback for getting the list of platforms to build for.
	FString HandleBuildPlatformsTextBlockText( ) const;

	// Callback for getting the text in the 'Command Line' text block.
	FString HandleCommandLineTextBlockText( ) const;

	// Callback for getting the text in the 'Cooked Cultures' text block.
	FString HandleCookedCulturesTextBlockText( ) const;

	// Callback for getting the text in the 'Cooked Maps' text block.
	FString HandleCookedMapsTextBlockText( ) const;

	// Callback for getting the text in the 'Cooker Options' text block.
	FString HandleCookerOptionsTextBlockText( ) const;

	// Callback for getting the visibility of the specified cook summary box.
	EVisibility HandleCookSummaryBoxVisibility( ELauncherProfileCookModes::Type CookMode ) const;

	// Callback for getting the visibility of the deploy details box.
	EVisibility HandleDeployDetailsBoxVisibility( ) const;

	// Callback for getting the visibility of the specified deploy summary box.
	EVisibility HandleDeploySummaryBoxVisibility( ELauncherProfileDeploymentModes::Type DeploymentMode ) const;

	// Callback for generating a row for the device list.
	TSharedRef<ITableRow> HandleDeviceProxyListViewGenerateRow( ITargetDeviceProxyPtr InItem, const TSharedRef<STableViewBase>& OwnerTable );

	// Callback for getting the text in the 'Initial Culture' text block.
	FString HandleInitialCultureTextBlockText( ) const;

	// Callback for getting the text in the 'Initial Map' text block.
	FString HandleInitialMapTextBlockText( ) const;

	// Callback for getting the visibility of the specified cook summary box.
	EVisibility HandleLaunchSummaryBoxVisibility( ELauncherProfileLaunchModes::Type LaunchMode ) const;

	// Callback for getting the text in the 'VSync' text block.
	FString HandleLaunchVsyncTextBlockText( ) const;

	// Callback for getting the visibility of the specified cook summary box.
	EVisibility HandlePackageSummaryBoxVisibility( ELauncherProfilePackagingModes::Type PackagingMode ) const;

	// Callback for getting the text in the 'Game' text block.
	FString HandleProjectTextBlockText( ) const;

	// Callback for getting the name of the selected device group.
	FString HandleSelectedDeviceGroupTextBlockText( ) const;

	// Callback for getting the name of the selected profile.
	FString HandleSelectedProfileTextBlockText( ) const;

	// Callback for determining the visibility of a validation error icon.
	EVisibility HandleValidationErrorIconVisibility( ELauncherProfileValidationErrors::Type Error ) const;


private:

	// Holds the list of available device proxies.
	TArray<ITargetDeviceProxyPtr> DeviceProxyList;

	// Holds the device proxy list view .
	TSharedPtr<SListView<ITargetDeviceProxyPtr> > DeviceProxyListView;

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;
};
