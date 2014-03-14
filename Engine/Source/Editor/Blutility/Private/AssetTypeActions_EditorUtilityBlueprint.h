// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetTypeActions_Base.h"

class FAssetTypeActions_EditorUtilityBlueprint : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions interface
	virtual FText GetName() const OVERRIDE;
	virtual FColor GetTypeColor() const OVERRIDE;
	virtual UClass* GetSupportedClass() const OVERRIDE;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const OVERRIDE;
	virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) OVERRIDE;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) OVERRIDE;
	virtual uint32 GetCategories() OVERRIDE;
	// End of IAssetTypeActions interface

protected:
	typedef TArray< TWeakObjectPtr<class UEditorUtilityBlueprint> > FWeakBlueprintPointerArray;

	void ExecuteEdit(FWeakBlueprintPointerArray Objects);
	void ExecuteEditDefaults(FWeakBlueprintPointerArray Objects);
	void ExecuteNewDerivedBlueprint(TWeakObjectPtr<class UEditorUtilityBlueprint> InObject);
	void ExecuteGlobalBlutility(TWeakObjectPtr<class UEditorUtilityBlueprint> InObject);
};