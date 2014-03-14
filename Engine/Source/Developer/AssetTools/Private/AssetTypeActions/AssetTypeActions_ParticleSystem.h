// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_ParticleSystem : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_ParticleSystem", "Particle System"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(255,255,255); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UParticleSystem::StaticClass(); }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const OVERRIDE { return true; }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Basic; }
	
private:
	/** Handler for when Edit is selected */
	void ExecuteEdit(TArray<TWeakObjectPtr<UParticleSystem>> Objects);

	/** Handler for when Copy Parameters is selected */
	void ExecuteCopyParameters(TArray<TWeakObjectPtr<UParticleSystem>> Objects);

	/** Handler for when Convert to Seeded is selected */
	void ConvertToSeeded(TArray<TWeakObjectPtr<UParticleSystem>> Objects);
};