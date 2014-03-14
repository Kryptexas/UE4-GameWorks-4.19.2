// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_SoundCue : public FAssetTypeActions_SoundBase
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_SoundCue", "Sound Cue"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(0, 175, 255); }
	virtual UClass* GetSupportedClass() const OVERRIDE;
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;
	virtual bool CanFilter() OVERRIDE { return true; }

private:
	/** Handler for when Edit is selected */
	void ExecuteEdit(TArray<TWeakObjectPtr<USoundCue>> Objects);

	/** Take selected SoundCues and combine, as much as possible, them to using shared attenuation settings */
	void ExecuteConsolidateAttenuation(TArray<TWeakObjectPtr<USoundCue>> Objects);

	/** Returns true if more than one cue is selected to consolidate */
	bool CanExecuteConsolidateCommand(TArray<TWeakObjectPtr<USoundCue>> Objects) const;

};