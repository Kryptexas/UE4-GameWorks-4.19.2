// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"

class SPropertyBinding : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SPropertyBinding)
		: _GeneratePureBindings(true)
		{}
		SLATE_ARGUMENT(bool, GeneratePureBindings)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FWidgetBlueprintEditor> InEditor, UDelegateProperty* DelegateProperty, TSharedRef<IPropertyHandle> Property);

protected:
	struct FunctionInfo
	{
		FText DisplayName;
		FString Tooltip;

		FName FuncName;
		UEdGraph* EdGraph;
	};

	void RefreshBlueprintMemberCache(const UFunction* DelegateSignature, bool bIsPure);

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

	FReply AddOrViewEventBinding(TSharedPtr<FEdGraphSchemaAction> Action);

private:

	TWeakPtr<FWidgetBlueprintEditor> Editor;

	UWidgetBlueprint* Blueprint;

	TArray< TSharedPtr<FunctionInfo> > BlueprintFunctionCache;
	TArray< UProperty* > BlueprintPropertyCache;
};
