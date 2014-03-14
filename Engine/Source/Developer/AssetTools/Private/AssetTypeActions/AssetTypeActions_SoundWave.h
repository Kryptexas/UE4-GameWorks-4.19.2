// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_SoundWave : public FAssetTypeActions_SoundBase
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_SoundWave", "Sound Wave"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(0, 0, 255); }
	virtual UClass* GetSupportedClass() const OVERRIDE;
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;
	virtual bool CanFilter() OVERRIDE { return true; }

private:

	/** Handler for when CompressionPreview is selected */
	void ExecuteCompressionPreview(TArray<TWeakObjectPtr<USoundWave>> Objects);

	/** Handler for when Reimport is selected */
	void ExecuteReimport(TArray<TWeakObjectPtr<USoundWave>> Objects);

	/** Handler for when FindInExplorer is selected */
	void ExecuteFindInExplorer(TArray<TWeakObjectPtr<USoundWave>> Objects);

	/** Handler for when OpenInExternalEditor is selected */
	void ExecuteOpenInExternalEditor(TArray<TWeakObjectPtr<USoundWave>> Objects);

	/** Creates a SoundCue of the same name for the sound, if one does not already exist */
	void ExecuteCreateSoundCue(TArray<TWeakObjectPtr<USoundWave>> Objects);

	/** Returns true to allow execution of source file commands */
	bool CanExecuteSourceCommands(TArray<TWeakObjectPtr<USoundWave>> Objects) const;
};