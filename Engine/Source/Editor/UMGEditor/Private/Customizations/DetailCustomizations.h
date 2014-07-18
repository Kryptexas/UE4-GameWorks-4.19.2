// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FBlueprintWidgetCustomization : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<class IDetailCustomization> MakeInstance(TSharedRef<FWidgetBlueprintEditor> InEditor, UBlueprint* InBlueprint)
	{
		return MakeShareable(new FBlueprintWidgetCustomization(InEditor, InBlueprint));
	}

	FBlueprintWidgetCustomization(TSharedRef<FWidgetBlueprintEditor> InEditor, UBlueprint* InBlueprint)
		: Editor(InEditor)
		, Blueprint(CastChecked<UWidgetBlueprint>(InBlueprint))
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

	void RefreshBlueprintMemberCache(const UFunction* DelegateSignature);

	TSharedRef<SWidget> OnGenerateDelegateMenu(TSharedRef<IPropertyHandle> PropertyHandle, UFunction* DelegateSignature);

	const FSlateBrush* GetCurrentBindingImage(TSharedRef<IPropertyHandle> PropertyHandle) const;
	FText GetCurrentBindingText(TSharedRef<IPropertyHandle> PropertyHandle) const;

	bool CanRemoveBinding(TSharedRef<IPropertyHandle> PropertyHandle);
	void HandleRemoveBinding(TSharedRef<IPropertyHandle> PropertyHandle);

	void HandleAddFunctionBinding(TSharedRef<IPropertyHandle> PropertyHandle, TSharedPtr<FunctionInfo> SelectedFunction);
	void HandleAddPropertyBinding(TSharedRef<IPropertyHandle> PropertyHandle, UProperty* SelectedProperty);

	void HandleCreateAndAddBinding(TSharedRef<IPropertyHandle> PropertyHandle, UFunction* DelegateSignature);
	void GotoFunction(UEdGraph* FunctionGraph);

private:

	TWeakPtr<FWidgetBlueprintEditor> Editor;

	UWidgetBlueprint* Blueprint;

	TArray< TSharedPtr<FunctionInfo> > BlueprintFunctionCache;
	TArray< UProperty* > BlueprintPropertyCache;
};
