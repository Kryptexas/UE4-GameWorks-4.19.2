// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Types/SlateEnums.h" // for ETextCommit, ESelectInfo
#include "Misc/Attribute.h"

class IDetailCategoryBuilder;
class IDetailLayoutBuilder;
class IPropertyHandle;
class IDetailPropertyRow;
class ITableRow;
class STableViewBase;
class IPropertyHandle;

class FMotionControllerDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

private:
	void RefreshXRSourceList();
	void SetSourcePropertyValue(const FName NewSystemName);
	void UpdateSourceSelection(TSharedPtr<FName> NewSelection);

	void CustomizeModelSourceRow(TSharedRef<IPropertyHandle>& Property, IDetailPropertyRow& PropertyRow);
	void OnResetSourceValue(TSharedPtr<IPropertyHandle> PropertyHandle);
	bool IsSourceValueModified(TSharedPtr<IPropertyHandle> PropertyHandle);
	FText OnGetSelectedSourceText() const;
	void OnSourceMenuOpened();
	void OnSourceNameCommited(const FText& NewText, ETextCommit::Type InTextCommit);
	TSharedRef<ITableRow> MakeSourceSelectionWidget(TSharedPtr<FName> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnSourceSelectionChanged(TSharedPtr<FName> NewSelection, ESelectInfo::Type SelectInfo);
	
	void CustomizeCustomMeshRow(IDetailPropertyRow& PropertyRow);
	bool IsCustomMeshPropertyEnabled() const;

	TArray< TWeakObjectPtr<UObject> > SelectedObjects;
	TSharedPtr<IPropertyHandle> XRSourceProperty;
	TArray< TSharedPtr<FName> > XRSourceNames;
	TAttribute<bool> UseCustomMeshAttr;
	TSharedPtr<IPropertyHandle> DisplayModelProperty;
	TSharedPtr<IPropertyHandle> DisplayMaterialsProperty;

	static TMap< FName, TSharedPtr<FName> > CustomSourceNames;
};
