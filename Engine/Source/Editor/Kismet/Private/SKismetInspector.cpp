// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintEditorPrivatePCH.h"
#include "GraphEditor.h"
#include "BlueprintUtilities.h"
#include "AnimGraphDefinitions.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "EngineKismetLibraryClasses.h"
#include "SKismetInspector.h"
#include "SKismetLinearExpression.h"
#include "STutorialWrapper.h"

#include "Editor/PropertyEditor/Public/PropertyEditing.h"

#include "SMyBlueprint.h"
#include "BlueprintDetailsCustomization.h"
#include "UserDefinedEnumEditor.h"
#include "FormatTextDetails.h"

#define LOCTEXT_NAMESPACE "KismetInspector"

//////////////////////////////////////////////////////////////////////////
// FKismetSelectionInfo

struct FKismetSelectionInfo
{
public:
	TArray<UActorComponent*> EditableComponentTemplates;
	TArray<UObject*> ObjectsForPropertyEditing;
};

//////////////////////////////////////////////////////////////////////////
// SKismetInspector

TSharedRef<SWidget> SKismetInspector::MakeContextualEditingWidget(struct FKismetSelectionInfo& SelectionInfo, const FShowDetailsOptions& Options)
{
	TSharedRef< SVerticalBox > ContextualEditingWidget = SNew( SVerticalBox );
	TSharedPtr<SMyBlueprint> MyBlueprintWidget = Kismet2Ptr.Pin()->GetMyBlueprintWidget();

	if(bShowTitleArea)
	{
		if (SelectedObjects.Num() == 0)
		{
			// Warning about nothing being selected
			ContextualEditingWidget->AddSlot()
			.AutoHeight()
			.HAlign( HAlign_Center )
			.Padding( 2.0f, 14.0f, 2.0f, 2.0f )
			[
				SNew( STextBlock )
				.Text( LOCTEXT("NoNodesSelected", "Select a node to edit details.").ToString() )
			];
		}
		else
		{
			// Title of things being edited
			ContextualEditingWidget->AddSlot()
			.AutoHeight()
			.Padding( 2.0f, 0.0f, 2.0f, 0.0f )
			[
				SNew(STextBlock)
				.Text(this, &SKismetInspector::GetContextualEditingWidgetTitle)
			];
		}
	}

	// Show the property editor
	PropertyView->HideFilterArea(Options.bHideFilterArea);
	PropertyView->SetObjects(SelectionInfo.ObjectsForPropertyEditing, Options.bForceRefresh);
	if (SelectionInfo.ObjectsForPropertyEditing.Num())
	{
		ContextualEditingWidget->AddSlot()
			.FillHeight( 0.9f )
			.VAlign( VAlign_Top )
			[
				SNew( SBorder )
				.Visibility(this, &SKismetInspector::GetPropertyViewVisibility)
				.BorderImage( FEditorStyle::GetBrush("NoBorder") )
				[
					PropertyView.ToSharedRef()
				]
			];

		if (bShowPublicView)
		{
			ContextualEditingWidget->AddSlot()
				.AutoHeight()
				.VAlign( VAlign_Top )
				[
					SNew(SCheckBox)
					.ToolTipText(LOCTEXT("TogglePublicView", "Toggle Public View").ToString())
					.IsChecked(this, &SKismetInspector::GetPublicViewCheckboxState)
					.OnCheckStateChanged( this, &SKismetInspector::SetPublicViewCheckboxState)
					[
						SNew(STextBlock) .Text(LOCTEXT("PublicViewCheckboxLabel", "Public View").ToString())
					]
				];
		}
	}

	return ContextualEditingWidget;
}

FString SKismetInspector::GetContextualEditingWidgetTitle() const
{
	FString Title = PropertyViewTitle;
	if (Title.IsEmpty())
	{
		if (SelectedObjects.Num() == 1 && SelectedObjects[0].IsValid())
		{
			UObject* Object = SelectedObjects[0].Get();

			if (UEdGraphNode* Node = Cast<UEdGraphNode>(Object))
			{
				Title = Node->GetNodeTitle(ENodeTitleType::ListView);
			}
			else if (USCS_Node* SCSNode = Cast<USCS_Node>(Object))
			{
				if (SCSNode->ComponentTemplate != NULL)
				{
					if (SCSNode->VariableName != NAME_None)
					{
						Title = FString::Printf(*LOCTEXT("TemplateFor", "Template for %s").ToString(), *(SCSNode->VariableName.ToString()));
					}
					else 
					{
						Title = FString::Printf(*LOCTEXT("Name_Template", "%s Template").ToString(), *(SCSNode->ComponentTemplate->GetClass()->GetName()));
					}
				}
			}
			else if (UK2Node* K2Node = Cast<UK2Node>(Object))
			{
				// Edit the component template
				if (UActorComponent* Template = K2Node->GetTemplateFromNode())
				{
					Title = FString::Printf(*LOCTEXT("Name_Template", "%s Template").ToString(), *(Template->GetClass()->GetName()));
				}
			}

			if (Title.IsEmpty())
			{
				Title = UKismetSystemLibrary::GetDisplayName(Object);
			}
		}
		else if (SelectedObjects.Num() > 1)
		{
			UClass* BaseClass = NULL;

			for (auto ObjectWkPtrIt = SelectedObjects.CreateConstIterator(); ObjectWkPtrIt; ++ObjectWkPtrIt)
			{
				TWeakObjectPtr<UObject> ObjectWkPtr = *ObjectWkPtrIt;
				if (ObjectWkPtr.IsValid())
				{
					UObject* Object = ObjectWkPtr.Get();
					UClass* ObjClass = Object->GetClass();

					if (UEdGraphNode* Node = Cast<UEdGraphNode>(Object))
					{
						// Hide any specifics of node types; they're all ed graph nodes
						ObjClass = UEdGraphNode::StaticClass();
					}

					// Keep track of the class of objects selected
					if (BaseClass == NULL)
					{
						BaseClass = ObjClass;
					}
					while (!ObjClass->IsChildOf(BaseClass))
					{
						BaseClass = BaseClass->GetSuperClass();
					}
				}
			}

			if (BaseClass)
			{
				Title = FString::Printf( *LOCTEXT("MultipleObjectsSelected", "%d %ss selected").ToString(), SelectedObjects.Num(), *BaseClass->GetName() );
			}
		}
	}
	return Title;
}

void SKismetInspector::Construct(const FArguments& InArgs)
{
	bShowInspectorPropertyView = true;
	PublicViewState = ESlateCheckBoxState::Unchecked;

	Kismet2Ptr = InArgs._Kismet2;
	bShowPublicView = InArgs._ShowPublicViewControl;
	bShowTitleArea = InArgs._ShowTitleArea;
	TSharedPtr<FBlueprintEditor> Kismet2 = Kismet2Ptr.Pin();

	// Create a property view
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FNotifyHook* NotifyHook = NULL;
	if(InArgs._SetNotifyHook)
	{
		NotifyHook = Kismet2.Get();
	}
	FDetailsViewArgs DetailsViewArgs( /*bUpdateFromSelection=*/ false, /*bLockable=*/ false, /*bAllowSearch=*/ true, /*bObjectsUseNameArea=*/ true, /*bHideSelectionTip=*/ true, /*InNotifyHook=*/ NotifyHook, /*InSearchInitialKeyFocus=*/ false, /*InViewIdentifier=*/ InArgs._ViewIdentifier );
	DetailsViewArgs.bHideActorNameArea = InArgs._HideNameArea;

	PropertyView = EditModule.CreateDetailView( DetailsViewArgs );
		
	//@TODO: .IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() );
	PropertyView->SetIsPropertyVisibleDelegate( FIsPropertyVisible::CreateSP(this, &SKismetInspector::IsPropertyVisible) );
	PropertyView->SetIsPropertyEditingEnabledDelegate(InArgs._IsPropertyEditingEnabledDelegate);
	PropertyView->OnFinishedChangingProperties().Add( InArgs._OnFinishedChangingProperties );
	
	if (Kismet2->IsEditingSingleBlueprint())
	{
		FOnGetDetailCustomizationInstance LayoutDelegateDetails = FOnGetDetailCustomizationInstance::CreateStatic(&FBlueprintDelegateActionDetails::MakeInstance, TWeakPtr<SMyBlueprint>(Kismet2->GetMyBlueprintWidget()));
		PropertyView->RegisterInstancedCustomPropertyLayout(UMulticastDelegateProperty::StaticClass(), LayoutDelegateDetails);

		// Register function and variable details customization
		FOnGetDetailCustomizationInstance LayoutGraphDetails = FOnGetDetailCustomizationInstance::CreateStatic(&FBlueprintGraphActionDetails::MakeInstance, TWeakPtr<SMyBlueprint>(Kismet2->GetMyBlueprintWidget()));
		PropertyView->RegisterInstancedCustomPropertyLayout(UEdGraph::StaticClass(), LayoutGraphDetails);
		PropertyView->RegisterInstancedCustomPropertyLayout(UK2Node_EditablePinBase::StaticClass(), LayoutGraphDetails);
		PropertyView->RegisterInstancedCustomPropertyLayout(UK2Node_CallFunction::StaticClass(), LayoutGraphDetails);
	
		FOnGetDetailCustomizationInstance LayoutVariableDetails = FOnGetDetailCustomizationInstance::CreateStatic(&FBlueprintVarActionDetails::MakeInstance, TWeakPtr<SMyBlueprint>(Kismet2->GetMyBlueprintWidget()));
		PropertyView->RegisterInstancedCustomPropertyLayout(UProperty::StaticClass(), LayoutVariableDetails);
		PropertyView->RegisterInstancedCustomPropertyLayout(UK2Node_VariableGet::StaticClass(), LayoutVariableDetails);
		PropertyView->RegisterInstancedCustomPropertyLayout(UK2Node_VariableSet::StaticClass(), LayoutVariableDetails);

		FOnGetDetailCustomizationInstance LayoutLocalVariableDetails = FOnGetDetailCustomizationInstance::CreateStatic(&FBlueprintLocalVarActionDetails::MakeInstance, TWeakPtr<SMyBlueprint>(Kismet2->GetMyBlueprintWidget()));
		PropertyView->RegisterInstancedCustomPropertyLayout(UK2Node_LocalVariable::StaticClass(), LayoutLocalVariableDetails);

		FOnGetDetailCustomizationInstance LayoutOptionDetails = FOnGetDetailCustomizationInstance::CreateStatic(&FBlueprintGlobalOptionsDetails::MakeInstance, Kismet2Ptr);
		PropertyView->RegisterInstancedCustomPropertyLayout(UBlueprint::StaticClass(), LayoutOptionDetails);

		FOnGetDetailCustomizationInstance LayoutEnumDetails = FOnGetDetailCustomizationInstance::CreateStatic(&FEnumDetails::MakeInstance);
		PropertyView->RegisterInstancedCustomPropertyLayout(UUserDefinedEnum::StaticClass(), LayoutEnumDetails);

		FOnGetDetailCustomizationInstance LayoutFormatTextDetails = FOnGetDetailCustomizationInstance::CreateStatic(&FFormatTextDetails::MakeInstance);
		PropertyView->RegisterInstancedCustomPropertyLayout(UK2Node_FormatText::StaticClass(), LayoutFormatTextDetails);
	}

	// Create the border that all of the content will get stuffed into
	ChildSlot
	[
		SNew(STutorialWrapper)
		.Name(TEXT("BlueprintInspector"))
		.Content()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew( ContextualEditingBorderWidget, SBorder )
				.Padding(0)
				.BorderImage( FEditorStyle::GetBrush("NoBorder") )
			]
		]
	];

	// Update based on the current (empty) selection set
	TArray<UObject*> InitialSelectedObjects;
	FKismetSelectionInfo SelectionInfo;
	UpdateFromObjects(InitialSelectedObjects, SelectionInfo, SKismetInspector::FShowDetailsOptions(FString(), true));
}

void SKismetInspector::EnableComponentDetailsCustomization(bool bEnable)
{
	if (bEnable)
	{
		FOnGetDetailCustomizationInstance LayoutComponentDetails = FOnGetDetailCustomizationInstance::CreateStatic(&FBlueprintComponentDetails::MakeInstance, Kismet2Ptr);
		PropertyView->RegisterInstancedCustomPropertyLayout(UActorComponent::StaticClass(), LayoutComponentDetails);
	}
	else
	{
		PropertyView->UnregisterInstancedCustomPropertyLayout(UActorComponent::StaticClass());
	}
}

/** Update the inspector window to show information on the supplied object */
void SKismetInspector::ShowDetailsForSingleObject(UObject* Object, const FShowDetailsOptions& Options)
{
	TArray<UObject*> PropertyObjects;
	FKismetSelectionInfo SelectionInfo;

	if (Object != NULL)
	{
		PropertyObjects.Add(Object);
		SelectionInfo.ObjectsForPropertyEditing.Add(Object);
	}

	UpdateFromObjects(PropertyObjects, SelectionInfo, Options);
}

void SKismetInspector::ShowDetailsForObjects(const TArray<UObject*>& PropertyObjects, const FShowDetailsOptions& Options)
{
	FKismetSelectionInfo SelectionInfo;
	UpdateFromObjects(PropertyObjects, SelectionInfo, Options);
}

void SKismetInspector::UpdateFromObjects(const TArray<UObject*>& PropertyObjects, struct FKismetSelectionInfo& SelectionInfo, const FShowDetailsOptions& Options)
{
	if (!Options.bForceRefresh)
	{
		// Early out if the PropertyObjects and the SelectedObjects are the same
		bool bEquivalentSets = (PropertyObjects.Num() == SelectedObjects.Num());
		if (bEquivalentSets)
		{
			// Verify the elements of the sets are equivalent
			for (int32 i = 0; i < PropertyObjects.Num(); i++)
			{
				if (PropertyObjects[i] != SelectedObjects[i].Get())
				{
					bEquivalentSets = false;
					break;
				}
			}
		}

		if (bEquivalentSets)
		{
			return;
		}
	}

	// Proceed to update
	SelectedObjects.Empty();

	for (auto ObjectIt = PropertyObjects.CreateConstIterator(); ObjectIt; ++ObjectIt)
	{
		if (UObject* Object = *ObjectIt)
		{
			if (!Object->IsValidLowLevel())
			{
				ensureMsg(false, TEXT("Object in KismetInspector is invalid, see TTP 281915"));
				continue;
			}

			SelectedObjects.Add(Object);

			if (USCS_Node* SCSNode = Cast<USCS_Node>(Object))
			{
				// Edit the component template
				UActorComponent* NodeComponent = SCSNode->ComponentTemplate;
				if (NodeComponent != NULL)
				{
					SelectionInfo.ObjectsForPropertyEditing.Add(NodeComponent);
					SelectionInfo.EditableComponentTemplates.Add(NodeComponent);
				}
			}
			else if (UK2Node* K2Node = Cast<UK2Node>(Object))
			{
				// Edit the component template if it exists
				if (UActorComponent* Template = K2Node->GetTemplateFromNode())
				{
					SelectionInfo.ObjectsForPropertyEditing.Add(Template);
					SelectionInfo.EditableComponentTemplates.Add(Template);
				}

				// See if we should edit properties of the node
				if (K2Node->ShouldShowNodeProperties())
				{
					SelectionInfo.ObjectsForPropertyEditing.Add(Object);
				}
			}
			else if (UActorComponent* ActorComponent = Cast<UActorComponent>(Object))
			{
				AActor* Owner = ActorComponent->GetOwner();
				if(Owner != NULL && Owner->HasAnyFlags(RF_ClassDefaultObject))
				{
					// We're editing a component that's owned by a CDO, so set the CDO to the property editor (so that propagation works) and then filter to just the component property that we want to edit
					SelectionInfo.ObjectsForPropertyEditing.AddUnique(Owner);
					SelectionInfo.EditableComponentTemplates.Add(ActorComponent);
				}
				else
				{
					// We're editing a component that exists outside of a CDO, so just edit the component instance directly
					SelectionInfo.ObjectsForPropertyEditing.AddUnique(ActorComponent);
				}
			}
			else
			{
				// Editing any UObject*
				SelectionInfo.ObjectsForPropertyEditing.AddUnique(Object);
			}
		}
	}

	// By default, no property filtering
	SelectedObjectProperties.Empty();

	// Add to the property filter list for any editable component templates
	if(SelectionInfo.EditableComponentTemplates.Num())
	{
		for(auto CompIt = SelectionInfo.EditableComponentTemplates.CreateIterator(); CompIt; ++CompIt)
		{
			UActorComponent* EditableComponentTemplate = *CompIt;
			check(EditableComponentTemplate != NULL);

			// Add all properties belonging to the component template class
			for(TFieldIterator<UProperty> PropIt(EditableComponentTemplate->GetClass()); PropIt; ++PropIt)
			{
				UProperty* Property = *PropIt;
				check(Property != NULL);

				SelectedObjectProperties.AddUnique(Property);
			}

			// Attempt to locate a matching property for the current component template
			for(auto ObjIt = SelectionInfo.ObjectsForPropertyEditing.CreateIterator(); ObjIt; ++ObjIt)
			{
				UObject* Object = *ObjIt;
				check(Object != NULL);

				if(Object != EditableComponentTemplate)
				{
					for(TFieldIterator<UObjectProperty> ObjPropIt(Object->GetClass()); ObjPropIt; ++ObjPropIt)
					{
						UObjectProperty* ObjectProperty = *ObjPropIt;
						check(ObjectProperty != NULL);

						// If the property value matches the current component template, add it as a selected property for filtering
						if(EditableComponentTemplate == ObjectProperty->GetObjectPropertyValue_InContainer(Object))
						{
							SelectedObjectProperties.AddUnique(ObjectProperty);
						}
					}
				}
			}
		}
	}

	PropertyViewTitle = Options.ForcedTitle;

	// Update our context-sensitive editing widget
	ContextualEditingBorderWidget->SetContent( MakeContextualEditingWidget(SelectionInfo, Options) );
}

bool SKismetInspector::IsPropertyVisible(UProperty const * const InProperty) const
{
	check(InProperty);

	// If we are in 'instance preview' - hide anything marked 'disabled edit on instance'
	if ((ESlateCheckBoxState::Checked == PublicViewState) && InProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance))
	{
		return false;
	}

	bool bEditOnTemplateDisabled = InProperty->HasAnyPropertyFlags(CPF_DisableEditOnTemplate);

	if(const UClass* OwningClass = Cast<UClass>(InProperty->GetOuter()))
	{
		const UBlueprint* BP = Kismet2Ptr.IsValid() ? Kismet2Ptr.Pin()->GetBlueprintObj() : NULL;
		const bool VariableAddedInCurentBlueprint = (OwningClass->ClassGeneratedBy == BP);

		// If we did not add this var, hide it!
		if(!VariableAddedInCurentBlueprint)
		{
			if (bEditOnTemplateDisabled || InProperty->GetBoolMetaData(FBlueprintMetadata::MD_Private))
			{
				return false;
			}
		}
	}

	// figure out if this Blueprint variable is an Actor variable
	const UArrayProperty* ArrayProperty = Cast<const UArrayProperty>(InProperty);
	const UProperty* TestProperty = ArrayProperty ? ArrayProperty->Inner : InProperty;
	const UObjectPropertyBase* ObjectProperty = Cast<const UObjectPropertyBase>(TestProperty);
	bool bIsActorProperty = (ObjectProperty != NULL && ObjectProperty->PropertyClass->IsChildOf(AActor::StaticClass()));

	if (bEditOnTemplateDisabled && bIsActorProperty)
	{
		// Actor variables can't have default values (because Blueprint templates are library elements that can 
		// bridge multiple levels and different levels might not have the actor that the default is referencing).
		return false;
	}

	// Filter down to selected properties only if set
	for(auto PropIt = SelectedObjectProperties.CreateConstIterator(); PropIt; ++PropIt)
	{
		if(PropIt->IsValid() && PropIt->Get() == InProperty)
		{
			return true;
		}
	}

	return !SelectedObjectProperties.Num();
}

void SKismetInspector::SetPropertyWindowContents(TArray<UObject*> Objects)
{
	if (FSlateApplication::IsInitialized())
	{
		check(PropertyView.IsValid());
		PropertyView->SetObjects(Objects);
	}
}

EVisibility SKismetInspector::GetPropertyViewVisibility() const
{
	return bShowInspectorPropertyView? EVisibility::Visible : EVisibility::Collapsed;
}

ESlateCheckBoxState::Type SKismetInspector::GetPublicViewCheckboxState() const
{
	return PublicViewState;
}

void SKismetInspector::SetPublicViewCheckboxState( ESlateCheckBoxState::Type InIsChecked )
{
	PublicViewState = InIsChecked;

	//reset the details view
	TArray<UObject*> Objs;
	for(auto It(SelectedObjects.CreateIterator());It;++It)
	{
		Objs.Add(It->Get());
	}
	SelectedObjects.Empty();
	
	if(Objs.Num() > 1)
	{
		ShowDetailsForObjects(Objs);
	}
	else if(Objs.Num() == 1)
	{
		ShowDetailsForSingleObject(Objs[0], FShowDetailsOptions(PropertyViewTitle));
	}
	
	Kismet2Ptr.Pin()->StartEditingDefaults();
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
