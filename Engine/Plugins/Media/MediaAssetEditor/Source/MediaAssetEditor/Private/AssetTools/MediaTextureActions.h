// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


#include "AssetTypeActions_Base.h"


/**
 * Implements an action for media assets.
 */
class FMediaTextureActions
	: public FAssetTypeActions_Base
{
public:

	// FAssetTypeActions_Base overrides

	virtual bool CanFilter( ) override;
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) override;
	virtual uint32 GetCategories( ) override;
	virtual FText GetName( ) const override;
	virtual UClass* GetSupportedClass( ) const override;
	virtual FColor GetTypeColor( ) const override;
	virtual bool HasActions( const TArray<UObject*>& InObjects ) const override;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) override;

private:

	// Callback for selecting CreateMaterial.
	void ExecuteCreateMaterial( TArray<TWeakObjectPtr<UTexture>> Objects );

	// Callback for selecting CreateSlateBrush.
	void ExecuteCreateSlateBrush( TArray<TWeakObjectPtr<UTexture>> Objects );

	// Callback for selecting Edit.
	void ExecuteEdit( TArray<TWeakObjectPtr<UTexture>> Objects );

	// Callback for selecting FindMaterials.
	void ExecuteFindMaterials( TWeakObjectPtr<UTexture> Object );
};
