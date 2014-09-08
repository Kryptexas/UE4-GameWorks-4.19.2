// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Provides the customization for all UWidgets.  Bindings, style disabling...etc.
 */
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

	void PerformBindingCustomization(IDetailLayoutBuilder& DetailLayout);

	void RefreshBlueprintMemberCache(const UFunction* DelegateSignature);

	TSharedRef<SWidget> OnGenerateDelegateMenu(UWidget* Widget, TSharedRef<IPropertyHandle> PropertyHandle, UFunction* DelegateSignature, bool bIsPure);

	const FSlateBrush* GetCurrentBindingImage(TSharedRef<IPropertyHandle> PropertyHandle) const;
	FText GetCurrentBindingText(TSharedRef<IPropertyHandle> PropertyHandle) const;

	bool CanRemoveBinding(TSharedRef<IPropertyHandle> PropertyHandle);
	void HandleRemoveBinding(TSharedRef<IPropertyHandle> PropertyHandle);

	void HandleAddFunctionBinding(TSharedRef<IPropertyHandle> PropertyHandle, TSharedPtr<FunctionInfo> SelectedFunction);
	void HandleAddPropertyBinding(TSharedRef<IPropertyHandle> PropertyHandle, UProperty* SelectedProperty);

	void HandleCreateAndAddBinding(UWidget* Widget, TSharedRef<IPropertyHandle> PropertyHandle, UFunction* DelegateSignature, bool bIsPure);
	void GotoFunction(UEdGraph* FunctionGraph);

	EVisibility GetGotoBindingVisibility(TSharedRef<IPropertyHandle> PropertyHandle) const;

	FReply HandleGotoBindingClicked(TSharedRef<IPropertyHandle> PropertyHandle);

	void CreateDelegateCustomization( IDetailLayoutBuilder& DetailLayout, UDelegateProperty* Property, UWidget* Widget );

	void CreateEventCustomization( IDetailLayoutBuilder& DetailLayout, UDelegateProperty* Property, UWidget* Widget );

	void CreateMulticastEventCustomization(IDetailLayoutBuilder& DetailLayout, FName ThisComponentName, UClass* PropertyClass, UMulticastDelegateProperty* Property);

	FReply AddOrViewEventBinding(TSharedPtr<FEdGraphSchemaAction> Action);
private:

	TWeakPtr<FWidgetBlueprintEditor> Editor;

	UWidgetBlueprint* Blueprint;

	TArray< TSharedPtr<FunctionInfo> > BlueprintFunctionCache;
	TArray< UProperty* > BlueprintPropertyCache;
};
