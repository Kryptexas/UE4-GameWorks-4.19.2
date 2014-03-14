// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"

class FEnvQueryTestDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailLayout ) OVERRIDE;

protected:

	TSharedPtr<IPropertyHandle> ConditionHandle;
	TSharedPtr<IPropertyHandle> ModifierHandle;

	void BuildConditionValues();
	void OnConditionComboChange(int32 Index);
	TSharedRef<SWidget> OnGetConditionContent();
	FString GetCurrentConditionDesc() const;
	FString GetWeightModifierInfo() const;
	EVisibility GetFloatFilterVisibility() const;
	EVisibility GetBoolFilterVisibility() const;

	/** cached names of conditions */
	struct FStringIntPair
	{
		FString Str;
		int32 Int;

		FStringIntPair() {}
		FStringIntPair(FString InStr, int32 InInt) : Str(InStr), Int(InInt) {}
	};
	TArray<FStringIntPair> ConditionValues;

	TWeakObjectPtr<UObject> MyTest;
};
