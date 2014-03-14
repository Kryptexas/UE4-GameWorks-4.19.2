// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_CSVAssetBase : public FAssetTypeActions_Base
{

protected:
	/** Handler for when Edit is selected */
	void ExecuteEdit(TArray< TWeakObjectPtr<UObject> > Objects);

	/** Handler for when Reimport is selected */
	void ExecuteReimport(TArray< TWeakObjectPtr<UObject> > Objects);

	/** Handler for when FindInExplorer is selected */
	void ExecuteFindInExplorer(TArray<FString> Filenames);

	/** Handler for when OpenInExternalEditor is selected */
	void ExecuteOpenInExternalEditor(TArray<FString> Filenames, TArray<FString> OverrideExtensions);

	/** Determine whether the launch in external editor commands can execute or not */
	bool CanExecuteLaunchExternalSourceCommands(TArray<FString> Filenames, TArray<FString> OverrideExtensions) const;

	/** Determine whether the find in exploder editor command can execute or not */
	bool CanExecuteFindInExplorerSourceCommand(TArray<FString> Filenames) const;

	/** Verify the specified filename exists */
	bool VerifyFileExists(const FString& InFileName) const;
};