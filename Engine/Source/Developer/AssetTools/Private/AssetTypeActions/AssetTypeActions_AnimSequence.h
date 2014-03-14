// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Delegate used when creating Assets from an AnimSequence */
DECLARE_DELEGATE_TwoParams(FOnConfigureFactory, class UFactory*, class UAnimSequence* );

class FAssetTypeActions_AnimSequence : public FAssetTypeActions_AnimationAsset
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AnimSequence", "Animation Sequence"); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UAnimSequence::StaticClass(); }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual bool CanFilter() OVERRIDE { return true; }

private:
	/** Handler for when Reimport is selected */
	void ExecuteReimport(TArray<TWeakObjectPtr<UAnimSequence>> Objects);

	/** Handler for when FindInExplorer is selected */
	void ExecuteFindInExplorer(TArray<TWeakObjectPtr<UAnimSequence>> Objects);

	/** Handler for when OpenInExternalEditor is selected */
	void ExecuteOpenInExternalEditor(TArray<TWeakObjectPtr<UAnimSequence>> Objects);

	/** Returns true to allow execution of source file commands */
	bool CanExecuteSourceCommands(TArray<TWeakObjectPtr<UAnimSequence>> Objects) const;

	/** Handler for when Create AnimComposite is selected */
	void ExecuteNewAnimComposite(TArray<TWeakObjectPtr<UAnimSequence>> Objects) const;

	/** Handler for when Create AnimMontage is selected */
	void ExecuteNewAnimMontage(TArray<TWeakObjectPtr<UAnimSequence>> Objects) const;

	/** Delegate handler passed to CreateAnimationAssets when creating AnimComposites */
	void ConfigureFactoryForAnimComposite(UFactory* AssetFactory, UAnimSequence* SourceAnimation) const;

	/** Delegate handler passed to CreateAnimationAssets when creating AnimMontages */
	void ConfigureFactoryForAnimMontage(UFactory* AssetFactory, UAnimSequence* SourceAnimation) const;

	/** Creates animation assets of the supplied class */
	void CreateAnimationAssets(const TArray<TWeakObjectPtr<UAnimSequence>>& AnimSequences, TSubclassOf<UAnimationAsset> AssetClass, UFactory* AssetFactory, const FString& InSuffix, FOnConfigureFactory OnConfigureFactory) const;
};