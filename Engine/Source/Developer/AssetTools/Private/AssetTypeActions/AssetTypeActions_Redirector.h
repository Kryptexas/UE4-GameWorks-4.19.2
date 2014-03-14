// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_Redirector : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_Redirector", "Redirector"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(128, 128, 128); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UObjectRedirector::StaticClass(); }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const OVERRIDE { return true; }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual void AssetsActivated( const TArray<UObject*>& InObjects, EAssetTypeActivationMethod::Type ActivationType ) OVERRIDE;
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Misc; }

private:
	/** Handler for when FindTarget is selected */
	void ExecuteFindTarget(TArray<TWeakObjectPtr<UObjectRedirector>> Objects);

	/** Handler for when FixUp is selected */
	void ExecuteFixUp(TArray<TWeakObjectPtr<UObjectRedirector>> Objects);

	/** Syncs the content browser to the destination objects for all the supplied redirectors */
	void FindTargets(const TArray<UObjectRedirector*>& Redirectors) const;
};