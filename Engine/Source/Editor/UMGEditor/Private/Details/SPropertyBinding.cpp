// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "DetailLayoutBuilder.h"
#include "PropertyHandle.h"
#include "SPropertyBinding.h"
#include "WidgetBlueprintApplicationModes.h"
#include "WidgetGraphSchema.h"
#include "BlueprintEditorUtils.h"
#include "ScopedTransaction.h"


#define LOCTEXT_NAMESPACE "UMG"


/////////////////////////////////////////////////////
// SPropertyBinding

void SPropertyBinding::Construct(const FArguments& InArgs, TSharedRef<FWidgetBlueprintEditor> InEditor, UDelegateProperty* DelegateProperty, TSharedRef<IPropertyHandle> Property)
{
	Editor = InEditor;
	Blueprint = InEditor->GetWidgetBlueprintObj();

	TArray<UObject*> Objects;
	Property->GetOuterObjects(Objects);

	UWidget* Widget = CastChecked<UWidget>(Objects[0]);

	ChildSlot
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SComboButton)
			.OnGetMenuContent(this, &SPropertyBinding::OnGenerateDelegateMenu, Widget, Property, DelegateProperty->SignatureFunction, InArgs._GeneratePureBindings)
			.ContentPadding(1)
			.ButtonContent()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(this, &SPropertyBinding::GetCurrentBindingImage, Property)
					.ColorAndOpacity(FLinearColor(0.25f, 0.25f, 0.25f))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(4, 1, 0, 0)
				[
					SNew(STextBlock)
					.Text(this, &SPropertyBinding::GetCurrentBindingText, Property)
					.Font(IDetailLayoutBuilder::GetDetailFont())
				]
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
			.Visibility(this, &SPropertyBinding::GetGotoBindingVisibility, Property)
			.OnClicked(this, &SPropertyBinding::HandleGotoBindingClicked, Property)
			.VAlign(VAlign_Center)
			.ToolTipText(LOCTEXT("GotoFunction", "Goto Function"))
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("PropertyWindow.Button_Browse"))
			]
		]
	];
}

void SPropertyBinding::RefreshBlueprintMemberCache(const UFunction* DelegateSignature, bool bIsPure)
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
		if ( Function == nullptr )
		{
			continue;
		}

		// We ignore CPF_ReturnParm because all that matters for binding to script functions is that the number of out parameters match.
		if ( Function->IsSignatureCompatibleWith(DelegateSignature, UFunction::GetDefaultIgnoredSignatureCompatibilityFlags() | CPF_ReturnParm) )
		{
			// Only allow binding pure functions if we're limited to pure function bindings.
			if ( bIsPure && !Function->HasAllFunctionFlags(FUNC_BlueprintPure) )
			{
				continue;
			}
			
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

	// Walk up class hierarchy for native functions and properties
	for ( UClass* Class = SkeletonClass; Class != UUserWidget::StaticClass(); Class = Class->GetSuperClass() )
	{
		// Add matching native functions
		for ( TFieldIterator<UFunction> FuncIt(Class, EFieldIteratorFlags::ExcludeSuper); FuncIt; ++FuncIt )
		{
			UFunction* Func = *FuncIt;

			if ( !Func->HasAnyFunctionFlags(UF::BlueprintCallable | UF::BlueprintPure) )
			{
				continue;
			}

			FName FunctionFName = Func->GetFName();

			UFunction* Function = Class->FindFunctionByName(FunctionFName, EIncludeSuperFlag::IncludeSuper);
			if ( Function == nullptr )
			{
				continue;
			}

			// We ignore CPF_ReturnParm because all that matters for binding to script functions is that the number of out parameters match.
			if ( Function->IsSignatureCompatibleWith(DelegateSignature, UFunction::GetDefaultIgnoredSignatureCompatibilityFlags() | CPF_ReturnParm) )
			{
				TSharedPtr<FunctionInfo> NewFuncAction = MakeShareable(new FunctionInfo());
				NewFuncAction->DisplayName = FText::FromName(Func->GetFName());
				NewFuncAction->Tooltip = Func->GetMetaData("Tooltip");
				NewFuncAction->FuncName = FunctionFName;
				NewFuncAction->EdGraph = nullptr;

				BlueprintFunctionCache.Add(NewFuncAction);
			}
		}

		// Grab functions implemented by the blueprint
		for ( TFieldIterator<UProperty> PropIt(Class, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt )
		{
			UProperty* Prop = *PropIt;

			// Add matching properties.
			if ( UProperty* ReturnProperty = DelegateSignature->GetReturnProperty() )
			{
				if ( ReturnProperty->SameType(Prop) )
				{
					if ( Prop->HasAnyPropertyFlags(UP::BlueprintReadWrite | UP::BlueprintReadOnly) )
					{
						BlueprintPropertyCache.Add(Prop);
					}
				}
			}
		}
	}
}

TSharedRef<SWidget> SPropertyBinding::OnGenerateDelegateMenu(UWidget* Widget, TSharedRef<IPropertyHandle> PropertyHandle, UFunction* DelegateSignature, bool bIsPure)
{
	RefreshBlueprintMemberCache(DelegateSignature, bIsPure);

	const bool bInShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bInShouldCloseWindowAfterMenuSelection, nullptr);

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
				FUIAction(FExecuteAction::CreateSP(this, &SPropertyBinding::HandleRemoveBinding, PropertyHandle))
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
			FUIAction(FExecuteAction::CreateSP(this, &SPropertyBinding::HandleCreateAndAddBinding, Widget, PropertyHandle, DelegateSignature, bIsPure))
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
				FUIAction(FExecuteAction::CreateSP(this, &SPropertyBinding::HandleAddFunctionBinding, PropertyHandle, Function))
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
				FUIAction(FExecuteAction::CreateSP(this, &SPropertyBinding::HandleAddPropertyBinding, PropertyHandle, ExistingProperty))
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

const FSlateBrush* SPropertyBinding::GetCurrentBindingImage(TSharedRef<IPropertyHandle> PropertyHandle) const
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

	return nullptr;
}

FText SPropertyBinding::GetCurrentBindingText(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	TArray<UObject*> OuterObjects;
	PropertyHandle->GetOuterObjects(OuterObjects);

	//TODO UMG O(N) Isn't good for this, needs to be map, but map isn't serialized, need cached runtime map for fast lookups.

	FName PropertyName = PropertyHandle->GetProperty()->GetFName();
	for ( int32 ObjectIndex = 0; ObjectIndex < OuterObjects.Num(); ObjectIndex++ )
	{
		// Ignore null outer objects
		if ( OuterObjects[ObjectIndex] == nullptr )
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
					if ( Binding.MemberGuid.IsValid() )
					{
						// Graph function, look up by Guid
						FName FoundName = Blueprint->GetFieldNameFromClassByGuid<UFunction>(Blueprint->GeneratedClass, Binding.MemberGuid);
						return FText::FromString(FName::NameToDisplayString(FoundName.ToString(), false));
					}
					else
					{
						// No GUID, native function, return function name.
						return FText::FromName(Binding.FunctionName);
					}
				}
				else // Property
				{
					if ( Binding.MemberGuid.IsValid() )
					{
						FName FoundName = Blueprint->GetFieldNameFromClassByGuid<UProperty>(Blueprint->GeneratedClass, Binding.MemberGuid);
						return FText::FromString(FName::NameToDisplayString(FoundName.ToString(), false));
					}
					else
					{
						// No GUID, native property, return source property.
						return FText::FromName(Binding.SourceProperty);
					}
				}
			}
		}

		//TODO UMG Do something about missing functions, little exclamation points if they're missing and such.

		break;
	}

	return LOCTEXT("Bind", "Bind");
}

bool SPropertyBinding::CanRemoveBinding(TSharedRef<IPropertyHandle> PropertyHandle)
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

void SPropertyBinding::HandleRemoveBinding(TSharedRef<IPropertyHandle> PropertyHandle)
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

void SPropertyBinding::HandleAddFunctionBinding(TSharedRef<IPropertyHandle> PropertyHandle, TSharedPtr<FunctionInfo> SelectedFunction)
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
		Binding.MemberGuid = ( SelectedFunction->EdGraph ) ? SelectedFunction->EdGraph->GraphGuid : FGuid();
		Binding.Kind = EBindingKind::Function;

		Blueprint->Bindings.Remove(Binding);
		Blueprint->Bindings.AddUnique(Binding);
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
}

void SPropertyBinding::HandleAddPropertyBinding(TSharedRef<IPropertyHandle> PropertyHandle, UProperty* SelectedProperty)
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

void SPropertyBinding::HandleCreateAndAddBinding(UWidget* Widget, TSharedRef<IPropertyHandle> PropertyHandle, UFunction* DelegateSignature, bool bIsPure)
{
	const FScopedTransaction Transaction(LOCTEXT("CreateDelegate", "Create Binding"));

	Blueprint->Modify();

	FString Pre = bIsPure ? FString(TEXT("Get")) : FString(TEXT("On"));

	FString WidgetName;
	if ( Widget && !Widget->IsGeneratedName() )
	{
		WidgetName = TEXT("_") + Widget->GetName() + TEXT("_");
	}

	FString Post = PropertyHandle->GetProperty()->GetName();
	Post.RemoveFromStart(TEXT("On"));
	Post.RemoveFromEnd(TEXT("Event"));

	// Create the function graph.
	FString FunctionName = Pre + WidgetName + Post;
	UEdGraph* FunctionGraph = FBlueprintEditorUtils::CreateNewGraph(
		Blueprint, 
		FBlueprintEditorUtils::FindUniqueKismetName(Blueprint, FunctionName),
		UEdGraph::StaticClass(),
		UEdGraphSchema_K2::StaticClass());
	
	// Add the binding to the blueprint
	TSharedPtr<FunctionInfo> SelectedFunction = MakeShareable(new FunctionInfo());
	SelectedFunction->FuncName = FunctionGraph->GetFName();
	SelectedFunction->EdGraph = FunctionGraph;

	HandleAddFunctionBinding(PropertyHandle, SelectedFunction);

	const bool bUserCreated = true;
	FBlueprintEditorUtils::AddFunctionGraph(Blueprint, FunctionGraph, bUserCreated, DelegateSignature);

	// Only mark bindings as pure that need to be.
	if ( bIsPure )
	{
		const UEdGraphSchema_K2* Schema_K2 = Cast<UEdGraphSchema_K2>(FunctionGraph->GetSchema());
		Schema_K2->AddExtraFunctionFlags(FunctionGraph, FUNC_BlueprintPure);
	}

	GotoFunction(FunctionGraph);
}

EVisibility SPropertyBinding::GetGotoBindingVisibility(TSharedRef<IPropertyHandle> PropertyHandle) const
{
	TArray<UObject*> OuterObjects;
	PropertyHandle->GetOuterObjects(OuterObjects);

	//TODO UMG O(N) Isn't good for this, needs to be map, but map isn't serialized, need cached runtime map for fast lookups.

	FName PropertyName = PropertyHandle->GetProperty()->GetFName();
	for ( int32 ObjectIndex = 0; ObjectIndex < OuterObjects.Num(); ObjectIndex++ )
	{
		// Ignore null outer objects
		if ( OuterObjects[ObjectIndex] == nullptr )
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

FReply SPropertyBinding::HandleGotoBindingClicked(TSharedRef<IPropertyHandle> PropertyHandle)
{
	TArray<UObject*> OuterObjects;
	PropertyHandle->GetOuterObjects(OuterObjects);

	//TODO UMG O(N) Isn't good for this, needs to be map, but map isn't serialized, need cached runtime map for fast lookups.

	FName PropertyName = PropertyHandle->GetProperty()->GetFName();
	for ( int32 ObjectIndex = 0; ObjectIndex < OuterObjects.Num(); ObjectIndex++ )
	{
		// Ignore null outer objects
		if ( OuterObjects[ObjectIndex] == nullptr )
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

void SPropertyBinding::GotoFunction(UEdGraph* FunctionGraph)
{
	Editor.Pin()->SetCurrentMode(FWidgetBlueprintApplicationModes::GraphMode);

	Editor.Pin()->OpenDocument(FunctionGraph, FDocumentTracker::OpenNewDocument);
}


#undef LOCTEXT_NAMESPACE
