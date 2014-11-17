// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_VectorFieldStatic : public FAssetTypeActions_VectorField
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_VectorFieldStatic", "Vector Field"); }
	virtual UClass* GetSupportedClass() const override { return UVectorFieldStatic::StaticClass(); }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const override { return true; }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) override;
	virtual bool CanFilter() override { return true; }

private:
	/** Handler for when Edit is selected */
	void ExecuteEdit(TArray<TWeakObjectPtr<UVectorFieldStatic>> Objects);

	/** Handler for when Reimport is selected */
	void ExecuteReimport(TArray<TWeakObjectPtr<UVectorFieldStatic>> Objects);

	/** Handler for when FindInExplorer is selected */
	void ExecuteFindInExplorer(TArray<TWeakObjectPtr<UVectorFieldStatic>> Objects);

	/** Handler for when OpenInExternalEditor is selected */
	void ExecuteOpenInExternalEditor(TArray<TWeakObjectPtr<UVectorFieldStatic>> Objects);

	/** Returns true to allow execution of source file commands */
	bool CanExecuteSourceCommands(TArray<TWeakObjectPtr<UVectorFieldStatic>> Objects) const;
};