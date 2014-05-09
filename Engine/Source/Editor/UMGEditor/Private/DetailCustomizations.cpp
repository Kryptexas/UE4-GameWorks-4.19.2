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
				TSharedRef<IPropertyHandle> ConstPropertyHandle = DetailLayout.GetProperty(FName(*ConstPropertyName), CastChecked<UClass>(Property->GetOuter()));

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
				.OnGetMenuContent(this, &FBlueprintWidgetCustomization::OnGenerateDelegateMenu, ConstPropertyHandle, Property->SignatureFunction)
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

		//--------------------------------------------

		for ( TFieldIterator<UProperty> PropertyIt(PropertyClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt )
		{
			if ( UDelegateProperty* Property = Cast<UDelegateProperty>(*PropertyIt) )
			{
				if ( !Property->GetName().EndsWith(TEXT("Event")) )
				{
					continue;
				}

				TSharedRef<IPropertyHandle> DelegatePropertyHandle = DetailLayout.GetProperty(Property->GetFName(), CastChecked<UClass>(Property->GetOuter()));

				const bool bHasValidHandle = DelegatePropertyHandle->IsValidHandle();

				if ( !bHasValidHandle )
				{
					continue;
				}

				IDetailCategoryBuilder& PropertyCategory = DetailLayout.EditCategory(FObjectEditorUtils::GetCategoryFName(Property));

				IDetailPropertyRow& PropertyRow = PropertyCategory.AddProperty(DelegatePropertyHandle);

				TSharedPtr<SWidget> NameWidget;
				TSharedPtr<SWidget> ValueWidget;
				FDetailWidgetRow Row;
				PropertyRow.GetDefaultWidgets(NameWidget, ValueWidget, Row);

				TSharedRef<SComboButton> DelegateButton = SNew(SComboButton)
				.OnGetMenuContent(this, &FBlueprintWidgetCustomization::OnGenerateDelegateMenu, DelegatePropertyHandle, Property->SignatureFunction)
				.ContentPadding(0)
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(this, &FBlueprintWidgetCustomization::GetCurrentBindingText, DelegatePropertyHandle)
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
				.MinDesiredWidth(Row.ValueWidget.MinWidth)
				.MaxDesiredWidth(Row.ValueWidget.MaxWidth)
				[
					SNew(SHorizontalBox)
					
					+ SHorizontalBox::Slot()
					.FillWidth(1)
					[
						DelegateButton
					]
				];
			}
		}

	}
}

void FBlueprintWidgetCustomization::RefreshBlueprintFunctionCache(const UFunction* DelegateSignature)
{
	const UWidgetGraphSchema* Schema = GetDefault<UWidgetGraphSchema>();
	const FSlateFontInfo DetailFontInfo = IDetailLayoutBuilder::GetDetailFont();

	BlueprintFunctionCache.Reset();

	// Get the current skeleton class, think header for the blueprint.
	UBlueprintGeneratedClass* SkeletonClass = Cast<UBlueprintGeneratedClass>(Blueprint->SkeletonGeneratedClass);

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

		FName FunctionFName = Graph->GetFName();

		UFunction* Function = SkeletonClass->FindFunctionByName(FunctionFName, EIncludeSuperFlag::IncludeSuper);
		if ( Function == NULL )
		{
			continue;
		}

		// We ignore CPF_ReturnParm because all that matters for binding to script functions is that the number of out parameters match.
		if ( Function->IsSignatureCompatibleWith(DelegateSignature, UFunction::GetDefaultIgnoredSignatureCompatibilityFlags() | CPF_ReturnParm) )
		{
			FGraphDisplayInfo DisplayInfo;
			Graph->GetSchema()->GetGraphDisplayInformation(*Graph, DisplayInfo);

			TSharedPtr<FunctionInfo> NewFuncAction = MakeShareable(new FunctionInfo());
			NewFuncAction->DisplayName = DisplayInfo.PlainName;
			NewFuncAction->Tooltip = DisplayInfo.Tooltip;
			NewFuncAction->FuncName = FunctionFName;
			NewFuncAction->EdGraph = Graph;

			BlueprintFunctionCache.Add(NewFuncAction);
		}
	}
}

TSharedRef<SWidget> FBlueprintWidgetCustomization::OnGenerateDelegateMenu(TSharedRef<IPropertyHandle> PropertyHandle, UFunction* DelegateSignature)
{
	RefreshBlueprintFunctionCache(DelegateSignature);

	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bInShouldCloseWindowAfterMenuSelection, NULL);

	//TODO UMG Enable or disable Remove Binding if needed.

	MenuBuilder.BeginSection("ClearBinding");
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("RemoveBinding", "Remove Binding"),
			LOCTEXT("RemoveBindingToolTip", "Removes the current binding"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &FBlueprintWidgetCustomization::HandleRemoveBinding, PropertyHandle))
			);
	}
	MenuBuilder.EndSection(); //ClearBinding

	MenuBuilder.BeginSection("CreateBinding");
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("CreateBinding", "Create Binding"),
			LOCTEXT("CreateBindingToolTip", "Creates a new function for this property to be bound to"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &FBlueprintWidgetCustomization::HandleCreateAndAddBinding, PropertyHandle, DelegateSignature))
			);
	}
	MenuBuilder.EndSection(); //CreateBinding

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
		];
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
			if ( Binding.ObjectName == OuterObjects[ObjectIndex]->GetName() && Binding.PropertyName == PropertyName )
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

void FBlueprintWidgetCustomization::HandleCreateAndAddBinding(TSharedRef<IPropertyHandle> PropertyHandle, UFunction* DelegateSignature)
{
	const FScopedTransaction Transaction(LOCTEXT("CreateDelegate", "Create Binding"));

	Blueprint->Modify();

	// Create the function graph.
	FString FunctionName = FString(TEXT("Get")) + PropertyHandle->GetProperty()->GetName();
	UEdGraph* FunctionGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, FBlueprintEditorUtils::FindUniqueKismetName(Blueprint, FunctionName), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());

	bool bUserCreated = true;
	FBlueprintEditorUtils::AddFunctionGraph(Blueprint, FunctionGraph, bUserCreated, DelegateSignature);

	//TODO UMG Once we start binding event functions, pure needs to be variable, so maybe pass it in.
	
	// All bound delegates should be pure.
	const UEdGraphSchema_K2* Schema_K2 = Cast<UEdGraphSchema_K2>(FunctionGraph->GetSchema());
	Schema_K2->AddExtraFunctionFlags(FunctionGraph, FUNC_BlueprintPure);

	// Add the binding to the blueprint
	TSharedPtr<FunctionInfo> SelectedFunction = MakeShareable(new FunctionInfo());
	SelectedFunction->FuncName = FunctionGraph->GetFName();
	SelectedFunction->EdGraph = FunctionGraph;

	HandleAddBinding(PropertyHandle, SelectedFunction);
}

#undef LOCTEXT_NAMESPACE
