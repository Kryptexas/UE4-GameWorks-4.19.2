// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SDeviceManager.h: Declares the SDeviceManager class.
=============================================================================*/

#pragma once


/**
 * Implements the device manager front-end widget.
 */
class SDeviceManager
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SDeviceManager) { }
	SLATE_END_ARGS()

public:

	/**
	 * Default constructor.
	 */
	SDeviceManager( );

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InDeviceServiceManager - The target device manager to use.
	 * @param ConstructUnderMajorTab - The major tab which will contain the session front-end.
	 * @param ConstructUnderWindow - The window in which this widget is being constructed.
	 */
	void Construct( const FArguments& InArgs, const ITargetDeviceServiceManagerRef& InDeviceServiceManager, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow );

protected:

	/**
	 * Fills the Window menu with menu items.
	 *
	 * @param MenuBuilder - The multi-box builder that should be filled with content for this pull-down menu.
	 * @param RootMenuGroup - The root menu group.
	 * @param AppMenuGroup - The application menu group.
	 * @param TabManager - A Tab Manager from which to populate tab spawner menu items.
	 */
	static void FillWindowMenu( FMenuBuilder& MenuBuilder, TSharedRef<FWorkspaceItem> RootMenuGroup, TSharedRef<FWorkspaceItem> AppMenuGroup, const TSharedPtr<FTabManager> TabManager );

private:

	// Callback for spawning tabs.
	TSharedRef<SDockTab> HandleTabManagerSpawnTab( const FSpawnTabArgs& Args, FName TabIdentifier );

private:

	// Holds the target device service manager.
	ITargetDeviceServiceManagerPtr DeviceServiceManager;

	// Holds the device manager's view model.
	FDeviceManagerModelRef Model;

	// Holds the tab manager that manages the front-end's tabs.
	TSharedPtr<FTabManager> TabManager;
};
