// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


struct SLATE_API FLayoutSaveRestore
{
	/** Gets the ini section label for the additional layout configs */
	static const FString& GetAdditionalLayoutConfigIni();

	/** Write the LayoutToSave out into a a config file uses GEditorUserSettingsIni
	 *
	 * @param LayoutToSave the layout to save.
	 */
	static void SaveTheLayout( const TSharedRef<FTabManager::FLayout>& LayoutToSave );

	/** Write the layout out into a named config file.
	 *
	 * @param LayoutToSave the layout to save.
	 * @param ConfigFileName file to be save to
	 */
	static void SaveToConfig( const TSharedRef<FTabManager::FLayout>& LayoutToSave, FString ConfigFileName );

	/** Given a named DefaultLayout, return any saved versions of it. Otherwise return the default layout uses GEditorUserSettingsIni
	 *
	 * @param DefaultLayout the layout to be used if the file does not exist.
	 *
	 * @return Loaded layout or the default
	 */
	static TSharedRef<FTabManager::FLayout> LoadUserConfigVersionOf( const TSharedRef<FTabManager::FLayout>& DefaultLayout );

	/** Given a named DefualtLayout, return any saved version of it from the given ini file, otherwise return the default, also default to open tabs based on bool
	 *
	 * @param DefaultLayout the layout to be used if the file does not exist.
	 * @param ConfigFileName file to be used to load an existing layout
	 *
	 * @return Loaded layout or the default
	 */
	static TSharedRef<FTabManager::FLayout> LoadFromConfig( const TSharedRef<FTabManager::FLayout>& DefaultLayout, FString ConfigFileName);

private:
	/**
	 * Make a Json string friendly for writing out to UE .ini config files.
	 * The opposite of GetLayoutStringFromIni.
	 */
	static FString PrepareLayoutStringForIni(const FString& LayoutString);

	/**
	 * Convert from UE .ini Json string to a vanilla Json string.
	 * The opposite of PrepareLayoutStringForIni.
	 */
	static FString GetLayoutStringFromIni(const FString& LayoutString);
};