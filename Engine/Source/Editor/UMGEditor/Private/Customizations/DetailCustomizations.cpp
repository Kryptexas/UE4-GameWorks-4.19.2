// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "DetailCustomizations.h"

#include "PropertyEditing.h"
#include "ObjectEditorUtils.h"
#include "WidgetGraphSchema.h"
#include "ScopedTransaction.h"
#include "BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "UMG"

void FBlueprintWidgetCustomization::CreateDelegateCustomization( IDetailLayoutBuilder& DetailLayout, UDelegateProperty* Property )
{
	FString ConstPropertyName = Property->GetName();
	ConstPropertyName.RemoveFromEnd(TEXT("Delegate"));

	//TSharedRef<IPropertyHandle> DelegatePropertyHandle = DetailLayout.GetProperty(Property->GetFName(), PropertyClass);
	TSharedRef<IPropertyHandle> ConstPropertyHandle = DetailLayout.GetProperty(FName(*ConstPropertyName), CastChecked<UClass>(Property->GetOuter()));

	const bool bHasValidHandle = ConstPropertyHandle->IsValidHandle();
	if(!bHasValidHandle)
	{
		return;
	}

	UProperty* ConstProperty = ConstPropertyHandle->GetProperty();

	const bool bIsEditable = ConstProperty->HasAnyPropertyFlags(CPF_Edit | CPF_EditConst);
	const bool bDoSignaturesMatch = Property->SignatureFunction->GetReturnProperty()->SameType(ConstProperty);

	if(!(bIsEditable && bDoSignaturesMatch))
	{
		return;
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
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(this, &FBlueprintWidgetCustomization::GetCurrentBindingImage, ConstPropertyHandle)
				.ColorAndOpacity(FLinearColor(0.25f, 0.25f, 0.25f))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4, 1, 0, 0)
			[
				SNew(STextBlock)
				.Text(this, &FBlueprintWidgetCustomization::GetCurrentBindingText, ConstPropertyHandle)
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
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
			.VAlign(VAlign_Center)
			[
				DelegateButton
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
				.Visibility(this, &FBlueprintWidgetCustomization::GetGotoBindingVisibility, ConstPropertyHandle)
				.OnClicked(this, &FBlueprintWidgetCustomization::HandleGotoBindingClicked, ConstPropertyHandle)
				.VAlign(VAlign_Center)
				.ToolTipText(LOCTEXT("GotoFunction", "Goto Function"))
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("PropertyWindow.Button_Browse"))
				]
			]
		];
}

void FBlueprintWidgetCustomization::CreateEventCustomization( IDetailLayoutBuilder& DetailLayout, UDelegateProperty* Property )
{
	if(!Property->GetName().EndsWith(TEXT("Event")))
	{
		return;
	}

	TSharedRef<IPropertyHandle> DelegatePropertyHandle = DetailLayout.GetProperty(Property->GetFName(), CastChecked<UClass>(Property->GetOuter()));

	const bool bHasValidHandle = DelegatePropertyHandle->IsValidHandle();
	if(!bHasValidHandle)
	{
		return;
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
			.VAlign(VAlign_Center)
			[
				DelegateButton
			]

			+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
					.Visibility(this, &FBlueprintWidgetCustomization::GetGotoBindingVisibility, DelegatePropertyHandle)
					.OnClicked(this, &FBlueprintWidgetCustomization::HandleGotoBindingClicked, DelegatePropertyHandle)
					.VAlign(VAlign_Center)
					.ToolTipText(LOCTEXT("GotoFunction", "Goto Function"))
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("PropertyWindow.Button_Browse"))
					]
				]
		];
}

void FBlueprintWidgetCustomization::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	TArray< TWeakObjectPtr<UObject> > OutObjects;
	DetailLayout.GetObjectsBeingCustomized(OutObjects);

	if ( OutObjects.Num() == 1 )
	{
		UClass* PropertyClass = OutObjects[0].Get()->GetClass();

		for ( TFieldIterator<UProperty> PropertyIt(PropertyClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt )
		{
			UProperty* Property = *PropertyIt;
			if ( UDelegateProperty* DelegateProperty = Cast<UDelegateProperty>(*PropertyIt) )
			{
				if( DelegateProperty->GetName().EndsWith(TEXT("Delegate")) )
				{
					CreateDelegateCustomization( DetailLayout, DelegateProperty );
				}
				else if( DelegateProperty->GetName().EndsWith(TEXT("Event")) )
				{
					CreateEventCustomization( DetailLayout, DelegateProperty );
				}
			}
		}
	}
}

void FBlueprintWidgetCustomization::RefreshBlueprintMemberCache(const UFunction* DelegateSignature)
{
	const UWidgetGraphSchema* Schema = GetDefault<UWidgetGraphSchema>();
	const FSlateFontInfo DetailFontInfo = IDetailLayoutBuilder::GetDetailFont();

	BlueprintFunctionCache.Reset();
	BlueprintPropertyCache.Reset();

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

	// Grab functions implemented by the blueprint
	for ( TFieldIterator<UProperty> PropIt(SkeletonClass, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt )
	{
		UProperty* Prop = *PropIt;
		
		if ( DelegateSignature->GetReturnProperty()->SameType(Prop) )
		{
			if ( Prop->HasAnyPropertyFlags(UP::BlueprintReadWrite) )
			{
				BlueprintPropertyCache.Add(Prop);
			}
		}
	}
}

TSharedRef<SWidget> FBlueprintWidgetCustomization::OnGenerateDelegateMenu(TSharedRef<IPropertyHandle> PropertyHandle, UFunction* DelegateSignature)
{
	RefreshBlueprintMemberCache(DelegateSignature);

	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bInShouldCloseWindowAfterMenuSelection, NULL);

	static FName PropertyIcon(TEXT("Kismet.Tabs.Variables"));
	static FName FunctionIcon(TEXT("GraphEditor.Function_16x"));

	//FLinearColor ColorOut;
	//const UClass* VarClass = PropertyHandle->GetPropertyClass();
	//BrushOut = FBlueprintEditor::GetVarIconAndColor(VarClass, PropertyHandle->GetProperty()->GetFName(), ColorOut);

	if ( CanRemoveBinding(PropertyHandle) )
	{
		MenuBuilder.BeginSection("RemoveBinding");
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("RemoveBinding", "Remove Binding"),
				LOCTEXT("RemoveBindingToolTip", "Removes the current binding"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FBlueprintWidgetCustomization::HandleRemoveBinding, PropertyHandle))
				);
		}
		MenuBuilder.EndSection(); //RemoveBinding
	}

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

	MenuBuilder.BeginSection("Functions", LOCTEXT("Functions", "Functions"));
	{
		for ( TSharedPtr<FunctionInfo>& Function : BlueprintFunctionCache )
		{
			MenuBuilder.AddMenuEntry(
				Function->DisplayName,
				FText::FromString(Function->Tooltip),
				FSlateIcon(FEditorStyle::GetStyleSetName(), FunctionIcon),
				FUIAction(FExecuteAction::CreateSP(this, &FBlueprintWidgetCustomization::HandleAddFunctionBinding, PropertyHandle, Function))
				);
		}
	}
	MenuBuilder.EndSection(); //Functions

	MenuBuilder.BeginSection("Properties", LOCTEXT("Properties", "Properties"));
	{
		for ( UProperty* ExistingProperty : BlueprintPropertyCache )
		{
			MenuBuilder.AddMenuEntry(
				ExistingProperty->GetDisplayNameText(),
				ExistingProperty->GetToolTipText(),
				FSlateIcon(FEditorStyle::GetStyleSetName(), PropertyIcon),
				FUIAction(FExecuteAction::CreateSP(this, &FBlueprintWidgetCustomization::HandleAddPropertyBinding, PropertyHandle, ExistingProperty))
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

const FSlateBrush* FBlueprintWidgetCustomization::GetCurrentBindingImage(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	static FName PropertyIcon(TEXT("Kismet.Tabs.Variables"));
	static FName FunctionIcon(TEXT("GraphEditor.Function_16x"));

	TArray<UObject*> OuterObjects;
	PropertyHandle->GetOuterObjects(OuterObjects);

	//TODO UMG O(N) Isn't good for this, needs to be map, but map isn't serialized, need cached runtime map for fast lookups.

	FName PropertyName = PropertyHandle->GetProperty()->GetFName();
	for ( int32 ObjectIndex = 0; ObjectIndex < OuterObjects.Num(); ObjectIndex++ )
	{
		// Ignore null outer objects
		if ( OuterObjects[ObjectIndex] == NULL )
		{
			continue;
		}

		//TODO UMG handle multiple things selected

		for ( const FDelegateEditorBinding& Binding : Blueprint->Bindings )
		{
			if ( Binding.ObjectName == OuterObjects[ObjectIndex]->GetName() && Binding.PropertyName == PropertyName )
			{
				if ( Binding.Kind == EBindingKind::Function )
				{
					return FEditorStyle::GetBrush(FunctionIcon);
				}
				else // Property
				{
					return FEditorStyle::GetBrush(PropertyIcon);
				}
			}
		}
	}

	return NULL;
}

FText FBlueprintWidgetCustomization::GetCurrentBindingText(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	TArray<UObject*> OuterObjects;
	PropertyHandle->GetOuterObjects(OuterObjects);

	//TODO UMG O(N) Isn't good for this, needs to be map, but map isn't serialized, need cached runtime map for fast lookups.

	FName PropertyName = PropertyHandle->GetProperty()->GetFName();
	for ( int32 ObjectIndex = 0; ObjectIndex < OuterObjects.Num(); ObjectIndex++ )
	{
		// Ignore null outer objects
		if ( OuterObjects[ObjectIndex] == NULL )
		{
			continue;
		}

		//TODO UMG handle multiple things selected

		for ( const FDelegateEditorBinding& Binding : Blueprint->Bindings )
		{
			if ( Binding.ObjectName == OuterObjects[ObjectIndex]->GetName() && Binding.PropertyName == PropertyName )
			{
				if ( Binding.Kind == EBindingKind::Function )
				{
					FName FoundName = Blueprint->GetFieldNameFromClassByGuid<UFunction>(Blueprint->GeneratedClass, Binding.MemberGuid);
					return FText::FromString(FName::NameToDisplayString(FoundName.ToString(), false));
				}
				else // Property
				{
					FName FoundName = Blueprint->GetFieldNameFromClassByGuid<UProperty>(Blueprint->GeneratedClass, Binding.MemberGuid);
					return FText::FromString( FName::NameToDisplayString(FoundName.ToString(), false) );
				}
			}
		}

		//TODO UMG Do something about missing functions, little exclamation points if they're missing and such.

		break;
	}

	return LOCTEXT("Bind", "Bind");
}

bool FBlueprintWidgetCustomization::CanRemoveBinding(TSharedRef<IPropertyHandle> PropertyHandle)
{
	FName PropertyName = PropertyHandle->GetProperty()->GetFName();

	TArray<UObject*> OuterObjects;
	PropertyHandle->GetOuterObjects(OuterObjects);
	for ( UObject* SelectedObject : OuterObjects )
	{
		for ( const FDelegateEditorBinding& Binding : Blueprint->Bindings )
		{
			if ( Binding.ObjectName == SelectedObject->GetName() && Binding.PropertyName == PropertyName )
			{
				return true;
			}
		}
	}

	return false;
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

void FBlueprintWidgetCustomization::HandleAddFunctionBinding(TSharedRef<IPropertyHandle> PropertyHandle, TSharedPtr<FunctionInfo> SelectedFunction)
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
		Binding.Kind = EBindingKind::Function;

		Blueprint->Bindings.Remove(Binding);
		Blueprint->Bindings.AddUnique(Binding);
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
}

void FBlueprintWidgetCustomization::HandleAddPropertyBinding(TSharedRef<IPropertyHandle> PropertyHandle, UProperty* SelectedProperty)
{
	const FScopedTransaction Transaction(LOCTEXT("BindDelegate", "Set Binding"));

	// Get the current skeleton class, think header for the blueprint.
	UBlueprintGeneratedClass* SkeletonClass = Cast<UBlueprintGeneratedClass>(Blueprint->SkeletonGeneratedClass);

	Blueprint->Modify();

	FGuid MemberGuid;
	UBlueprint::GetGuidFromClassByFieldName<UProperty>(SkeletonClass, SelectedProperty->GetFName(), MemberGuid);

	TArray<UObject*> OuterObjects;
	PropertyHandle->GetOuterObjects(OuterObjects);
	for ( UObject* SelectedObject : OuterObjects )
	{
		FDelegateEditorBinding Binding;
		Binding.ObjectName = SelectedObject->GetName();
		Binding.PropertyName = PropertyHandle->GetProperty()->GetFName();
		Binding.SourceProperty = SelectedProperty->GetFName();
		Binding.MemberGuid = MemberGuid;
		Binding.Kind = EBindingKind::Property;

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

	HandleAddFunctionBinding(PropertyHandle, SelectedFunction);

	GotoFunction(FunctionGraph);
}

EVisibility FBlueprintWidgetCustomization::GetGotoBindingVisibility(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	TArray<UObject*> OuterObjects;
	PropertyHandle->GetOuterObjects(OuterObjects);

	//TODO UMG O(N) Isn't good for this, needs to be map, but map isn't serialized, need cached runtime map for fast lookups.

	FName PropertyName = PropertyHandle->GetProperty()->GetFName();
	for ( int32 ObjectIndex = 0; ObjectIndex < OuterObjects.Num(); ObjectIndex++ )
	{
		// Ignore null outer objects
		if ( OuterObjects[ObjectIndex] == NULL )
		{
			continue;
		}

		//TODO UMG handle multiple things selected

		for ( const FDelegateEditorBinding& Binding : Blueprint->Bindings )
		{
			if ( Binding.ObjectName == OuterObjects[ObjectIndex]->GetName() && Binding.PropertyName == PropertyName )
			{
				if ( Binding.Kind == EBindingKind::Function )
				{
					return EVisibility::Visible;
				}
			}
		}
	}

	return EVisibility::Collapsed;
}

FReply FBlueprintWidgetCustomization::HandleGotoBindingClicked(TSharedRef<IPropertyHandle> PropertyHandle)
{
	TArray<UObject*> OuterObjects;
	PropertyHandle->GetOuterObjects(OuterObjects);

	//TODO UMG O(N) Isn't good for this, needs to be map, but map isn't serialized, need cached runtime map for fast lookups.

	FName PropertyName = PropertyHandle->GetProperty()->GetFName();
	for ( int32 ObjectIndex = 0; ObjectIndex < OuterObjects.Num(); ObjectIndex++ )
	{
		// Ignore null outer objects
		if ( OuterObjects[ObjectIndex] == NULL )
		{
			continue;
		}

		//TODO UMG handle multiple things selected

		for ( const FDelegateEditorBinding& Binding : Blueprint->Bindings )
		{
			if ( Binding.ObjectName == OuterObjects[ObjectIndex]->GetName() && Binding.PropertyName == PropertyName )
			{
				if ( Binding.Kind == EBindingKind::Function )
				{
					TArray<UEdGraph*> AllGraphs;
					Blueprint->GetAllGraphs(AllGraphs);

					for ( UEdGraph* Graph : AllGraphs )
					{
						if ( Graph->GraphGuid == Binding.MemberGuid )
						{
							GotoFunction(Graph);
						}
					}


					// Either way return
					return FReply::Handled();
				}
			}
		}
	}

	return FReply::Unhandled();
}

void FBlueprintWidgetCustomization::GotoFunction(UEdGraph* FunctionGraph)
{
	Editor.Pin()->SetCurrentMode(FWidgetBlueprintApplicationModes::GraphMode);

	Editor.Pin()->OpenDocument(FunctionGraph, FDocumentTracker::OpenNewDocument);
}

#undef LOCTEXT_NAMESPACE
