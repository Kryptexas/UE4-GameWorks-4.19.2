// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetTypeActions_Base.h"

class FAssetTypeActions_SoundMod: public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE{ return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_SoundMod", "Sound Mod"); }
	virtual FColor GetTypeColor() const OVERRIDE{ return FColor(255, 175, 0); }
	virtual UClass* GetSupportedClass() const OVERRIDE;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const OVERRIDE{ return true; }
	virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) OVERRIDE;
	virtual void AssetsActivated(const TArray<UObject*>& InObjects, EAssetTypeActivationMethod::Type ActivationType) OVERRIDE;
	virtual uint32 GetCategories() OVERRIDE{ return EAssetTypeCategories::Sounds; }
	virtual bool CanFilter() OVERRIDE{ return false; }

private:
	/** Handler for when PlaySound is selected */
	void ExecutePlaySound(TArray<TWeakObjectPtr<USoundMod>> Objects);

	/** Handler for when StopSound is selected */
	void ExecuteStopSound(TArray<TWeakObjectPtr<USoundMod>> Objects);

	/** Returns true if only one sound is selected to play */
	bool CanExecutePlayCommand(TArray<TWeakObjectPtr<USoundMod>> Objects) const;

	/** Plays the specified sound wave */
	void PlaySound(USoundMod* Sound);

	/** Stops any currently playing sounds */
	void StopSound();
};