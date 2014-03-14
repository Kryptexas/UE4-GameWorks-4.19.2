// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditorStyle.h"
#include "PropertyEditorModule.h"
#include "AndroidRuntimeSettings.h"

//////////////////////////////////////////////////////////////////////////
// FAndroidTargetSettingsCustomization

class FAndroidTargetSettingsCustomization : public IDetailCustomization
{
public:
	// Makes a new instance of this detail layout class for a specific detail view requesting it
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) OVERRIDE;
	// End of IDetailCustomization interface

private:
	FAndroidTargetSettingsCustomization();

	void BuildAppManifestSection(IDetailLayoutBuilder& DetailLayout);
	void BuildIconSection(IDetailLayoutBuilder& DetailLayout);

	// Navigates to the manifest in the explorer or finder
	FReply OpenManifestFolder();

	// Copies the setup files for the platform into the project
	void CopySetupFilesIntoProject();

	// Called when the orientation is modified
	void OnOrientationModified();
private:
	const FString AndroidRelativePath;

	const FString EngineAndroidPath;
	const FString GameAndroidPath;

	const FString EngineManifestPath;
	const FString GameManifestPath;

	const FString EngineSigningConfigPath;
	const FString GameSigningConfigPath;

	const FString EngineProguardPath;
	const FString GameProguardPath;

	const FString EngineProjectPropertiesPath;
	const FString GameProjectPropertiesPath;

	TArray<struct FPlatformIconInfo> IconNames;

	// Is the manifest writable?
	TAttribute<bool> SetupForPlatformAttribute;

	// Converts an orientation enum to the associated string value
	static FString OrientationToString(const EAndroidScreenOrientation::Type Orientation);

	IDetailLayoutBuilder* SavedLayoutBuilder;
};
