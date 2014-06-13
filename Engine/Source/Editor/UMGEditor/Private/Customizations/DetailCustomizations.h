// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FBlueprintWidgetCustomization : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<class IDetailCustomization> MakeInstance(UBlueprint* Blueprint)
	{
		return MakeShareable(new FBlueprintWidgetCustomization(Blueprint));
	}

	FBlueprintWidgetCustomization(UBlueprint* InBlueprint)
		: Blueprint(CastChecked<UWidgetBlueprint>(InBlueprint))
	{
	}
	
	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailLayout ) override;

private:
	struct FunctionInfo
	{
		FText DisplayName;
		FString Tooltip;

		FName FuncName;
		UEdGraph* EdGraph;
	};

	void RefreshBlueprintFunctionCache(const UFunction* DelegateSignature);

	TSharedRef<SWidget> OnGenerateDelegateMenu(TSharedRef<IPropertyHandle> PropertyHandle, UFunction* DelegateSignature);

	FText GetCurrentBindingText(TSharedRef<IPropertyHandle> PropertyHandle) const;

	void HandleRemoveBinding(TSharedRef<IPropertyHandle> PropertyHandle);
	void HandleAddBinding(TSharedRef<IPropertyHandle> PropertyHandle, TSharedPtr<FunctionInfo> SelectedFunction);
	void HandleCreateAndAddBinding(TSharedRef<IPropertyHandle> PropertyHandle, UFunction* DelegateSignature);

private:

	UWidgetBlueprint* Blueprint;

	TArray< TSharedPtr<FunctionInfo> > BlueprintFunctionCache;
};
