// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "DetailCustomizations.h"

#include "PropertyEditing.h"
#include "ObjectEditorUtils.h"
#include "WidgetGraphSchema.h"
#include "ScopedTransaction.h"
#include "BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "UMG"

void FBlueprintWidgetCustomization::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	TArray< TWeakObjectPtr<UObject> > OutObjects;
	DetailLayout.GetObjectsBeingCustomized(OutObjects);

	if ( OutObjects.Num() == 1 )
	{
		UClass* PropertyClass = OutObjects[0].Get()->GetClass();

		for ( TFieldIterator<UProperty> PropertyIt(PropertyClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt )
		{
			if ( UDelegateProperty* Property = Cast<UDelegateProperty>(*PropertyIt) )
			{
				if ( !Property->GetName().EndsWith(TEXT("Delegate")) )
				{
					continue;
				}

				FString ConstPropertyName = Property->GetName();
				ConstPropertyName.RemoveFromEnd(TEXT("Delegate"));

				//TSharedRef<IPropertyHandle> DelegatePropertyHandle = DetailLayout.GetProperty(Property->GetFName(), PropertyClass);
				TSharedRef<IPropertyHandle> ConstPropertyHandle = DetailLayout.GetProperty(FName(*ConstPropertyName), PropertyClass);

				const bool bHasValidHandle = ConstPropertyHandle->IsValidHandle();

				if ( !bHasValidHandle )
				{
					continue;
				}

				UProperty* ConstProperty = ConstPropertyHandle->GetProperty();

				const bool bIsEditable = ConstProperty->HasAnyPropertyFlags(CPF_Edit | CPF_EditConst);
				const bool bDoSignaturesMatch = Property->SignatureFunction->GetReturnProperty()->SameType(ConstProperty);

				if ( !(bIsEditable && bDoSignaturesMatch) )
				{
					continue;
				}

				IDetailCategoryBuilder& PropertyCategory = DetailLayout.EditCategory(FObjectEditorUtils::GetCategoryFName(ConstProperty));

				IDetailPropertyRow& PropertyRow = PropertyCategory.AddProperty(ConstPropertyHandle);

				TSharedPtr<SWidget> NameWidget;
				TSharedPtr<SWidget> ValueWidget;
				FDetailWidgetRow Row;
				PropertyRow.GetDefaultWidgets(NameWidget, ValueWidget, Row);

				TSharedRef<SComboButton> DelegateButton = SNew(SComboButton)
				.OnGetMenuContent(this, &FBlueprintWidgetCustomization::OnGenerateDelegateMenu, ConstPropertyHandle)
				.ContentPadding(0)
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(this, &FBlueprintWidgetCustomization::GetCurrentBindingText, ConstPropertyHandle)
					.Font(IDetailLayoutBuilder::GetDetailFont())
				];

				const bool bShowChildren = true;
				PropertyRow.CustomWidget(bShowChildren)
				.NameContent()
				.MinDesiredWidth(Row.NameWidget.MinWidth)
				.MaxDesiredWidth(Row.NameWidget.MaxWidth)
				[
					NameWidget.ToSharedRef()
				]
				.ValueContent()
				.MinDesiredWidth(Row.ValueWidget.MinWidth * 2)
				.MaxDesiredWidth(Row.ValueWidget.MaxWidth * 2)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.FillWidth(1)
					.Padding(FMargin(0, 0, 4, 0))
					[
						ValueWidget.ToSharedRef()
					]
					
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						DelegateButton
					]
				];
			}
		}
	}
}

void FBlueprintWidgetCustomization::RefreshBlueprintFunctionCache()
{
	const UWidgetGraphSchema* Schema = GetDefault<UWidgetGraphSchema>();
	const FSlateFontInfo DetailFontInfo = IDetailLayoutBuilder::GetDetailFont();

	BlueprintFunctionCache.Reset();

	//TODO UMG Filter the list of functions to only be functions matching the signature.

	// Grab functions implemented by the blueprint
	for ( int32 i = 0; i < Blueprint->FunctionGraphs.Num(); i++ )
	{
		UEdGraph* Graph = Blueprint->FunctionGraphs[i];
		check(Graph);

		// Ignore the construction script, we just want user created functions.
		if ( Graph->GetFName() == Schema->FN_UserConstructionScript )
		{
			continue;
		}

		FGraphDisplayInfo DisplayInfo;
		Graph->GetSchema()->GetGraphDisplayInformation(*Graph, DisplayInfo);

		TSharedPtr<FunctionInfo> NewFuncAction = MakeShareable(new FunctionInfo());
		NewFuncAction->DisplayName = DisplayInfo.PlainName;
		NewFuncAction->Tooltip = DisplayInfo.Tooltip;
		NewFuncAction->FuncName = Graph->GetFName();
		NewFuncAction->EdGraph = Graph;

		BlueprintFunctionCache.Add(NewFuncAction);
	}
}

TSharedRef<SWidget> FBlueprintWidgetCustomization::OnGenerateDelegateMenu(TSharedRef<IPropertyHandle> PropertyHandle)
{
	RefreshBlueprintFunctionCache();

	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bInShouldCloseWindowAfterMenuSelection, NULL);

	MenuBuilder.BeginSection("ClearBinding");
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("FilterListResetFilters", "Remove Binding"),
			LOCTEXT("FilterListResetToolTip", "Removes the current binding"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &FBlueprintWidgetCustomization::HandleRemoveBinding, PropertyHandle))
			);
	}
	MenuBuilder.EndSection(); //ClearBinding

	MenuBuilder.BeginSection("Functions");
	{
		for ( TSharedPtr<FunctionInfo>& Function : BlueprintFunctionCache )
		{
			MenuBuilder.AddMenuEntry(
				Function->DisplayName,
				FText::FromString(Function->Tooltip),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FBlueprintWidgetCustomization::HandleAddBinding, PropertyHandle, Function))
				);
		}
	}
	MenuBuilder.EndSection(); //Functions


	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetDisplayMetrics(DisplayMetrics);

	return
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.MaxHeight(DisplayMetrics.PrimaryDisplayHeight * 0.5)
		[
			MenuBuilder.MakeWidget()
		];;
}

FText FBlueprintWidgetCustomization::GetCurrentBindingText(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	TArray<UObject*> OuterObjects;
	PropertyHandle->GetOuterObjects(OuterObjects);

	//TODO UMG O(N) Isn't good for this, needs to be map, but map isn't serialized, need cached runtime map for fast lookups.

	FName PropertyName = PropertyHandle->GetProperty()->GetFName();
	for ( int32 ObjectIndex = 0; ObjectIndex < OuterObjects.Num(); ObjectIndex++ )
	{
		//TODO UMG handle multiple things selected

		for ( const FDelegateEditorBinding& Binding : Blueprint->Bindings )
		{
			if ( Binding.PropertyName == PropertyName )
			{
				FName FoundName = Blueprint->GetFieldNameFromClassByGuid<UFunction>(Blueprint->GeneratedClass, Binding.MemberGuid);
				return FText::FromName(FoundName);
				//return FText::FromName(Binding.FunctionName);
			}
		}

		//TODO UMG Do something about missing functions, little exclamation points if they're missing and such.

		break;
	}

	return LOCTEXT("Bind", "Bind");
}

void FBlueprintWidgetCustomization::HandleRemoveBinding(TSharedRef<IPropertyHandle> PropertyHandle)
{
	const FScopedTransaction Transaction(LOCTEXT("BindDelegate", "Remove Binding"));

	Blueprint->Modify();

	TArray<UObject*> OuterObjects;
	PropertyHandle->GetOuterObjects(OuterObjects);
	for ( UObject* SelectedObject : OuterObjects )
	{
		FDelegateEditorBinding Binding;
		Binding.ObjectName = SelectedObject->GetName();
		Binding.PropertyName = PropertyHandle->GetProperty()->GetFName();

		Blueprint->Bindings.Remove(Binding);
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
}

void FBlueprintWidgetCustomization::HandleAddBinding(TSharedRef<IPropertyHandle> PropertyHandle, TSharedPtr<FunctionInfo> SelectedFunction)
{
	const FScopedTransaction Transaction(LOCTEXT("BindDelegate", "Set Binding"));

	Blueprint->Modify();

	TArray<UObject*> OuterObjects;
	PropertyHandle->GetOuterObjects(OuterObjects);
	for ( UObject* SelectedObject : OuterObjects )
	{
		FDelegateEditorBinding Binding;
		Binding.ObjectName = SelectedObject->GetName();
		Binding.PropertyName = PropertyHandle->GetProperty()->GetFName();
		Binding.FunctionName = SelectedFunction->FuncName;
		Binding.MemberGuid = SelectedFunction->EdGraph->GraphGuid;

		Blueprint->Bindings.Remove(Binding);
		Blueprint->Bindings.AddUnique(Binding);
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
}

#undef LOCTEXT_NAMESPACE
