// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintEditorPrivatePCH.h"
#include "BlueprintUtilities.h"
#include "ScopedTransaction.h"
#include "GraphEditor.h"

#include "BlueprintEditor.h"

#include "Editor/PropertyEditor/Public/PropertyEditing.h"

#include "SKismetInspector.h"
#include "SSCSEditor.h"
#include "SMyBlueprint.h"
#include "GraphEditorDragDropAction.h"
#include "BPFunctionDragDropAction.h"
#include "BPVariableDragDropAction.h"
#include "SBlueprintPalette.h"
#include "SGraphActionMenu.h"
#include "SPinTypeSelector.h"
#include "Kismet2NameValidators.h"

#include "ComponentAssetBroker.h"
#include "PropertyCustomizationHelpers.h"

#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"

#include "BlueprintDetailsCustomization.h"
#include "ObjectEditorUtils.h"

#include "Editor/SceneOutliner/Private/SSocketChooser.h"
#include "PropertyEditorModule.h"

#include "IDocumentation.h"

#define LOCTEXT_NAMESPACE "BlueprintDetailsCustomization"

// UProperty Detail Customization
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FBlueprintVarActionDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	UProperty* VariableProperty = SelectionAsProperty();

	if(!VariableProperty)
	{
		return;
	}

	FName VarName =	GetVariableName();
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	
	IDetailCategoryBuilder& Category = DetailLayout.EditCategory("Variable", LOCTEXT("VariableDetailsCategory", "Variable").ToString());
	const FSlateFontInfo DetailFontInfo = IDetailLayoutBuilder::GetDetailFont();
	
	const FString DocLink = TEXT("Shared/Editors/BlueprintEditor/VariableDetails");

	TSharedPtr<SToolTip> VarNameTooltip = IDocumentation::Get()->CreateToolTip(LOCTEXT("VarNameTooltip", "The name of the variable."), NULL, DocLink, TEXT("VariableName"));

	Category.AddCustomRow( TEXT("Variable Name") )
	.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("BlueprintVarActionDetails_VariableNameLabel", "Variable Name").ToString())
		.ToolTip(VarNameTooltip)
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	[
		SAssignNew(VarNameEditableTextBox, SEditableTextBox)
		.Text(this, &FBlueprintVarActionDetails::OnGetVarName)
		.ToolTip(VarNameTooltip)
		.OnTextChanged(this, &FBlueprintVarActionDetails::OnVarNameChanged)
		.OnTextCommitted(this, &FBlueprintVarActionDetails::OnVarNameCommitted)
		.IsReadOnly(this, &FBlueprintVarActionDetails::GetVariableNameChangeEnabled)
		.Font(IDetailLayoutBuilder::GetDetailFont())
	];

	TSharedPtr<SToolTip> VarTypeTooltip = IDocumentation::Get()->CreateToolTip(LOCTEXT("VarTypeTooltip", "The type of the variable."), NULL, DocLink, TEXT("VariableType"));

	Category.AddCustomRow( TEXT("Variable Type") )
	.NameContent()
	[
		SNew(STextBlock)
		.Text( LOCTEXT("VariableTypeLabel", "Variable Type") )
		.ToolTip(VarTypeTooltip)
		.Font(DetailFontInfo)
	]
	.ValueContent()
	[
		SNew(SPinTypeSelector, FGetPinTypeTree::CreateUObject(Schema, &UEdGraphSchema_K2::GetVariableTypeTree))
		.TargetPinType(this, &FBlueprintVarActionDetails::OnGetVarType)
		.OnPinTypeChanged(this, &FBlueprintVarActionDetails::OnVarTypeChanged)
		.IsEnabled(this, &FBlueprintVarActionDetails::GetVariableTypeChangeEnabled)
		.Schema(Schema)
		.bAllowExec(false)
		.Font( DetailFontInfo )
		.ToolTip(VarTypeTooltip)
	];

	TSharedPtr<SToolTip> EditableTooltip = IDocumentation::Get()->CreateToolTip(LOCTEXT("VarEditableTooltip", "Whether this variable is publically editable on instances of this Blueprint."), NULL, DocLink, TEXT("Editable"));

	Category.AddCustomRow( TEXT("Editable") )
	.Visibility(TAttribute<EVisibility>(this, &FBlueprintVarActionDetails::ShowEditableCheckboxVisibilty))
	.NameContent()
	[
		SNew(STextBlock)
		.Text( LOCTEXT("IsVariableEditableLabel", "Editable") )
		.ToolTip(EditableTooltip)
		.Font( IDetailLayoutBuilder::GetDetailFont() )
	]
	.ValueContent()
	[
		SNew(SCheckBox)
		.IsChecked( this, &FBlueprintVarActionDetails::OnEditableCheckboxState )
		.OnCheckStateChanged( this, &FBlueprintVarActionDetails::OnEditableChanged )
		.ToolTip(EditableTooltip)
	];

	TSharedPtr<SToolTip> ToolTipTooltip = IDocumentation::Get()->CreateToolTip(LOCTEXT("VarToolTipTooltip", "Extra information about this variable, shown when cursor is over it."), NULL, DocLink, TEXT("Tooltip"));

	Category.AddCustomRow( TEXT("Tooltip") )
	.Visibility(TAttribute<EVisibility>(this, &FBlueprintVarActionDetails::IsTooltipEditVisible))
	.NameContent()
	[
		SNew(STextBlock)
		.Text( LOCTEXT("IsVariableToolTipLabel", "Tooltip") )
		.ToolTip(ToolTipTooltip)
		.Font( DetailFontInfo )
	]
	.ValueContent()
	[
		SNew(SEditableTextBox)
		.Text( this, &FBlueprintVarActionDetails::OnGetTooltipText )
		.ToolTip(ToolTipTooltip)
		.OnTextCommitted( this, &FBlueprintVarActionDetails::OnTooltipTextCommitted, VarName )
		.Font( DetailFontInfo )
	];

	TSharedPtr<SToolTip> Widget3DTooltip = IDocumentation::Get()->CreateToolTip(LOCTEXT("VariableWidget3D_Tooltip", "When true, allows the user to tweak the vector variable by using a 3D transform widget in the viewport (usable when varible is public/enabled)."), NULL, DocLink, TEXT("Widget3D"));

	Category.AddCustomRow( LOCTEXT("VariableWidget3D_Prompt", "Show 3D Widget").ToString() )
	.Visibility(TAttribute<EVisibility>(this, &FBlueprintVarActionDetails::Show3DWidgetVisibility))
	.NameContent()
	[
		SNew(STextBlock)
		.ToolTip(Widget3DTooltip)
		.Text(LOCTEXT("VariableWidget3D_Prompt", "Show 3D Widget"))
		.Font( DetailFontInfo )
		.IsEnabled(Is3DWidgetEnabled())
	]
	.ValueContent()
	[
		SNew(SCheckBox)
		.IsChecked( this, &FBlueprintVarActionDetails::OnCreateWidgetCheckboxState )
		.OnCheckStateChanged( this, &FBlueprintVarActionDetails::OnCreateWidgetChanged )
		.IsEnabled(Is3DWidgetEnabled())
		.ToolTip(Widget3DTooltip)
	];

	TSharedPtr<SToolTip> ExposeOnSpawnTooltip = IDocumentation::Get()->CreateToolTip(LOCTEXT("VariableExposeToSpawn_Tooltip", "Should this variable be exposed as a pin when spawning this Blueprint?"), NULL, DocLink, TEXT("ExposeOnSpawn"));

	Category.AddCustomRow( TEXT("Expose on Spawn") )
	.Visibility(TAttribute<EVisibility>(this, &FBlueprintVarActionDetails::ExposeOnSpawnVisibility))
	.NameContent()
	[
		SNew(STextBlock)
		.ToolTip(ExposeOnSpawnTooltip)
		.Text( LOCTEXT("VariableExposeToSpawnLabel", "Expose on Spawn") )
		.Font( DetailFontInfo )
	]
	.ValueContent()
	[
		SNew(SCheckBox)
		.IsChecked( this, &FBlueprintVarActionDetails::OnGetExposedToSpawnCheckboxState )
		.OnCheckStateChanged( this, &FBlueprintVarActionDetails::OnExposedToSpawnChanged )
		.ToolTip(ExposeOnSpawnTooltip)
	];

	TSharedPtr<SToolTip> PrivateTooltip = IDocumentation::Get()->CreateToolTip(LOCTEXT("VariablePrivate_Tooltip", "Should this variable be private (derived blueprints cannot modify it)?"), NULL, DocLink, TEXT("Private"));

	Category.AddCustomRow(TEXT(""))
	.Visibility(TAttribute<EVisibility>(this, &FBlueprintVarActionDetails::ExposePrivateVisibility))
	.NameContent()
	[
		SNew(STextBlock)
		.ToolTip(PrivateTooltip)
		.Text( LOCTEXT("VariablePrivate", "Private") )
		.Font( DetailFontInfo )
	]
	.ValueContent()
	[
		SNew(SCheckBox)
		.IsChecked( this, &FBlueprintVarActionDetails::OnGetPrivateCheckboxState )
		.OnCheckStateChanged( this, &FBlueprintVarActionDetails::OnPrivateChanged )
		.ToolTip(PrivateTooltip)
	];

	TSharedPtr<SToolTip> ExposeToMatineeTooltip = IDocumentation::Get()->CreateToolTip(LOCTEXT("VariableExposeToMatinee_Tooltip", "Should this variable be exposed for Matinee to modify?"), NULL, DocLink, TEXT("ExposeToMatinee"));

	Category.AddCustomRow( TEXT("Expose to Matinee") )
	.Visibility(TAttribute<EVisibility>(this, &FBlueprintVarActionDetails::ExposeToMatineeVisibility))
	.NameContent()
	[
		SNew(STextBlock)
		.ToolTip(ExposeToMatineeTooltip)
		.Text( LOCTEXT("VariableExposeToMatinee", "Expose to Matinee") )
		.Font( DetailFontInfo )
	]
	.ValueContent()
	[
		SNew(SCheckBox)
		.IsChecked( this, &FBlueprintVarActionDetails::OnGetExposedToMatineeCheckboxState )
		.OnCheckStateChanged( this, &FBlueprintVarActionDetails::OnExposedToMatineeChanged )
		.ToolTip(ExposeToMatineeTooltip)
	];

	PopulateCategories(MyBlueprint.Pin().Get(), CategorySource);
	TSharedPtr<SComboButton> NewComboButton;
	TSharedPtr<SListView<TSharedPtr<FString>>> NewListView;

	TSharedPtr<SToolTip> CategoryTooltip = IDocumentation::Get()->CreateToolTip(LOCTEXT("EditCategoryName_Tooltip", "The category of the variable; editing this will place the variable into another category or create a new one"), NULL, DocLink, TEXT("Category"));

	Category.AddCustomRow( TEXT("Category") )
	.NameContent()
	[
		SNew(STextBlock)
		.Text( LOCTEXT("CategoryLabel", "Category") )
		.ToolTip(CategoryTooltip)
		.Font( DetailFontInfo )
	]
	.ValueContent()
	[
		SAssignNew(NewComboButton, SComboButton)
		.ContentPadding(FMargin(0,0,5,0))
		.IsEnabled(this, &FBlueprintVarActionDetails::GetVariableCategoryChangeEnabled)
		.ToolTip(CategoryTooltip)
		.ButtonContent()
		[
			SNew(SBorder)
			.BorderImage( FEditorStyle::GetBrush("NoBorder") )
			.Padding(FMargin(0, 0, 5, 0))
			[
				SNew(SEditableTextBox)
					.Text(this, &FBlueprintVarActionDetails::OnGetCategoryText)
					.OnTextCommitted(this, &FBlueprintVarActionDetails::OnCategoryTextCommitted, VarName )
					.ToolTip(CategoryTooltip)
					.SelectAllTextWhenFocused(true)
					.RevertTextOnEscape(true)
					.Font( DetailFontInfo )
			]
		]
		.MenuContent()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.MaxHeight(400.0f)
			[
				SAssignNew(NewListView, SListView<TSharedPtr<FString>>)
					.ListItemsSource(&CategorySource)
					.OnGenerateRow(this, &FBlueprintVarActionDetails::MakeCategoryViewWidget)
					.OnSelectionChanged(this, &FBlueprintVarActionDetails::OnCategorySelectionChanged)
			]
		]
	];

	CategoryComboButton = NewComboButton;
	CategoryListView = NewListView;

	TSharedPtr<SToolTip> SliderRangeTooltip = IDocumentation::Get()->CreateToolTip(LOCTEXT("SliderRange_Tooltip", "Allows setting the minimum and maximum values for the UI slider for this variable."), NULL, DocLink, TEXT("SliderRange"));

	Category.AddCustomRow( TEXT("Slider Range") )
	.Visibility(TAttribute<EVisibility>(this, &FBlueprintVarActionDetails::SliderVisibility))
	.NameContent()
	[
		SNew(STextBlock)
		.Text( LOCTEXT("SliderRangeLabel", "Slider Range") )
		.ToolTip(SliderRangeTooltip)
		.Font( DetailFontInfo )
	]
	.ValueContent()
	[
		SNew(SHorizontalBox)
		.ToolTip(SliderRangeTooltip)
		+SHorizontalBox::Slot()
		.FillWidth(1)
		[
			SNew(SEditableTextBox)
			.Text( this, &FBlueprintVarActionDetails::OnGetSliderMinValue )
			.OnTextCommitted( this, &FBlueprintVarActionDetails::OnSliderMinValueChanged )
			.Font( DetailFontInfo )
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew( STextBlock )
			.Text( LOCTEXT("Min .. Max Separator", " .. ") )
			.Font(DetailFontInfo)
		]
		+SHorizontalBox::Slot()
		.FillWidth(1)
		[
			SNew(SEditableTextBox)
			.Text( this, &FBlueprintVarActionDetails::OnGetSliderMaxValue )
			.OnTextCommitted(this, &FBlueprintVarActionDetails::OnSliderMaxValueChanged)
			.Font( DetailFontInfo )
		]
	];

	ReplicationOptions.Empty();
	ReplicationOptions.Add(MakeShareable(new FString("None")));
	ReplicationOptions.Add(MakeShareable(new FString("Replicated")));
	ReplicationOptions.Add(MakeShareable(new FString("RepNotify")));

	TSharedPtr<SToolTip> ReplicationTooltip = IDocumentation::Get()->CreateToolTip(LOCTEXT("VariableReplicate_Tooltip", "Should this Variable be replicated over the network?"), NULL, DocLink, TEXT("Replication"));

	Category.AddCustomRow( TEXT("Replication") )
	.Visibility(TAttribute<EVisibility>(this, &FBlueprintVarActionDetails::ReplicationVisibility))
	.NameContent()
	[
		SNew(STextBlock)
		.ToolTip(ReplicationTooltip)
		.Text( LOCTEXT("VariableReplicationLabel", "Replication") )
		.Font( DetailFontInfo )
	]
	.ValueContent()
	[
		SNew(STextComboBox)
		.OptionsSource( &ReplicationOptions )
		.InitiallySelectedItem(GetVariableReplicationType())
		.OnSelectionChanged( this, &FBlueprintVarActionDetails::OnChangeReplication )
		.ToolTip(ReplicationTooltip)
	];

		// Handle event generation
	if(FBlueprintEditorUtils::DoesSupportEventGraphs(GetBlueprintObj()))
	{
		if (UObjectProperty* ComponentProperty = Cast<UObjectProperty>(VariableProperty))
		{
			// Check for Ed Graph vars that can generate events
			if (ComponentProperty->PropertyClass && ComponentProperty->PropertyClass->IsChildOf(UActorComponent::StaticClass()))
			{
				// Add Events Section property class that has valid events
				if( FBlueprintEditor::CanClassGenerateEvents( ComponentProperty->PropertyClass ))
				{
					IDetailCategoryBuilder& EventCategory = DetailLayout.EditCategory("Component", LOCTEXT("ComponentDetailsCategory", "Events").ToString());

					FText NodeName = FText::FromName( VarName );
					const FText AddEventLabel = FText::Format( LOCTEXT( "ScriptingEvents_AddEvent", "Add Event For {0}" ), NodeName );
					const FText AddEventToolTip = FText::Format( LOCTEXT( "ScriptingEvents_AddOrView", "Adds or views events for {0} in the Component Blueprint" ), NodeName );

					TSharedPtr<SToolTip> EventsTooltip = IDocumentation::Get()->CreateToolTip(AddEventToolTip, NULL, DocLink, TEXT("Events"));

					EventCategory.AddCustomRow( LOCTEXT("AddEventHeader", "Add Event").ToString())
						.NameContent()
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.Padding( FMargin( 2.0f, 0, 0, 0 ) )
							.VAlign( VAlign_Center )
							.AutoWidth()
							[
								SNew( STextBlock )
								.Text( LOCTEXT("ScriptingEvents_Label", "Add Event"))
								.ToolTip(EventsTooltip)
								.Font( IDetailLayoutBuilder::GetDetailFont() )
							]
						]
					.ValueContent()
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.FillWidth( 1 )
							.VAlign( VAlign_Center )
							[
								SNew( SComboButton )
								.HAlign(HAlign_Fill)
								.VAlign(VAlign_Center)
								.OnGetMenuContent( this, &FBlueprintVarActionDetails::BuildEventsMenuForVariable )
								.ButtonContent()
								[
									SNew( STextBlock )
									.Text( AddEventLabel ) 
									.ToolTip( EventsTooltip )
									.Font( IDetailLayoutBuilder::GetDetailFont() )
								]
							]
						];
				}
			}
		}
	}

	// Add in default value editing for properties that can be edited
	UBlueprint* Blueprint = GetBlueprintObj();
	if ((Blueprint != NULL) && (Blueprint->GeneratedClass != NULL))
	{
		if (VariableProperty != NULL)
		{
			const UProperty* OryginalProperty = FindField<UProperty>(Blueprint->GeneratedClass, VariableProperty->GetFName());
			if (OryginalProperty == NULL)
			{
				// Prevent editing the default value of a skeleton property
				VariableProperty = NULL;
			}
			else  if (auto StructProperty = Cast<const UStructProperty>(OryginalProperty))
			{
				// Prevent editing the default value of a stale struct
				auto BGStruct = Cast<const UBlueprintGeneratedStruct>(StructProperty->Struct);
				if (BGStruct && (EBlueprintStructureStatus::BSS_UpToDate != BGStruct->Status))
				{
					VariableProperty = NULL;
				}
			}
		}

		// Find the class containing the variable
		UClass* VariableClass = (VariableProperty != NULL) ? VariableProperty->GetTypedOuter<UClass>() : NULL;

		FText ErrorMessage;
		const FString TooltipStr = LOCTEXT("DefaultValue_Tooltip", "Edit the default value of this variable.").ToString();
		IDetailCategoryBuilder& DefaultValueCategory = DetailLayout.EditCategory(TEXT("DefaultValueCategory"), LOCTEXT("DefaultValueCategoryHeading", "Default Value").ToString());

		if (VariableProperty == NULL)
		{
			if (Blueprint->Status != BS_UpToDate)
			{
				ErrorMessage = LOCTEXT("VariableMissing_DirtyBlueprint", "Please compile the blueprint");
			}
			else
			{
				ErrorMessage = LOCTEXT("VariableMissing_CleanBlueprint", "Failed to find variable property");
			}
		}
		else if (VariableProperty->HasAnyPropertyFlags(CPF_DisableEditOnTemplate))
		{
			if (VariableClass->ClassGeneratedBy != Blueprint)
			{
				ErrorMessage = LOCTEXT("VariableHasDisableEditOnTemplate", "Editing this value is not allowed");
			}
			else
			{
				// determine if the variable is an object type
				const UArrayProperty* ArrayProperty = Cast<const UArrayProperty>(VariableProperty);
				const UProperty* TestProperty = ArrayProperty ? ArrayProperty->Inner : VariableProperty;
				const UObjectPropertyBase* ObjectProperty = Cast<const UObjectPropertyBase>(TestProperty);

				// if this is variable is an Actor
				if ((ObjectProperty != NULL) && ObjectProperty->PropertyClass->IsChildOf(AActor::StaticClass()))
				{
					// Actor variables can't have default values (because Blueprint templates are library elements that can 
					// bridge multiple levels and different levels might not have the actor that the default is referencing).
					ErrorMessage = LOCTEXT("VariableHasDisableEditOnTemplate", "Editing this value is not allowed");
				}
			}
		}

		// Show the error message if something went wrong
		if (!ErrorMessage.IsEmpty())
		{
			DefaultValueCategory.AddCustomRow( TEXT("Error") )
			[
				SNew(STextBlock)
				.ToolTipText(ErrorMessage.ToString())
				.Text(ErrorMessage.ToString())
				.Font(DetailFontInfo)
			];
		}
		else 
		{
			// Things are in order, show the property and allow it to be edited
			TArray<UObject*> ObjectList;
			ObjectList.Add(Blueprint->GeneratedClass->GetDefaultObject());
			IDetailPropertyRow* Row = DefaultValueCategory.AddExternalProperty(ObjectList, VariableProperty->GetFName());
		}

		TSharedPtr<SToolTip> TransientTooltip = IDocumentation::Get()->CreateToolTip(LOCTEXT("VariableTransient_Tooltip", "Should this variable not serialize and be zero-filled at load?"), NULL, DocLink, TEXT("Transient"));

		Category.AddCustomRow(TEXT(""), true)
			.Visibility(TAttribute<EVisibility>(this, &FBlueprintVarActionDetails::GetTransientVisibility))
			.NameContent()
		[
			SNew(STextBlock)
			.ToolTip(TransientTooltip)
			.Text( LOCTEXT("VariableTransient", "Transient") )
			.Font( DetailFontInfo )
		]
		.ValueContent()
		[
			SNew(SCheckBox)
			.IsChecked( this, &FBlueprintVarActionDetails::OnGetTransientCheckboxState )
			.OnCheckStateChanged( this, &FBlueprintVarActionDetails::OnTransientChanged )
			.ToolTip(TransientTooltip)
		];

		TSharedPtr<SToolTip> SaveGameTooltip = IDocumentation::Get()->CreateToolTip(LOCTEXT("VariableSaveGame_Tooltip", "Should this variable be serialized for saved games?"), NULL, DocLink, TEXT("SaveGame"));

		Category.AddCustomRow(TEXT(""), true)
		.Visibility(TAttribute<EVisibility>(this, &FBlueprintVarActionDetails::GetSaveGameVisibility))
		.NameContent()
		[
			SNew(STextBlock)
			.ToolTip(SaveGameTooltip)
			.Text( LOCTEXT("VariableSaveGame", "SaveGame") )
			.Font( DetailFontInfo )
		]
		.ValueContent()
		[
			SNew(SCheckBox)
			.IsChecked( this, &FBlueprintVarActionDetails::OnGetSaveGameCheckboxState )
			.OnCheckStateChanged( this, &FBlueprintVarActionDetails::OnSaveGameChanged )
			.ToolTip(SaveGameTooltip)
		];

		TSharedPtr<SToolTip> PropertyFlagsTooltip = IDocumentation::Get()->CreateToolTip(LOCTEXT("DefinedPropertyFlags_Tooltip", "List of defined flags for this property"), NULL, DocLink, TEXT("PropertyFlags"));

		Category.AddCustomRow(TEXT(""), true)
		.WholeRowWidget
		[
			SNew(STextBlock)
			.ToolTip(PropertyFlagsTooltip)
			.Text( LOCTEXT("DefinedPropertyFlags", "Defined Property Flags") )
			.Font( IDetailLayoutBuilder::GetDetailFontBold() )
		];

		Category.AddCustomRow(TEXT(""), true)
		.WholeRowWidget
		[
			SAssignNew(PropertyFlagWidget, SListView< TSharedPtr< FString > >)
				.OnGenerateRow(this, &FBlueprintVarActionDetails::OnGenerateWidgetForPropertyList)
				.ListItemsSource(&PropertyFlags)
				.SelectionMode(ESelectionMode::None)
				.ScrollbarVisibility(EVisibility::Collapsed)
				.ToolTip(PropertyFlagsTooltip)
		];

		RefreshPropertyFlags();
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FBlueprintVarActionDetails::RefreshPropertyFlags()
{
	UProperty* Property = SelectionAsProperty();

	PropertyFlags.Empty();
	for( const TCHAR* PropertyFlag : ParsePropertyFlags(Property->PropertyFlags) )
	{
		PropertyFlags.Add(MakeShareable<FString>(new FString(PropertyFlag)));
	}

	PropertyFlagWidget.Pin()->RequestListRefresh();
}

TSharedRef<ITableRow> FBlueprintVarActionDetails::OnGenerateWidgetForPropertyList( TSharedPtr< FString > Item, const TSharedRef<STableViewBase>& OwnerTable )
{
	return SNew(STableRow< TSharedPtr< FString > >, OwnerTable)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			[
				SNew(STextBlock)
					.Text(FText::FromString(*Item.Get()))
					.ToolTipText(FText::FromString(*Item.Get()))
					.Font( IDetailLayoutBuilder::GetDetailFont() )
			]

			+SHorizontalBox::Slot()
				.AutoWidth()
			[
				SNew(SCheckBox)
					.IsChecked(true)
					.IsEnabled(false)
			]
		];
}

bool FBlueprintVarActionDetails::IsAComponentVariable(UProperty* VariableProperty) const
{
	UObjectProperty* VariableObjProp = VariableProperty ? Cast<UObjectProperty>(VariableProperty) : NULL;
	return (VariableObjProp != NULL) && (VariableObjProp->PropertyClass != NULL) && (VariableObjProp->PropertyClass->IsChildOf(UActorComponent::StaticClass()));
}

bool FBlueprintVarActionDetails::IsABlueprintVariable(UProperty* VariableProperty) const
{
	UClass* VarSourceClass = VariableProperty ? CastChecked<UClass>(VariableProperty->GetOuter()) : NULL;
	return (VarSourceClass->ClassGeneratedBy != NULL);
}

bool FBlueprintVarActionDetails::GetVariableNameChangeEnabled() const
{
	bool bIsReadOnly = true;

	UBlueprint* Blueprint = GetBlueprintObj();
	check(Blueprint != NULL);

	UProperty* VariableProperty = SelectionAsProperty();
	if(VariableProperty != NULL)
	{
		if(FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, GetVariableName()) != INDEX_NONE)
		{
			bIsReadOnly = false;
		}
		else if(Blueprint->FindTimelineTemplateByVariableName(GetVariableName()))
		{
			bIsReadOnly = false;
		}
		else if(IsAComponentVariable(VariableProperty) && Blueprint->SimpleConstructionScript != NULL)
		{
			TArray<USCS_Node*> Nodes = Blueprint->SimpleConstructionScript->GetAllNodes();
			for (TArray<USCS_Node*>::TConstIterator NodeIt(Nodes); NodeIt; ++NodeIt)
			{
				USCS_Node* Node = *NodeIt;
				if (Node->VariableName == GetVariableName())
				{
					bIsReadOnly = !Node->IsValidVariableNameString(Node->VariableName.ToString());
					break;
				}
			}
		}
	}

	return bIsReadOnly;
}

FText FBlueprintVarActionDetails::OnGetVarName() const
{
	return FText::FromName(GetVariableName());
}

void FBlueprintVarActionDetails::OnVarNameChanged(const FText& InNewText)
{
	bIsVarNameInvalid = true;

	UBlueprint* Blueprint = GetBlueprintObj();
	check(Blueprint != NULL);

	UProperty* VariableProperty = SelectionAsProperty();
	if(VariableProperty && IsAComponentVariable(VariableProperty) && Blueprint->SimpleConstructionScript != NULL)
	{
		TArray<USCS_Node*> Nodes = Blueprint->SimpleConstructionScript->GetAllNodes();
		for (TArray<USCS_Node*>::TConstIterator NodeIt(Nodes); NodeIt; ++NodeIt)
		{
			USCS_Node* Node = *NodeIt;
			if (Node->VariableName == GetVariableName() && !Node->IsValidVariableNameString(InNewText.ToString()))
			{
				VarNameEditableTextBox->SetError(LOCTEXT("ComponentVariableRenameFailed_NotValid", "This name is reserved for engine use."));
				return;
			}
		}
	}

	TSharedPtr<INameValidatorInterface> NameValidator = MakeShareable(new FKismetNameValidator(Blueprint, GetVariableName()));

	EValidatorResult ValidatorResult = NameValidator->IsValid(InNewText.ToString());
	if(ValidatorResult == EValidatorResult::AlreadyInUse)
	{
		VarNameEditableTextBox->SetError(FText::Format(LOCTEXT("RenameFailed_InUse", "{0} is in use by another variable or function!"), InNewText));
	}
	else if(ValidatorResult == EValidatorResult::EmptyName)
	{
		VarNameEditableTextBox->SetError(LOCTEXT("RenameFailed_LeftBlank", "Names cannot be left blank!"));
	}
	else if(ValidatorResult == EValidatorResult::TooLong)
	{
		VarNameEditableTextBox->SetError(LOCTEXT("RenameFailed_NameTooLong", "Names must have fewer than 100 characters!"));
	}
	else
	{
		bIsVarNameInvalid = false;
		VarNameEditableTextBox->SetError(FText::GetEmpty());
	}
}

void FBlueprintVarActionDetails::OnVarNameCommitted(const FText& InNewText, ETextCommit::Type InTextCommit)
{
	if(InTextCommit == ETextCommit::OnEnter && !bIsVarNameInvalid)
	{
		const FScopedTransaction Transaction( LOCTEXT( "RenameVariable", "Rename Variable" ) );

		FName NewVarName = FName(*InNewText.ToString());

		// Double check we're not renaming a timeline disguised as a variable
		bool bIsTimeline = false;
		UProperty* VariableProperty = SelectionAsProperty();
		if (VariableProperty != NULL)
		{
			// Don't allow removal of timeline properties - you need to remove the timeline node for that
			UObjectProperty* ObjProperty = Cast<UObjectProperty>(VariableProperty);
			if(ObjProperty != NULL && ObjProperty->PropertyClass == UTimelineComponent::StaticClass())
			{
				bIsTimeline = true;
			}
		}

		// Rename as a timeline if required
		if (bIsTimeline)
		{
			FBlueprintEditorUtils::RenameTimeline(GetBlueprintObj(), GetVariableName(), NewVarName);
		}
		else
		{
			FBlueprintEditorUtils::RenameMemberVariable(GetBlueprintObj(), GetVariableName(), NewVarName);
		}

		check(MyBlueprint.IsValid());
		MyBlueprint.Pin()->SelectItemByName(NewVarName, ESelectInfo::OnMouseClick);
	}

	bIsVarNameInvalid = false;
	VarNameEditableTextBox->SetError(FText::GetEmpty());
}

bool FBlueprintVarActionDetails::GetVariableTypeChangeEnabled() const
{
	FEdGraphSchemaAction_K2Var* VarAction = MyBlueprintSelectionAsVar();
	if (VarAction)
	{
		TArray<UK2Node_Variable*> VariableNodes;
		FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_Variable>(GetBlueprintObj(), VariableNodes);

		bool bNodesPendingDeletion = false;
		for( TArray<UK2Node_Variable*>::TConstIterator NodeIt(VariableNodes); NodeIt; ++NodeIt )
		{
			UK2Node_Variable* CurrentNode = *NodeIt;
			if (VarAction->GetVariableName() == CurrentNode->GetVarName())
			{
				bNodesPendingDeletion = true;
				break;
			}
		}
		bool bIsAVarInThisBlueprint = FBlueprintEditorUtils::FindNewVariableIndex(GetBlueprintObj(), VarAction->GetVariableName()) != INDEX_NONE;
		return !bNodesPendingDeletion && bIsAVarInThisBlueprint;
	}
	return false;
}

bool FBlueprintVarActionDetails::GetVariableCategoryChangeEnabled() const
{
	UProperty* VariableProp = SelectionAsProperty();

	if(VariableProp)
	{
		UClass* VarSourceClass = CastChecked<UClass>(VariableProp->GetOuter());

		// If the variable's source class is the same as the current blueprint's class then it was created in this blueprint and it's category can be changed.
		return VarSourceClass == GetBlueprintObj()->SkeletonGeneratedClass;
	}

	return false;
}

FEdGraphPinType FBlueprintVarActionDetails::OnGetVarType() const
{
	UProperty* VariableProp = SelectionAsProperty();

	if(VariableProp)
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		FEdGraphPinType Type;
		K2Schema->ConvertPropertyToPinType(VariableProp, Type);
		return Type;
	}
	return FEdGraphPinType();
}

void FBlueprintVarActionDetails::OnVarTypeChanged(const FEdGraphPinType& NewPinType)
{
	FName VarName = GetVariableName();
	if (VarName != NAME_None)
	{
		FBlueprintEditorUtils::ChangeMemberVariableType(GetBlueprintObj(), VarName, NewPinType);

		// Set the MyBP tab's last pin type used as this, for adding lots of variables of the same type
		MyBlueprint.Pin()->GetLastPinTypeUsed() = NewPinType;
	}
}

FText FBlueprintVarActionDetails::OnGetTooltipText() const
{
	FName VarName = GetVariableName();
	if (VarName != NAME_None)
	{
		FString Result;
		FBlueprintEditorUtils::GetBlueprintVariableMetaData(GetBlueprintObj(), VarName, TEXT("tooltip"), Result);
		return FText::FromString(Result);
	}
	return FText();
}

void FBlueprintVarActionDetails::OnTooltipTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit, FName VarName)
{
	FBlueprintEditorUtils::SetBlueprintVariableMetaData(GetBlueprintObj(), VarName, TEXT("tooltip"), NewText.ToString() );
}

void FBlueprintVarActionDetails::PopulateCategories(SMyBlueprint* MyBlueprint, TArray<TSharedPtr<FString>>& CategorySource)
{
	TArray<FName> VisibleVariables;
	bool bShowUserVarsOnly = MyBlueprint->ShowUserVarsOnly();
	UBlueprint* Blueprint = MyBlueprint->GetBlueprintObj();
	check(Blueprint && (Blueprint->SkeletonGeneratedClass != NULL));
	EFieldIteratorFlags::SuperClassFlags SuperClassFlag = EFieldIteratorFlags::ExcludeSuper;
	if(!bShowUserVarsOnly)
	{
		SuperClassFlag = EFieldIteratorFlags::IncludeSuper;
	}

	for (TFieldIterator<UProperty> PropertyIt(Blueprint->SkeletonGeneratedClass, SuperClassFlag); PropertyIt; ++PropertyIt)
	{
		UProperty* Property = *PropertyIt;

		if ((!Property->HasAnyPropertyFlags(CPF_Parm) && Property->HasAllPropertyFlags(CPF_BlueprintVisible)))
		{
			VisibleVariables.Add(Property->GetFName());
		}
	}

	CategorySource.Empty();
	CategorySource.Add(MakeShareable(new FString(TEXT("Default"))));
	for (int32 i = 0; i < VisibleVariables.Num(); ++i)
	{
		FName Category = FBlueprintEditorUtils::GetBlueprintVariableCategory(Blueprint, VisibleVariables[i]);
		if (Category != NAME_None && Category != Blueprint->GetFName())
		{
			bool bNewCategory = true;
			for (int32 j = 0; j < CategorySource.Num() && bNewCategory; ++j)
			{
				bNewCategory &= *CategorySource[j].Get() != Category.ToString();
			}
			if (bNewCategory)
			{
				CategorySource.Add(MakeShareable(new FString(Category.ToString())));
			}
		}
	}
}

UK2Node_Variable* FBlueprintVarActionDetails::EdGraphSelectionAsVar() const
{
	TWeakPtr<FBlueprintEditor> BlueprintEditor = MyBlueprint.Pin()->GetBlueprintEditor();

	if( BlueprintEditor.IsValid() )
	{
		/** Get the currently selected set of nodes */
		TSet<UObject*> Objects = BlueprintEditor.Pin()->GetSelectedNodes();

		if (Objects.Num() == 1)
		{
			TSet<UObject*>::TIterator Iter(Objects);
			UObject* Object = *Iter;

			if (Object && Object->IsA<UK2Node_Variable>())
			{
				return Cast<UK2Node_Variable>(Object);
			}
		}
	}
	return NULL;
}

UProperty* FBlueprintVarActionDetails::SelectionAsProperty() const
{
	FEdGraphSchemaAction_K2Var* VarAction = MyBlueprintSelectionAsVar();
	if(VarAction)
	{
		return VarAction->GetProperty();
	}
	UK2Node_Variable* GraphVar = EdGraphSelectionAsVar();
	if(GraphVar)
	{
		return GraphVar->GetPropertyForVariable();
	}
	return NULL;
}

UClass* FBlueprintVarActionDetails::SelectionAsClass() const
{
	FEdGraphSchemaAction_K2Var* VarAction = MyBlueprintSelectionAsVar();
	if(VarAction)
	{
		return VarAction->GetVariableClass();
	}
	UK2Node_Variable* GraphVar = EdGraphSelectionAsVar();
	if(GraphVar)
	{
		return GraphVar->GetClass();
	}
	return NULL;
}

FName FBlueprintVarActionDetails::GetVariableName() const
{
	FEdGraphSchemaAction_K2Var* VarAction = MyBlueprintSelectionAsVar();
	if(VarAction)
	{
		return VarAction->GetVariableName();
	}
	UK2Node_Variable* GraphVar = EdGraphSelectionAsVar();
	if(GraphVar)
	{
		return GraphVar->GetVarName();
	}
	return NAME_None;
}

FText FBlueprintVarActionDetails::OnGetCategoryText() const
{
	FName VarName = GetVariableName();
	if (VarName != NAME_None)
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

		FName Category = FBlueprintEditorUtils::GetBlueprintVariableCategory(GetBlueprintObj(), VarName);

		// Older blueprints will have their name as the default category
		if( Category == GetBlueprintObj()->GetFName() )
		{
			return FText::FromName(K2Schema->VR_DefaultCategory);
		}
		else
		{
			return FText::FromName(Category);
		}
		return FText::FromName(VarName);
	}
	return FText();
}

void FBlueprintVarActionDetails::OnCategoryTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit, FName VarName)
{
	if (InTextCommit == ETextCommit::OnEnter || InTextCommit == ETextCommit::OnUserMovedFocus)
	{
		FString NewCategory = NewText.ToString();
		if(NewCategory.Len() <= NAME_SIZE)
		{
			FBlueprintEditorUtils::SetBlueprintVariableCategory(GetBlueprintObj(), VarName, FName( *NewCategory ));
			check(MyBlueprint.IsValid());
			PopulateCategories(MyBlueprint.Pin().Get(), CategorySource);
			MyBlueprint.Pin()->ExpandCategory(NewCategory);
		}
	}
}

TSharedRef< ITableRow > FBlueprintVarActionDetails::MakeCategoryViewWidget( TSharedPtr<FString> Item, const TSharedRef< STableViewBase >& OwnerTable )
{
	return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
		[
			SNew(STextBlock) .Text(*Item.Get())
		];
}

void FBlueprintVarActionDetails::OnCategorySelectionChanged( TSharedPtr<FString> ProposedSelection, ESelectInfo::Type /*SelectInfo*/ )
{
	FName VarName = GetVariableName();
	if (ProposedSelection.IsValid() && VarName != NAME_None)
	{
		FString NewCategory = *ProposedSelection.Get();

		FBlueprintEditorUtils::SetBlueprintVariableCategory(GetBlueprintObj(), VarName, FName( *NewCategory ));
		CategoryListView.Pin()->ClearSelection();
		CategoryComboButton.Pin()->SetIsOpen(false);
		MyBlueprint.Pin()->ExpandCategory(NewCategory);
	}
}

EVisibility FBlueprintVarActionDetails::ShowEditableCheckboxVisibilty() const
{
	UProperty* VariableProperty = SelectionAsProperty();
	if (VariableProperty)
	{
		if (IsABlueprintVariable(VariableProperty) && !IsAComponentVariable(VariableProperty))
		{
			return EVisibility::Visible;
		}
	}
	return EVisibility::Collapsed;
}

ESlateCheckBoxState::Type FBlueprintVarActionDetails::OnEditableCheckboxState() const
{
	UProperty* VariableProperty = SelectionAsProperty();
	if (VariableProperty)
	{
		return VariableProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance) ? ESlateCheckBoxState::Unchecked : ESlateCheckBoxState::Checked;
	}
	return ESlateCheckBoxState::Unchecked;
}

void FBlueprintVarActionDetails::OnEditableChanged(ESlateCheckBoxState::Type InNewState)
{
	FName VarName = GetVariableName();

	// Toggle the flag on the blueprint's version of the variable description, based on state
	const bool bVariableIsExposed = InNewState == ESlateCheckBoxState::Checked;

	UBlueprint* Blueprint = MyBlueprint.Pin()->GetBlueprintObj();
	FBlueprintEditorUtils::SetBlueprintOnlyEditableFlag(Blueprint, VarName, !bVariableIsExposed);
}

ESlateCheckBoxState::Type FBlueprintVarActionDetails::OnCreateWidgetCheckboxState() const
{
	UProperty* Property = SelectionAsProperty();
	if (Property)
	{
		bool bMakingWidget = FEdMode::ShouldCreateWidgetForProperty(Property);

		return bMakingWidget ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}
	return ESlateCheckBoxState::Unchecked;
}

void FBlueprintVarActionDetails::OnCreateWidgetChanged(ESlateCheckBoxState::Type InNewState)
{
	const FName VarName = GetVariableName();
	if (VarName != NAME_None)
	{
		if (InNewState == ESlateCheckBoxState::Checked)
		{
			FBlueprintEditorUtils::SetBlueprintVariableMetaData(GetBlueprintObj(), VarName, FEdMode::MD_MakeEditWidget, TEXT("true"));
		}
		else
		{
			FBlueprintEditorUtils::RemoveBlueprintVariableMetaData(GetBlueprintObj(), VarName, FEdMode::MD_MakeEditWidget);
		}
	}
}

EVisibility FBlueprintVarActionDetails::Show3DWidgetVisibility() const
{
	UProperty* VariableProperty = SelectionAsProperty();
	if (VariableProperty)
	{
		if (IsABlueprintVariable(VariableProperty) && FEdMode::CanCreateWidgetForProperty(VariableProperty))
		{
			return EVisibility::Visible;
		}
	}
	return EVisibility::Collapsed;
}

bool FBlueprintVarActionDetails::Is3DWidgetEnabled()
{
	UProperty* VariableProperty = SelectionAsProperty();
	if (VariableProperty)
	{
		return ( VariableProperty && !VariableProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance) ) ;
	}
	return false;
}

ESlateCheckBoxState::Type FBlueprintVarActionDetails::OnGetExposedToSpawnCheckboxState() const
{
	UProperty* Property = SelectionAsProperty();
	if (Property)
	{
		return (Property && Property->GetBoolMetaData(FBlueprintMetadata::MD_ExposeOnSpawn) != false) ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}
	return ESlateCheckBoxState::Unchecked;
}

void FBlueprintVarActionDetails::OnExposedToSpawnChanged(ESlateCheckBoxState::Type InNewState)
{
	const FName VarName = GetVariableName();
	if (VarName != NAME_None)
	{
		const bool bExposeOnSpawn = (InNewState == ESlateCheckBoxState::Checked);
		if(bExposeOnSpawn)
		{
			FBlueprintEditorUtils::SetBlueprintVariableMetaData(GetBlueprintObj(), VarName, FBlueprintMetadata::MD_ExposeOnSpawn, TEXT("true"));
		}
		else
		{
			FBlueprintEditorUtils::RemoveBlueprintVariableMetaData(GetBlueprintObj(), VarName, FBlueprintMetadata::MD_ExposeOnSpawn);
		} 
	}
}

EVisibility FBlueprintVarActionDetails::ExposeOnSpawnVisibility() const
{
	UProperty* VariableProperty = SelectionAsProperty();
	if (VariableProperty)
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		FEdGraphPinType VariablePinType;
		K2Schema->ConvertPropertyToPinType(VariableProperty, VariablePinType);

		const bool bShowPrivacySetting = IsABlueprintVariable(VariableProperty) && !IsAComponentVariable(VariableProperty);
		if (bShowPrivacySetting && (K2Schema->FindSetVariableByNameFunction(VariablePinType) != NULL))
		{
			return EVisibility::Visible;
		}
	}
	return EVisibility::Collapsed;
}

ESlateCheckBoxState::Type FBlueprintVarActionDetails::OnGetPrivateCheckboxState() const
{
	UProperty* Property = SelectionAsProperty();
	if (Property)
	{
		return (Property && Property->GetBoolMetaData(FBlueprintMetadata::MD_Private) != false) ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}
	return ESlateCheckBoxState::Unchecked;
}

void FBlueprintVarActionDetails::OnPrivateChanged(ESlateCheckBoxState::Type InNewState)
{
	const FName VarName = GetVariableName();
	if (VarName != NAME_None)
	{
		const bool bExposeOnSpawn = (InNewState == ESlateCheckBoxState::Checked);
		if(bExposeOnSpawn)
		{
			FBlueprintEditorUtils::SetBlueprintVariableMetaData(GetBlueprintObj(), VarName, FBlueprintMetadata::MD_Private, TEXT("true"));
		}
		else
		{
			FBlueprintEditorUtils::RemoveBlueprintVariableMetaData(GetBlueprintObj(), VarName, FBlueprintMetadata::MD_Private);
		}
	}
}

EVisibility FBlueprintVarActionDetails::ExposePrivateVisibility() const
{
	UProperty* Property = SelectionAsProperty();
	if (Property)
	{
		if (IsABlueprintVariable(Property) && !IsAComponentVariable(Property))
		{
			return EVisibility::Visible;
		}
	}
	return EVisibility::Collapsed;
}

ESlateCheckBoxState::Type FBlueprintVarActionDetails::OnGetExposedToMatineeCheckboxState() const
{
	UProperty* Property = SelectionAsProperty();
	if (Property)
	{
		return Property && Property->HasAnyPropertyFlags(CPF_Interp) ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}
	return ESlateCheckBoxState::Unchecked;
}

void FBlueprintVarActionDetails::OnExposedToMatineeChanged(ESlateCheckBoxState::Type InNewState)
{
	// Toggle the flag on the blueprint's version of the variable description, based on state
	const bool bExposeToMatinee = (InNewState == ESlateCheckBoxState::Checked);
	
	const FName VarName = GetVariableName();
	if (VarName != NAME_None)
	{
		FBlueprintEditorUtils::SetInterpFlag(GetBlueprintObj(), VarName, bExposeToMatinee);
	}
}

EVisibility FBlueprintVarActionDetails::ExposeToMatineeVisibility() const
{
	UProperty* VariableProperty = SelectionAsProperty();
	if (VariableProperty)
	{
		const bool bIsInteger = VariableProperty->IsA(UIntProperty::StaticClass());
		const bool bIsNonEnumByte = (VariableProperty->IsA(UByteProperty::StaticClass()) && Cast<const UByteProperty>(VariableProperty)->Enum == NULL);
		const bool bIsFloat = VariableProperty->IsA(UFloatProperty::StaticClass());
		const bool bIsBool = VariableProperty->IsA(UBoolProperty::StaticClass());
		const bool bIsVectorStruct = VariableProperty->IsA(UStructProperty::StaticClass()) && Cast<UStructProperty>(VariableProperty)->Struct->GetFName() == NAME_Vector;
		const bool bIsColorStruct = VariableProperty->IsA(UStructProperty::StaticClass()) && Cast<UStructProperty>(VariableProperty)->Struct->GetFName() == NAME_Color;
		const bool bIsLinearColorStruct = VariableProperty->IsA(UStructProperty::StaticClass()) && Cast<UStructProperty>(VariableProperty)->Struct->GetFName() == NAME_LinearColor;

		if (bIsFloat || bIsBool || bIsVectorStruct || bIsColorStruct || bIsLinearColorStruct)
		{
			return EVisibility::Visible;
		}
	}
	return EVisibility::Collapsed;
}

FText FBlueprintVarActionDetails::OnGetSliderMinValue() const
{
	FName VarName = GetVariableName();
	if (VarName != NAME_None)
	{
		FString Result;
		FBlueprintEditorUtils::GetBlueprintVariableMetaData(GetBlueprintObj(), VarName, TEXT("UIMin"), /*out*/ Result);

		return FText::FromString(Result);
	}
	return FText();
}

void FBlueprintVarActionDetails::OnSliderMinValueChanged(const FText& NewMinValue, ETextCommit::Type CommitInfo)
{
	FName VarName = GetVariableName();
	if (VarName != NAME_None)
	{
		if ((CommitInfo == ETextCommit::OnEnter) || (CommitInfo == ETextCommit::OnUserMovedFocus))
		{
			FBlueprintEditorUtils::SetBlueprintVariableMetaData(GetBlueprintObj(), VarName, TEXT("UIMin"), NewMinValue.ToString());
		}
	}
}

FText FBlueprintVarActionDetails::OnGetSliderMaxValue() const
{
	FName VarName = GetVariableName();
	if (VarName != NAME_None)
	{
		FString Result;
		FBlueprintEditorUtils::GetBlueprintVariableMetaData(GetBlueprintObj(), VarName, TEXT("UIMax"), /*out*/ Result);

		return FText::FromString(Result);
	}
	return FText();
}

void FBlueprintVarActionDetails::OnSliderMaxValueChanged(const FText& NewMaxValue, ETextCommit::Type CommitInfo)
{
	FName VarName = GetVariableName();
	if (VarName != NAME_None)
	{
		if ((CommitInfo == ETextCommit::OnEnter) || (CommitInfo == ETextCommit::OnUserMovedFocus))
		{
			FBlueprintEditorUtils::SetBlueprintVariableMetaData(GetBlueprintObj(), VarName, TEXT("UIMax"), NewMaxValue.ToString());
		}
	}
}

EVisibility FBlueprintVarActionDetails::SliderVisibility() const
{
	UProperty* VariableProperty = SelectionAsProperty();
	if (VariableProperty)
	{
		const bool bIsInteger = VariableProperty->IsA(UIntProperty::StaticClass());
		const bool bIsNonEnumByte = (VariableProperty->IsA(UByteProperty::StaticClass()) && Cast<const UByteProperty>(VariableProperty)->Enum == NULL);
		const bool bIsFloat = VariableProperty->IsA(UFloatProperty::StaticClass());

		if (IsABlueprintVariable(VariableProperty) && (bIsInteger || bIsNonEnumByte || bIsFloat))
		{
			return EVisibility::Visible;
		}
	}
	return EVisibility::Collapsed;
}

TSharedPtr<FString> FBlueprintVarActionDetails::GetVariableReplicationType() const
{
	EVariableReplication::Type VariableReplication = EVariableReplication::None;
	
	uint64 PropFlags = 0;
	UProperty* VariableProperty = SelectionAsProperty();

	if (VariableProperty)
	{
		uint64 *PropFlagPtr = FBlueprintEditorUtils::GetBlueprintVariablePropertyFlags(GetBlueprintObj(), VariableProperty->GetFName());
		
		if (PropFlagPtr != NULL)
		{
			PropFlags = *PropFlagPtr;
			bool IsReplicated = (PropFlags & CPF_Net) > 0;
			bool bHasRepNotify = FBlueprintEditorUtils::GetBlueprintVariableRepNotifyFunc(GetBlueprintObj(), VariableProperty->GetFName()) != NAME_None;
			if (bHasRepNotify)
			{
				// Verify they actually have a valid rep notify function still
				UClass* GenClass = GetBlueprintObj()->SkeletonGeneratedClass;
				UFunction* OnRepFunc = GenClass->FindFunctionByName(FBlueprintEditorUtils::GetBlueprintVariableRepNotifyFunc(GetBlueprintObj(), VariableProperty->GetFName()));
				if( OnRepFunc == NULL || OnRepFunc->NumParms != 0 || OnRepFunc->GetReturnProperty() != NULL )
				{
					bHasRepNotify = false;
					ReplicationOnRepFuncChanged(FName(NAME_None).ToString());	
				}
			}

			VariableReplication = !IsReplicated ? EVariableReplication::None : 
				bHasRepNotify ? EVariableReplication::RepNotify : EVariableReplication::Replicated;
		}
	}

	return ReplicationOptions[(int32)VariableReplication];
}

void FBlueprintVarActionDetails::OnChangeReplication(TSharedPtr<FString> ItemSelected, ESelectInfo::Type SelectInfo)
{
	int32 NewSelection;
	bool bFound = ReplicationOptions.Find(ItemSelected, NewSelection);
	check(bFound && NewSelection != INDEX_NONE);

	EVariableReplication::Type VariableReplication = (EVariableReplication::Type)NewSelection;
	
	UProperty* VariableProperty = SelectionAsProperty();

	if (VariableProperty)
	{
		uint64 *PropFlagPtr = FBlueprintEditorUtils::GetBlueprintVariablePropertyFlags(GetBlueprintObj(), VariableProperty->GetFName());
		if (PropFlagPtr != NULL)
		{
			switch(VariableReplication)
			{
			case EVariableReplication::None:
				*PropFlagPtr &= ~CPF_Net;
				ReplicationOnRepFuncChanged(FName(NAME_None).ToString());	
				break;
				
			case EVariableReplication::Replicated:
				*PropFlagPtr |= CPF_Net;
				ReplicationOnRepFuncChanged(FName(NAME_None).ToString());	
				break;

			case EVariableReplication::RepNotify:
				*PropFlagPtr |= CPF_Net;
				FString NewFuncName = FString::Printf(TEXT("OnRep_%s"), *VariableProperty->GetName());
				UEdGraph* FuncGraph = FindObject<UEdGraph>(GetBlueprintObj(), *NewFuncName);
				if (!FuncGraph)
				{
					FuncGraph = FBlueprintEditorUtils::CreateNewGraph(GetBlueprintObj(), FName(*NewFuncName), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
					FBlueprintEditorUtils::AddFunctionGraph(GetBlueprintObj(), FuncGraph, true, NULL);
				}

				if (FuncGraph)
				{
					ReplicationOnRepFuncChanged(NewFuncName);
				}
				break;
			}

			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprintObj());
		}
	}
}

void FBlueprintVarActionDetails::ReplicationOnRepFuncChanged(const FString& NewOnRepFunc) const
{
	FName NewFuncName = FName(*NewOnRepFunc);
	UProperty* VariableProperty = SelectionAsProperty();

	if (VariableProperty)
	{
		FBlueprintEditorUtils::SetBlueprintVariableRepNotifyFunc(GetBlueprintObj(), VariableProperty->GetFName(), NewFuncName);
		uint64 *PropFlagPtr = FBlueprintEditorUtils::GetBlueprintVariablePropertyFlags(GetBlueprintObj(), VariableProperty->GetFName());
		if (PropFlagPtr != NULL)
		{
			if (NewFuncName != NAME_None)
			{
				*PropFlagPtr |= CPF_RepNotify;
				*PropFlagPtr |= CPF_Net;
			}
			else
			{
				*PropFlagPtr &= ~CPF_RepNotify;
			}
		}
	}
}

EVisibility FBlueprintVarActionDetails::ReplicationVisibility() const
{
	UProperty* VariableProperty = SelectionAsProperty();
	if(VariableProperty)
	{
		if (!IsAComponentVariable(VariableProperty) && IsABlueprintVariable(VariableProperty))
		{
			return EVisibility::Visible;
		}
	}
	return EVisibility::Collapsed;
}

TSharedRef<SWidget> FBlueprintVarActionDetails::BuildEventsMenuForVariable() const
{
	if( MyBlueprint.IsValid() )
	{
		TSharedPtr<SMyBlueprint> MyBlueprintPtr = MyBlueprint.Pin();
		FEdGraphSchemaAction_K2Var* Variable = MyBlueprintPtr->SelectionAsVar();
		UObjectProperty* ComponentProperty = Variable ? Cast<UObjectProperty>(Variable->GetProperty()) : NULL;
		TWeakPtr<FBlueprintEditor> BlueprintEditorPtr = MyBlueprintPtr->GetBlueprintEditor();
		if( BlueprintEditorPtr.IsValid() && ComponentProperty )
		{
			TSharedRef<SSCSEditor> Editor =  BlueprintEditorPtr.Pin()->GetSCSEditor();
			FMenuBuilder MenuBuilder( true, NULL );
			Editor->BuildMenuEventsSection( MenuBuilder, BlueprintEditorPtr.Pin(), ComponentProperty->PropertyClass, 
											FGetSelectedObjectsDelegate::CreateSP(MyBlueprintPtr.Get(), &SMyBlueprint::GetSelectedItemsForContextMenu));
			return MenuBuilder.MakeWidget();
		}
	}
	return SNullWidget::NullWidget;
}

EVisibility FBlueprintVarActionDetails::GetTransientVisibility() const
{
	UProperty* VariableProperty = SelectionAsProperty();
	if (VariableProperty)
	{
		if (IsABlueprintVariable(VariableProperty) && !IsAComponentVariable(VariableProperty))
		{
			return EVisibility::Visible;
		}
	}
	return EVisibility::Collapsed;
}

ESlateCheckBoxState::Type FBlueprintVarActionDetails::OnGetTransientCheckboxState() const
{
	UProperty* Property = SelectionAsProperty();
	if (Property)
	{
		return (Property && Property->HasAnyPropertyFlags(CPF_Transient)) ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}
	return ESlateCheckBoxState::Unchecked;
}

void FBlueprintVarActionDetails::OnTransientChanged(ESlateCheckBoxState::Type InNewState)
{
	UProperty* Property = SelectionAsProperty();
	if (Property)
	{
		const bool bTransientFlag = (InNewState == ESlateCheckBoxState::Checked);
		FBlueprintEditorUtils::SetVariableTransientFlag(GetBlueprintObj(), Property->GetFName(), bTransientFlag);
	}
}

EVisibility FBlueprintVarActionDetails::GetSaveGameVisibility() const
{
	UProperty* VariableProperty = SelectionAsProperty();
	if (VariableProperty)
	{
		if (IsABlueprintVariable(VariableProperty) && !IsAComponentVariable(VariableProperty))
		{
			return EVisibility::Visible;
		}
	}
	return EVisibility::Collapsed;
}

ESlateCheckBoxState::Type FBlueprintVarActionDetails::OnGetSaveGameCheckboxState() const
{
	UProperty* Property = SelectionAsProperty();
	if (Property)
	{
		return (Property && Property->HasAnyPropertyFlags(CPF_SaveGame)) ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}
	return ESlateCheckBoxState::Unchecked;
}

void FBlueprintVarActionDetails::OnSaveGameChanged(ESlateCheckBoxState::Type InNewState)
{
	UProperty* Property = SelectionAsProperty();
	if (Property)
	{
		const bool bSaveGameFlag = (InNewState == ESlateCheckBoxState::Checked);
		FBlueprintEditorUtils::SetVariableSaveGameFlag(GetBlueprintObj(), Property->GetFName(), bSaveGameFlag);
	}
}

EVisibility FBlueprintVarActionDetails::IsTooltipEditVisible() const
{
	UProperty* VariableProperty = SelectionAsProperty();
	if (VariableProperty)
	{
		if (IsABlueprintVariable(VariableProperty) && !IsAComponentVariable(VariableProperty))
		{
			return EVisibility::Visible;
		}
	}
	return EVisibility::Collapsed;
}

// Local Variable Detail Customization
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void FBlueprintLocalVarActionDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	FName VarName =	GetVariableName();
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	IDetailCategoryBuilder& Category = DetailLayout.EditCategory("Local Variable", LOCTEXT("LocalVariableDetailsCategory", "Local Variable").ToString());
	const FSlateFontInfo DetailFontInfo = IDetailLayoutBuilder::GetDetailFont();

	Category.AddCustomRow( TEXT("Variable Name") )
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BlueprintVarActionDetails_VariableNameLabel", "Variable Name").ToString())
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
	.ValueContent()
		[
			SAssignNew(VarNameEditableTextBox, SEditableTextBox)
			.Text(this, &FBlueprintLocalVarActionDetails::OnGetVarName)
			.OnTextChanged(this, &FBlueprintLocalVarActionDetails::OnLocalVarNameChanged)
			.OnTextCommitted(this, &FBlueprintLocalVarActionDetails::OnLocalVarNameCommitted)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		];
	Category.AddCustomRow( TEXT("Variable Type") )
		.NameContent()
		[
			SNew(STextBlock)
			.Text( LOCTEXT("VariableTypeLabel", "Variable Type") )
			.Font( DetailFontInfo )
		]
	.ValueContent()
		[
			SNew(SPinTypeSelector, FGetPinTypeTree::CreateUObject(Schema, &UEdGraphSchema_K2::GetVariableTypeTree))
			.TargetPinType(this, &FBlueprintLocalVarActionDetails::OnGetVarType)
			.OnPinTypeChanged(this, &FBlueprintLocalVarActionDetails::OnVarTypeChanged)
			.IsEnabled(this, &FBlueprintLocalVarActionDetails::GetVariableTypeChangeEnabled)
			.Schema(Schema)
			.bAllowExec(false)
			.Font( DetailFontInfo )
		];
	Category.AddCustomRow( TEXT("Tooltip") )
		.NameContent()
		[
			SNew(STextBlock)
			.Text( LOCTEXT("IsVariableToolTipLabel", "Tooltip") )
			.Font( DetailFontInfo )
		]
	.ValueContent()
		[
			SNew(SEditableTextBox)
			.Text( this, &FBlueprintLocalVarActionDetails::OnGetTooltipText )
			.OnTextCommitted( this, &FBlueprintLocalVarActionDetails::OnTooltipTextCommitted )
			.Font( DetailFontInfo )
		];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

	UK2Node_LocalVariable* FBlueprintLocalVarActionDetails::EdGraphSelectionAsLocalVar() const
{
	TWeakPtr<FBlueprintEditor> BlueprintEditor = MyBlueprint.Pin()->GetBlueprintEditor();

	if( BlueprintEditor.IsValid() )
	{
		/** Get the currently selected set of nodes */
		TSet<UObject*> Objects = BlueprintEditor.Pin()->GetSelectedNodes();

		if (Objects.Num() == 1)
		{
			TSet<UObject*>::TIterator Iter(Objects);
			UObject* Object = *Iter;

			if (Object && Object->IsA<UK2Node_LocalVariable>())
			{
				return Cast<UK2Node_LocalVariable>(Object);
			}
		}
	}
	return NULL;
}

UK2Node_LocalVariable* FBlueprintLocalVarActionDetails::GetDetailsSelection() const
{
	UK2Node_LocalVariable* LocalVar = nullptr;

	// Check if the selection is from the MyBlueprints window
	LocalVar = MyBlueprintSelectionAsLocalVar();
	if(LocalVar)
	{
		return LocalVar;
	}

	// Check if the selection is in the graph
	LocalVar = EdGraphSelectionAsLocalVar();
	if(LocalVar)
	{
		return LocalVar;
	}

	return nullptr;
}

void FBlueprintLocalVarActionDetails::OnLocalVarNameChanged(const FText& InNewText)
{
	bIsVarNameInvalid = true;

	UK2Node_LocalVariable* LocalVar = GetDetailsSelection();

	TSharedPtr<INameValidatorInterface> NameValidator = LocalVar->MakeNameValidator();

	EValidatorResult ValidatorResult = NameValidator->IsValid(InNewText.ToString());
	if(ValidatorResult == EValidatorResult::AlreadyInUse)
	{
		VarNameEditableTextBox->SetError(FText::Format(LOCTEXT("RenameFailed_InUse", "{0} is in use by another variable or function!"), InNewText));
	}
	else if(ValidatorResult == EValidatorResult::EmptyName)
	{
		VarNameEditableTextBox->SetError(LOCTEXT("RenameFailed_LeftBlank", "Names cannot be left blank!"));
	}
	else if(ValidatorResult == EValidatorResult::TooLong)
	{
		VarNameEditableTextBox->SetError(LOCTEXT("RenameFailed_NameTooLong", "Names must have fewer than 100 characters!"));
	}
	else
	{
		bIsVarNameInvalid = false;
		VarNameEditableTextBox->SetError(FText::GetEmpty());
	}
}

void FBlueprintLocalVarActionDetails::OnLocalVarNameCommitted(const FText& InNewText, ETextCommit::Type InTextCommit)
{
	if(InTextCommit == ETextCommit::OnEnter && !bIsVarNameInvalid)
	{
		const FScopedTransaction Transaction( LOCTEXT( "RenameVariable", "Rename Local Variable" ) );

		FName NewVarName = FName(*InNewText.ToString());

		auto LocalVar = GetDetailsSelection();
		LocalVar->Modify();
		LocalVar->CustomVariableName = NewVarName;

		check(MyBlueprint.IsValid());
		MyBlueprint.Pin()->Refresh();
		MyBlueprint.Pin()->SelectItemByName(NewVarName, ESelectInfo::OnMouseClick);
	}

	bIsVarNameInvalid = false;
	VarNameEditableTextBox->SetError(FText::GetEmpty());
}

FName FBlueprintLocalVarActionDetails::GetVariableName() const
{
	UK2Node_LocalVariable* LocalVar = GetDetailsSelection();
	if(LocalVar)
	{
		return LocalVar->CustomVariableName;
	}
	return NAME_None;
}

FText FBlueprintLocalVarActionDetails::OnGetVarName() const
{
	return FText::FromName(GetVariableName());
}

FEdGraphPinType FBlueprintLocalVarActionDetails::OnGetVarType() const
{
	UK2Node_LocalVariable* LocalVar = GetDetailsSelection();
	if(LocalVar)
	{
		return LocalVar->VariableType;
	}
	return FEdGraphPinType();
}

void FBlueprintLocalVarActionDetails::OnVarTypeChanged(const FEdGraphPinType& NewPinType)
{
	UK2Node_LocalVariable* LocalVar = GetDetailsSelection();
	if(LocalVar)
	{
		LocalVar->ChangeVariableType(NewPinType);

		// Set the MyBP tab's last pin type used as this, for adding lots of variables of the same type
		MyBlueprint.Pin()->GetLastPinTypeUsed() = NewPinType;
	}
}

bool FBlueprintLocalVarActionDetails::GetVariableTypeChangeEnabled() const
{
	UK2Node_LocalVariable* VarAction = GetDetailsSelection();
	if (VarAction)
	{
		if(VarAction->GetVariablePin()->LinkedTo.Num() != 0)
		{
			return false;
		}
	}
	return true;
}

FText FBlueprintLocalVarActionDetails::OnGetTooltipText() const
{
	UK2Node_LocalVariable* VarAction = GetDetailsSelection();
	if (VarAction && !VarAction->VariableTooltip.IsEmpty())
	{
		return FText::FromString(VarAction->GetTooltip());
	}
	return FText::GetEmpty();
}

void FBlueprintLocalVarActionDetails::OnTooltipTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
{
	UK2Node_LocalVariable* VarAction = GetDetailsSelection();
	if (VarAction)
	{
		VarAction->VariableTooltip = NewText;
	}
}

static FDetailWidgetRow& AddRow( TArray<TSharedRef<FDetailWidgetRow> >& OutChildRows )
{
	TSharedRef<FDetailWidgetRow> NewRow( new FDetailWidgetRow );
	OutChildRows.Add( NewRow );

	return *NewRow;
}

void FBlueprintGraphArgumentGroupLayout::SetOnRebuildChildren( FSimpleDelegate InOnRegenerateChildren )
{
	GraphActionDetailsPtr.Pin()->SetRefreshDelegate(InOnRegenerateChildren, TargetNode == GraphActionDetailsPtr.Pin()->GetFunctionEntryNode().Get());
}

void FBlueprintGraphArgumentGroupLayout::GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder )
{
	if(TargetNode.IsValid())
	{
		TArray<TSharedPtr<FUserPinInfo>> Pins = TargetNode->UserDefinedPins;

		bool bIsInputNode = TargetNode == GraphActionDetailsPtr.Pin()->GetFunctionEntryNode().Get();
		for (int32 i = 0; i < Pins.Num(); ++i)
		{
			TSharedRef<class FBlueprintGraphArgumentLayout> BlueprintArgumentLayout = MakeShareable(new FBlueprintGraphArgumentLayout(
				TWeakPtr<FUserPinInfo>(Pins[i]),
				TargetNode.Get(),
				GraphActionDetailsPtr,
				FName(*FString::Printf(bIsInputNode ? TEXT("InputArgument%i") : TEXT("OutputArgument%i"), i)),
				bIsInputNode));
			ChildrenBuilder.AddChildCustomBuilder(BlueprintArgumentLayout);
		}
	}
}

// Internal
static bool ShouldAllowWildcard(UK2Node_EditablePinBase* TargetNode)
{
	// allow wildcards for tunnel nodes in macro graphs
	if ( TargetNode->IsA(UK2Node_Tunnel::StaticClass()) )
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		return ( K2Schema->GetGraphType( TargetNode->GetGraph() ) == GT_Macro );
	}

	return false;
}

void FBlueprintGraphArgumentLayout::GenerateHeaderRowContent( FDetailWidgetRow& NodeRow )
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	NodeRow
	.NameContent()
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.FillWidth(1)
		.VAlign(VAlign_Center)
		[
			SNew(SEditableTextBox)
				.Text( this, &FBlueprintGraphArgumentLayout::OnGetArgNameText )
				.OnTextCommitted( this, &FBlueprintGraphArgumentLayout::OnArgNameTextCommitted )
				.Font( IDetailLayoutBuilder::GetDetailFont() )
				.IsEnabled(!ShouldPinBeReadOnly())
		]
	]
	.ValueContent()
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.FillWidth(1)
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, 4.0f, 0.0f)
		[
			SNew(SPinTypeSelector, FGetPinTypeTree::CreateUObject(K2Schema, &UEdGraphSchema_K2::GetVariableTypeTree))
				.TargetPinType(this, &FBlueprintGraphArgumentLayout::OnGetPinInfo)
				.OnPinTypePreChanged(this, &FBlueprintGraphArgumentLayout::OnPrePinInfoChange)
				.OnPinTypeChanged(this, &FBlueprintGraphArgumentLayout::PinInfoChanged)
				.Schema(K2Schema)
				.bAllowExec(TargetNode->CanModifyExecutionWires())
				.bAllowWildcard(ShouldAllowWildcard(TargetNode))
				.IsEnabled(!ShouldPinBeReadOnly())
				.Font( IDetailLayoutBuilder::GetDetailFont() )
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		[
			PropertyCustomizationHelpers::MakeClearButton(FSimpleDelegate::CreateSP(this, &FBlueprintGraphArgumentLayout::OnRemoveClicked))
		]
	];
}

void FBlueprintGraphArgumentLayout::GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder )
{
	if (bHasDefaultValue)
	{
		ChildrenBuilder.AddChildContent( *LOCTEXT( "FunctionArgDetailsDefaultValue", "Default Value" ).ToString() )
		.NameContent()
		[
			SNew(STextBlock)
				.Text( LOCTEXT( "FunctionArgDetailsDefaultValue", "Default Value" ) )
				.ToolTipText( LOCTEXT("FunctionArgDetailsDefaultValueTooltip", "The name of the argument that will be visible to users of this graph.") )
				.Font( IDetailLayoutBuilder::GetDetailFont() )
		]
		.ValueContent()
		[
			SNew(SEditableTextBox)
				.Text( this, &FBlueprintGraphArgumentLayout::OnGetArgDefaultValueText )
				.OnTextCommitted( this, &FBlueprintGraphArgumentLayout::OnArgDefaultValueCommitted )
				.IsEnabled(!ShouldPinBeReadOnly())
				.Font( IDetailLayoutBuilder::GetDetailFont() )
		];

		ChildrenBuilder.AddChildContent( *LOCTEXT( "FunctionArgDetailsPassByReference", "Pass-by-Reference" ).ToString() )
		.NameContent()
		[
			SNew(STextBlock)
				.Text( LOCTEXT( "FunctionArgDetailsPassByReference", "Pass-by-Reference" ) )
				.ToolTipText( LOCTEXT("FunctionArgDetailsPassByReferenceTooltip", "Pass this paremeter by reference?") )
				.Font( IDetailLayoutBuilder::GetDetailFont() )
		]
		.ValueContent()
		[
			SNew(SCheckBox)
				.IsChecked( this, &FBlueprintGraphArgumentLayout::IsRefChecked )
				.OnCheckStateChanged( this, &FBlueprintGraphArgumentLayout::OnRefCheckStateChanged)
				.IsEnabled(!ShouldPinBeReadOnly())
		];
	}
		
	ChildrenBuilder.AddChildContent( *LOCTEXT( "FunctionArgDetailsMoving", "Moving" ).ToString() )
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		[
			SNew(SSpacer)
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2, 0)
		[
			SNew(SButton)
			.ContentPadding(0)
			.OnClicked(this, &FBlueprintGraphArgumentLayout::OnArgMoveUp)
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("BlueprintEditor.Details.ArgUpButton"))
			]
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2, 0)
		[
			SNew(SButton)
			.ContentPadding(0)
			.OnClicked(this, &FBlueprintGraphArgumentLayout::OnArgMoveDown)
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("BlueprintEditor.Details.ArgDownButton"))
			]
		]
	];
}

void FBlueprintGraphArgumentLayout::OnRemoveClicked()
{
	if (ParamItemPtr.IsValid())
	{
		const FScopedTransaction Transaction( LOCTEXT( "RemoveParam", "Remove Parameter" ) );
		TargetNode->Modify();

		TargetNode->RemoveUserDefinedPin(ParamItemPtr.Pin());

		auto MyBlueprint = GraphActionDetailsPtr.Pin()->GetMyBlueprint();
		auto Graph = GraphActionDetailsPtr.Pin()->GetGraph();
		bool bNodeWasCleanedUp = GraphActionDetailsPtr.Pin()->ConditionallyCleanUpResultNode();
		GraphActionDetailsPtr.Pin()->OnParamsChanged(TargetNode, true);
		if (bNodeWasCleanedUp && MyBlueprint.IsValid() && Graph)
		{
			MyBlueprint.Pin()->SelectItemByName(Graph->GetFName());
		}
	}
}

FReply FBlueprintGraphArgumentLayout::OnArgMoveUp()
{
	const int32 ThisParamIndex = TargetNode->UserDefinedPins.Find( ParamItemPtr.Pin() );
	const int32 NewParamIndex = ThisParamIndex-1;
	if (ThisParamIndex != INDEX_NONE && NewParamIndex >= 0)
	{
		const FScopedTransaction Transaction( LOCTEXT("K2_MovePinUp", "Move Pin Up") );
		TargetNode->Modify();

		TargetNode->UserDefinedPins.Swap( ThisParamIndex, NewParamIndex );
		GraphActionDetailsPtr.Pin()->OnParamsChanged(TargetNode, true);
	}
	return FReply::Handled();
}

FReply FBlueprintGraphArgumentLayout::OnArgMoveDown()
{
	const int32 ThisParamIndex = TargetNode->UserDefinedPins.Find( ParamItemPtr.Pin() );
	const int32 NewParamIndex = ThisParamIndex+1;
	if (ThisParamIndex != INDEX_NONE && NewParamIndex < TargetNode->UserDefinedPins.Num())
	{
		const FScopedTransaction Transaction( LOCTEXT("K2_MovePinUp", "Move Pin Up") );
		TargetNode->Modify();

		TargetNode->UserDefinedPins.Swap( ThisParamIndex, NewParamIndex );
		GraphActionDetailsPtr.Pin()->OnParamsChanged(TargetNode, true);
	}
	return FReply::Handled();
}

bool FBlueprintGraphArgumentLayout::ShouldPinBeReadOnly() const
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	if (TargetNode && ParamItemPtr.IsValid())
	{
		// Right now, we only care that the user is unable to edit the auto-generated "then" pin
		if ((ParamItemPtr.Pin()->PinType.PinCategory == Schema->PC_Exec) && (!TargetNode->CanModifyExecutionWires()))
		{
			return true;
		}
	}
	
	return false;
}

FText FBlueprintGraphArgumentLayout::OnGetArgNameText() const
{
	if (ParamItemPtr.IsValid())
	{
		return FText::FromString(ParamItemPtr.Pin()->PinName);
	}
	return FText();
}

void FBlueprintGraphArgumentLayout::OnArgNameTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
{
	if (!NewText.IsEmpty() && TargetNode && ParamItemPtr.IsValid() && GraphActionDetailsPtr.IsValid() && !ShouldPinBeReadOnly())
	{
		const FString& OldName = ParamItemPtr.Pin()->PinName;
		const FString& NewName = NewText.ToString();
		if(OldName != NewName)
		{
			if(GraphActionDetailsPtr.Pin()->OnPinRenamed(TargetNode, OldName, NewName))
			{
				ParamItemPtr.Pin()->PinName = NewName;
			}
		}
	}
}

FEdGraphPinType FBlueprintGraphArgumentLayout::OnGetPinInfo() const
{
	if (ParamItemPtr.IsValid())
	{
		return ParamItemPtr.Pin()->PinType;
	}
	return FEdGraphPinType();
}

ESlateCheckBoxState::Type FBlueprintGraphArgumentLayout::IsRefChecked() const
{
	FEdGraphPinType PinType = OnGetPinInfo();
	return PinType.bIsReference? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void FBlueprintGraphArgumentLayout::OnRefCheckStateChanged(ESlateCheckBoxState::Type InState)
{
	FEdGraphPinType PinType = OnGetPinInfo();
	PinType.bIsReference = (InState == ESlateCheckBoxState::Checked)? true : false;
	PinInfoChanged(PinType);
}

void FBlueprintGraphArgumentLayout::PinInfoChanged(const FEdGraphPinType& PinType)
{
	if (ParamItemPtr.IsValid())
	{
		ParamItemPtr.Pin()->PinType = PinType;
		if (GraphActionDetailsPtr.IsValid())
		{
			GraphActionDetailsPtr.Pin()->GetMyBlueprint().Pin()->GetLastFunctionPinTypeUsed() = PinType;
			if( !ShouldPinBeReadOnly() )
			{
				GraphActionDetailsPtr.Pin()->OnParamsChanged(TargetNode);
			}
		}
	}
}

void FBlueprintGraphArgumentLayout::OnPrePinInfoChange(const FEdGraphPinType& PinType)
{
	if( !ShouldPinBeReadOnly() && TargetNode )
	{
		TargetNode->Modify();
	}
}

FText FBlueprintGraphArgumentLayout::OnGetArgDefaultValueText() const
{
	if (ParamItemPtr.IsValid())
	{
		return FText::FromString(ParamItemPtr.Pin()->PinDefaultValue);
	}
	return FText();
}

void FBlueprintGraphArgumentLayout::OnArgDefaultValueCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
{
	if (!NewText.IsEmpty() && !ShouldPinBeReadOnly() && (InTextCommit == ETextCommit::OnEnter) && ParamItemPtr.IsValid())
	{
		bool bSuccess = TargetNode->ModifyUserDefinedPinDefaultValue(ParamItemPtr.Pin(), NewText.ToString());
		
		if (bSuccess)
		{
			GraphActionDetailsPtr.Pin()->OnParamsChanged(TargetNode);
		}
	}
}


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FBlueprintGraphActionDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	DetailsLayoutPtr = &DetailLayout;
	ObjectsBeingEdited = DetailsLayoutPtr->GetDetailsView().GetSelectedObjects();

	SetEntryAndResultNodes();

	UK2Node_EditablePinBase* FunctionEntryNode = FunctionEntryNodePtr.Get();
	UK2Node_EditablePinBase* FunctionResultNode = FunctionResultNodePtr.Get();

	// Fill Access specifiers list
	AccessSpecifierLabels.Empty(3);
	AccessSpecifierLabels.Add( MakeShareable( new FAccessSpecifierLabel( AccessSpecifierProperName(FUNC_Public), FUNC_Public )));
	AccessSpecifierLabels.Add( MakeShareable( new FAccessSpecifierLabel( AccessSpecifierProperName(FUNC_Protected), FUNC_Protected )));
	AccessSpecifierLabels.Add( MakeShareable( new FAccessSpecifierLabel( AccessSpecifierProperName(FUNC_Private), FUNC_Private )));

	const bool bHasAGraph = (GetGraph() != NULL);

	if (FunctionEntryNode && FunctionEntryNode->IsEditable())
	{
		IDetailCategoryBuilder& Category = DetailLayout.EditCategory("Graph", LOCTEXT("FunctionDetailsGraph", "Graph").ToString());
		if (bHasAGraph)
		{
			Category.AddCustomRow( LOCTEXT( "DefaultTooltip", "Description" ).ToString() )
			.NameContent()
			[
				SNew(STextBlock)
					.Text( LOCTEXT( "DefaultTooltip", "Description" ).ToString() )
					.ToolTipText(LOCTEXT("FunctionTooltipTooltip", "Enter a short message describing the purpose and operation of this graph").ToString())
					.Font( IDetailLayoutBuilder::GetDetailFont() )
			]
			.ValueContent()
			[
				SNew(SEditableTextBox)
					.Text( this, &FBlueprintGraphActionDetails::OnGetTooltipText )
					.OnTextCommitted( this, &FBlueprintGraphActionDetails::OnTooltipTextCommitted )
					.Font( IDetailLayoutBuilder::GetDetailFont() )
			];
			Category.AddCustomRow( LOCTEXT( "Category", "Category" ).ToString() )
			.NameContent()
			[
				SNew(STextBlock)
					.Text( LOCTEXT( "Category", "Category" ).ToString() )
					.ToolTipText(LOCTEXT("CategoryTooltip", "The category affects how this function will show up in menus and when searching").ToString())
					.Font( IDetailLayoutBuilder::GetDetailFont() )
			]
			.ValueContent()
			[
				SNew(SEditableTextBox)
					.Text( this, &FBlueprintGraphActionDetails::OnGetCategoryText )
					.OnTextCommitted( this, &FBlueprintGraphActionDetails::OnCategoryTextCommitted )
					.Font( IDetailLayoutBuilder::GetDetailFont() )
			];
			if (IsAccessSpecifierVisible())
			{
				Category.AddCustomRow( LOCTEXT( "AccessSpecifier", "Access Specifier" ).ToString() )
				.NameContent()
				[
					SNew(STextBlock)
						.Text( LOCTEXT( "AccessSpecifier", "Access Specifier" ).ToString() )
						.Font( IDetailLayoutBuilder::GetDetailFont() )
				]
				.ValueContent()
				[
					SAssignNew(AccessSpecifierComboButton, SComboButton)
					.ContentPadding(0)
					.ButtonContent()
					[
						SNew(STextBlock)
							.Text(this, &FBlueprintGraphActionDetails::GetCurrentAccessSpecifierName)
							.Font( IDetailLayoutBuilder::GetDetailFont() )
					]
					.MenuContent()
					[
						SNew(SListView<TSharedPtr<FAccessSpecifierLabel> >)
							.ListItemsSource( &AccessSpecifierLabels )
							.OnGenerateRow(this, &FBlueprintGraphActionDetails::HandleGenerateRowAccessSpecifier)
							.OnSelectionChanged(this, &FBlueprintGraphActionDetails::OnAccessSpecifierSelected)
					]
				];
			}
			if (GetInstanceColorVisibility())
			{
				Category.AddCustomRow( LOCTEXT( "InstanceColor", "Instance Color" ).ToString() )
				.NameContent()
				[
					SNew(STextBlock)
						.Text( LOCTEXT( "InstanceColor", "Instance Color" ) )
						.ToolTipText( LOCTEXT("FunctionColorTooltip", "Choose a title bar color for references of this graph") )
						.Font( IDetailLayoutBuilder::GetDetailFont() )
				]
				.ValueContent()
				[
					SAssignNew( ColorBlock, SColorBlock )
						.Color( this, &FBlueprintGraphActionDetails::GetNodeTitleColor )
						.IgnoreAlpha(true)
						.OnMouseButtonDown( this, &FBlueprintGraphActionDetails::ColorBlock_OnMouseButtonDown )
				];
			}
			if (IsPureFunctionVisible())
			{
				Category.AddCustomRow( LOCTEXT( "FunctionPure_Tooltip", "Pure" ).ToString() )
				.NameContent()
				[
					SNew(STextBlock)
						.Text( LOCTEXT( "FunctionPure_Tooltip", "Pure" ).ToString() )
						.ToolTipText( LOCTEXT("FunctionIsPure_Tooltip", "Force this to be a pure function?").ToString() )
						.Font( IDetailLayoutBuilder::GetDetailFont() )
				]
				.ValueContent()
				[
					SNew( SCheckBox )
						.IsChecked( this, &FBlueprintGraphActionDetails::GetIsPureFunction )
						.OnCheckStateChanged( this, &FBlueprintGraphActionDetails::OnIsPureFunctionModified )
				];
			}
		}

		if (IsCustomEvent())
		{
			FMenuBuilder RepComboMenu( true, NULL );
			RepComboMenu.AddMenuEntry( 	ReplicationSpecifierProperName(0), 
										LOCTEXT("NotReplicatedToolTip", "This event is not replicated to anyone."),
										FSlateIcon(),
										FUIAction(FExecuteAction::CreateStatic( &FBlueprintGraphActionDetails::SetNetFlags, FunctionEntryNodePtr, 0U )));
			RepComboMenu.AddMenuEntry(	ReplicationSpecifierProperName(FUNC_NetMulticast), 
										LOCTEXT("MulticastToolTip", "Replicate this event from the server to everyone else. Server executes this event locally too. Only call this from the server."),
										FSlateIcon(),
										FUIAction(FExecuteAction::CreateStatic( &FBlueprintGraphActionDetails::SetNetFlags, FunctionEntryNodePtr, static_cast<uint32>(FUNC_NetMulticast) )));
			RepComboMenu.AddMenuEntry(	ReplicationSpecifierProperName(FUNC_NetServer), 
										LOCTEXT("ServerToolTip", "Replicate this event from net owning client to server."),
										FSlateIcon(),
										FUIAction(FExecuteAction::CreateStatic( &FBlueprintGraphActionDetails::SetNetFlags, FunctionEntryNodePtr, static_cast<uint32>(FUNC_NetServer) )));
			RepComboMenu.AddMenuEntry(	ReplicationSpecifierProperName(FUNC_NetClient), 
										LOCTEXT("ClientToolTip", "Replicate this event from the server to owning client."),
										FSlateIcon(),
										FUIAction(FExecuteAction::CreateStatic( &FBlueprintGraphActionDetails::SetNetFlags, FunctionEntryNodePtr, static_cast<uint32>(FUNC_NetClient) )));

			Category.AddCustomRow( LOCTEXT( "FunctionReplicate", "Replicates" ).ToString() )
			.NameContent()
			[
				SNew(STextBlock)
					.Text( LOCTEXT( "FunctionReplicate", "Replicates" ) )
					.ToolTipText( LOCTEXT("FunctionReplicate_Tooltip", "Should this Event be replicated to all clients when called on the server?") )
					.Font( IDetailLayoutBuilder::GetDetailFont() )
			]
			.ValueContent()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				[
					SNew(SComboButton)
					.ContentPadding(0)
					.ButtonContent()
					[
						SNew(STextBlock)
						.Text(this, &FBlueprintGraphActionDetails::GetCurrentReplicatedEventString)
						.Font( IDetailLayoutBuilder::GetDetailFont() )
					]
					.MenuContent()
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot()
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.AutoHeight()
							.MaxHeight(400.0f)
							[
								RepComboMenu.MakeWidget()
							]
						]
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.MaxHeight(400.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew( SCheckBox )
						.IsChecked( this, &FBlueprintGraphActionDetails::GetIsReliableReplicatedFunction )
						.IsEnabled( this, &FBlueprintGraphActionDetails::GetIsReplicatedFunction )
						.OnCheckStateChanged( this, &FBlueprintGraphActionDetails::OnIsReliableReplicationFunctionModified )
						[
							SNew(STextBlock)
							.Text( LOCTEXT( "FunctionReplicateReliable", "Reliable" ).ToString() )
						]
					]
				]
			];
		}

		IDetailCategoryBuilder& InputsCategory = DetailLayout.EditCategory("Inputs", LOCTEXT("FunctionDetailsInputs", "Inputs").ToString());
		
		TSharedRef<FBlueprintGraphArgumentGroupLayout> InputArgumentGroup =
			MakeShareable(new FBlueprintGraphArgumentGroupLayout(SharedThis(this), FunctionEntryNode));
		InputsCategory.AddCustomBuilder(InputArgumentGroup);

		InputsCategory.AddCustomRow( LOCTEXT("FunctionNewInputArg", "New").ToString() )
		[
			SNew(SBox)
			.HAlign(HAlign_Right)
			[
				SNew(SButton)
				.Text(LOCTEXT("FunctionNewInputArg", "New").ToString())
				.OnClicked(this, &FBlueprintGraphActionDetails::OnAddNewInputClicked)
			]
		];

		if (bHasAGraph)
		{
			IDetailCategoryBuilder& OutputsCategory = DetailLayout.EditCategory("Outputs", LOCTEXT("FunctionDetailsOutputs", "Outputs").ToString());
		
			if (FunctionResultNode)
			{
				TSharedRef<FBlueprintGraphArgumentGroupLayout> OutputArgumentGroup =
					MakeShareable(new FBlueprintGraphArgumentGroupLayout(SharedThis(this), FunctionResultNode));
				OutputsCategory.AddCustomBuilder(OutputArgumentGroup);
			}

			OutputsCategory.AddCustomRow( LOCTEXT("FunctionNewOutputArg", "New").ToString() )
			[
				SNew(SBox)
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.Text(LOCTEXT("FunctionNewOutputArg", "New").ToString())
					.OnClicked(this, &FBlueprintGraphActionDetails::OnAddNewOutputClicked)
				]
			];
		}
	}
	else
	{
		if (bHasAGraph)
		{
			IDetailCategoryBuilder& Category = DetailLayout.EditCategory("Graph", LOCTEXT("FunctionDetailsGraph", "Graph").ToString());
			Category.AddCustomRow( TEXT("") )
			[
				SNew(STextBlock)
				.Text( LOCTEXT("GraphPresentButNotEditable", "Graph is not editable.").ToString() )
			];
		}
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<ITableRow> FBlueprintGraphActionDetails::OnGenerateReplicationComboWidget( TSharedPtr<FReplicationSpecifierLabel> InNetFlag, const TSharedRef<STableViewBase>& OwnerTable )
{
	return
		SNew(STableRow< TSharedPtr<FString> >, OwnerTable)
		[
			SNew( STextBlock )
			.Text( InNetFlag.IsValid() ? InNetFlag.Get()->LocalizedName.ToString() : FString() )
			.ToolTipText( InNetFlag.IsValid() ? InNetFlag.Get()->LocalizedToolTip.ToString() : FString() )
		];
}

void FBlueprintGraphActionDetails::SetNetFlags( TWeakObjectPtr<UK2Node_EditablePinBase> FunctionEntryNode, uint32 NetFlags)
{
	if( FunctionEntryNode.IsValid() )
	{
		// Clear all net flags before setting
		if (UK2Node_FunctionEntry* TypedEntryNode = Cast<UK2Node_FunctionEntry>(FunctionEntryNode.Get()))
		{
			TypedEntryNode->ExtraFlags &= ~FUNC_Net;
			TypedEntryNode->ExtraFlags &= ~FUNC_NetMulticast;
			TypedEntryNode->ExtraFlags &= ~FUNC_NetServer;
			TypedEntryNode->ExtraFlags &= ~FUNC_NetClient;
		}
		if (UK2Node_CustomEvent * CustomEventNode = Cast<UK2Node_CustomEvent>(FunctionEntryNode.Get()))
		{
			CustomEventNode->FunctionFlags &= ~FUNC_Net;
			CustomEventNode->FunctionFlags &= ~FUNC_NetMulticast;
			CustomEventNode->FunctionFlags &= ~FUNC_NetServer;
			CustomEventNode->FunctionFlags &= ~FUNC_NetClient;
		}

		if (NetFlags > 0)
		{
			if (UK2Node_FunctionEntry* TypedEntryNode = Cast<UK2Node_FunctionEntry>(FunctionEntryNode.Get()))
			{
				TypedEntryNode->ExtraFlags |= FUNC_Net;
				TypedEntryNode->ExtraFlags |= NetFlags;
			}
			if (UK2Node_CustomEvent * CustomEventNode = Cast<UK2Node_CustomEvent>(FunctionEntryNode.Get()))
			{
				CustomEventNode->FunctionFlags |= FUNC_Net;
				CustomEventNode->FunctionFlags |= NetFlags;
			}
		}
	}
}

FString FBlueprintGraphActionDetails::GetCurrentReplicatedEventString() const
{
	const UK2Node_EditablePinBase * FunctionEntryNode = FunctionEntryNodePtr.Get();
	const UK2Node_CustomEvent* CustomEvent = Cast<const UK2Node_CustomEvent>(FunctionEntryNode);

	uint32 NetFlags = CustomEvent->FunctionFlags & (FUNC_NetMulticast | FUNC_NetServer | FUNC_NetClient);
	return ReplicationSpecifierProperName(NetFlags).ToString();
}

bool FBaseBlueprintGraphActionDetails::ConditionallyCleanUpResultNode()
{
	UEdGraph* Graph = GetGraph();
	UK2Node_EditablePinBase * FunctionResultNode = FunctionResultNodePtr.Get();

	if( Graph && FunctionResultNode && FunctionResultNode->UserDefinedPins.Num() == 0 &&
		!Cast<UK2Node_Tunnel>(FunctionResultNode))
	{
		Graph->RemoveNode(FunctionResultNode);
		FunctionResultNodePtr = NULL;

		return true;
	}
	return false;
}

bool FBaseBlueprintGraphActionDetails::AttemptToCreateResultNode()
{
	UEdGraph* Graph = GetGraph();
	
	if( !FunctionResultNodePtr.IsValid() && Graph && GetBlueprintObj() )
	{
		FGraphNodeCreator<UK2Node_FunctionResult> ResultNodeCreator(*Graph);
		UK2Node_FunctionResult* FunctionResult = ResultNodeCreator.CreateNode();

		UK2Node_EditablePinBase * FunctionEntryNode = FunctionEntryNodePtr.Get();
		check(FunctionEntryNode);

		const UEdGraphSchema_K2* Schema = Cast<const UEdGraphSchema_K2>(FunctionResult->GetSchema());
		FunctionResult->NodePosX = FunctionEntryNode->NodePosX + FunctionEntryNode->NodeWidth + 256;
		FunctionResult->NodePosY = FunctionEntryNode->NodePosY;
			
		ResultNodeCreator.Finalize();
		
		// Connect the function entry to the result node, if applicable
		UEdGraphPin* ThenPin = Schema->FindExecutionPin(*FunctionEntryNode, EGPD_Output);
		UEdGraphPin* ReturnPin = Schema->FindExecutionPin(*FunctionResult, EGPD_Input);

		if(ThenPin->LinkedTo.Num() == 0) 
		{
			ThenPin->MakeLinkTo(ReturnPin);
		}
		else
		{
			// Bump the result node up a bit, so it's less likely to fall behind the node the entry is already connected to
			FunctionResult->NodePosY -= 100;
		}

		FunctionResultNodePtr = FunctionResult;
	}

	return (FunctionResultNodePtr.IsValid());
}

void FBaseBlueprintGraphActionDetails::SetRefreshDelegate(FSimpleDelegate RefreshDelegate, bool bForInputs)
{
	((bForInputs) ? RegenerateInputsChildrenDelegate : RegenerateOutputsChildrenDelegate) = RefreshDelegate;
}

UMulticastDelegateProperty* FBlueprintDelegateActionDetails::GetDelegatePoperty() const
{
	if (MyBlueprint.IsValid())
	{
		if (const FEdGraphSchemaAction_K2Delegate* DelegateVar = MyBlueprint.Pin()->SelectionAsDelegate())
		{
			return DelegateVar->GetDelegatePoperty();
		}
	}
	return NULL;
}

bool FBlueprintDelegateActionDetails::IsBlueprintProperty() const
{
	const UMulticastDelegateProperty* Property = GetDelegatePoperty();
	const UBlueprint* Blueprint = GetBlueprintObj();
	if(Property && Blueprint)
	{
		return (Property->GetOuter() == Blueprint->SkeletonGeneratedClass);
	}

	return false;
}

void FBlueprintDelegateActionDetails::SetEntryNode()
{
	if (UEdGraph* NewTargetGraph = GetGraph())
	{
		TArray<UK2Node_FunctionEntry*> EntryNodes;
		NewTargetGraph->GetNodesOfClass(EntryNodes);

		if ((EntryNodes.Num() > 0) && EntryNodes[0]->IsEditable())
		{
			FunctionEntryNodePtr = EntryNodes[0];
		}
	}
}

UEdGraph* FBlueprintDelegateActionDetails::GetGraph() const
{
	if (MyBlueprint.IsValid())
	{
		if (const FEdGraphSchemaAction_K2Delegate* DelegateVar = MyBlueprint.Pin()->SelectionAsDelegate())
		{
			return DelegateVar->EdGraph;
		}
	}
	return NULL;
}

FText FBlueprintDelegateActionDetails::OnGetTooltipText() const
{
	if (UMulticastDelegateProperty* DelegateProperty = GetDelegatePoperty())
	{
		FString Result;
		FBlueprintEditorUtils::GetBlueprintVariableMetaData(GetBlueprintObj(), DelegateProperty->GetFName(), TEXT("tooltip"), Result);
		return FText::FromString(Result);
	}
	return FText();
}

void FBlueprintDelegateActionDetails::OnTooltipTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
{
	if (UMulticastDelegateProperty* DelegateProperty = GetDelegatePoperty())
	{
		FBlueprintEditorUtils::SetBlueprintVariableMetaData(GetBlueprintObj(), DelegateProperty->GetFName(), TEXT("tooltip"), NewText.ToString() );
	}
}

FText FBlueprintDelegateActionDetails::OnGetCategoryText() const
{
	if (UMulticastDelegateProperty* DelegateProperty = GetDelegatePoperty())
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		FName DelegateName = DelegateProperty->GetFName();
		FName Category = FBlueprintEditorUtils::GetBlueprintVariableCategory(GetBlueprintObj(), DelegateName);

		// Older blueprints will have their name as the default category
		if( Category == GetBlueprintObj()->GetFName() )
		{
			return FText::FromName(K2Schema->VR_DefaultCategory);
		}
		else
		{
			return FText::FromName(Category);
		}
		return FText::FromName(DelegateName);
	}
	return FText();
}

void FBlueprintDelegateActionDetails::OnCategoryTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
{
	if (InTextCommit == ETextCommit::OnEnter || InTextCommit == ETextCommit::OnUserMovedFocus)
	{
		if (UMulticastDelegateProperty* DelegateProperty = GetDelegatePoperty())
		{
			FString NewCategory = NewText.ToString();
			
			FBlueprintEditorUtils::SetBlueprintVariableCategory(GetBlueprintObj(), DelegateProperty->GetFName(), FName( *NewCategory ));
			check(MyBlueprint.IsValid());
			FBlueprintVarActionDetails::PopulateCategories(MyBlueprint.Pin().Get(), CategorySource);
			MyBlueprint.Pin()->ExpandCategory(NewCategory);
		}
	}
}

TSharedRef< ITableRow > FBlueprintDelegateActionDetails::MakeCategoryViewWidget( TSharedPtr<FString> Item, const TSharedRef< STableViewBase >& OwnerTable )
{
	return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
		[
			SNew(STextBlock) .Text(*Item.Get())
		];
}

void FBlueprintDelegateActionDetails::OnCategorySelectionChanged( TSharedPtr<FString> ProposedSelection, ESelectInfo::Type /*SelectInfo*/ )
{
	UMulticastDelegateProperty* DelegateProperty = GetDelegatePoperty();
	if (DelegateProperty && ProposedSelection.IsValid())
	{
		FString NewCategory = *ProposedSelection.Get();

		FBlueprintEditorUtils::SetBlueprintVariableCategory(GetBlueprintObj(), DelegateProperty->GetFName(), FName( *NewCategory ));
		CategoryListView.Pin()->ClearSelection();
		CategoryComboButton.Pin()->SetIsOpen(false);
		MyBlueprint.Pin()->ExpandCategory(NewCategory);
	}
}

void FBlueprintDelegateActionDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	DetailsLayoutPtr = &DetailLayout;
	ObjectsBeingEdited = DetailsLayoutPtr->GetDetailsView().GetSelectedObjects();

	SetEntryNode();

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	const FSlateFontInfo DetailFontInfo = IDetailLayoutBuilder::GetDetailFont();

	{
		IDetailCategoryBuilder& Category = DetailLayout.EditCategory("Delegate", LOCTEXT("DelegateDetailsCategory", "Delegate").ToString());
		Category.AddCustomRow( TEXT("Tooltip") )
		.NameContent()
		[
			SNew(STextBlock)
				.Text( LOCTEXT("VariableToolTipLabel", "Tooltip") )
				.Font( DetailFontInfo )
		]
		.ValueContent()
		[
			SNew(SEditableTextBox)
				.Text( this, &FBlueprintDelegateActionDetails::OnGetTooltipText )
				.OnTextCommitted( this, &FBlueprintDelegateActionDetails::OnTooltipTextCommitted)
				.Font( DetailFontInfo )
		];

		FBlueprintVarActionDetails::PopulateCategories(MyBlueprint.Pin().Get(), CategorySource);
		TSharedPtr<SComboButton> NewComboButton;
		TSharedPtr<SListView<TSharedPtr<FString>>> NewListView;

		Category.AddCustomRow( TEXT("Category") )
		.NameContent()
		[
			SNew(STextBlock)
				.Text( LOCTEXT("CategoryLabel", "Category") )
				.Font( DetailFontInfo )
		]
		.ValueContent()
		[
			SAssignNew(NewComboButton, SComboButton)
			.ContentPadding(FMargin(0,0,5,0))
			.IsEnabled(this, &FBlueprintDelegateActionDetails::IsBlueprintProperty)
			.ButtonContent()
			[
				SNew(SBorder)
				.BorderImage( FEditorStyle::GetBrush("NoBorder") )
				.Padding(FMargin(0, 0, 5, 0))
				[
					SNew(SEditableTextBox)
						.Text(this, &FBlueprintDelegateActionDetails::OnGetCategoryText)
						.OnTextCommitted(this, &FBlueprintDelegateActionDetails::OnCategoryTextCommitted)
						.SelectAllTextWhenFocused(true)
						.RevertTextOnEscape(true)
						.Font( DetailFontInfo )
				]
			]
			.MenuContent()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				.MaxHeight(400.0f)
				[
					SAssignNew(NewListView, SListView<TSharedPtr<FString>>)
						.ListItemsSource(&CategorySource)
						.OnGenerateRow(this, &FBlueprintDelegateActionDetails::MakeCategoryViewWidget)
						.OnSelectionChanged(this, &FBlueprintDelegateActionDetails::OnCategorySelectionChanged)
				]
			]
		];

		CategoryComboButton = NewComboButton;
		CategoryListView = NewListView;
	}

	if (UK2Node_EditablePinBase* FunctionEntryNode = FunctionEntryNodePtr.Get())
	{
		IDetailCategoryBuilder& InputsCategory = DetailLayout.EditCategory("DelegateInputs", LOCTEXT("DelegateDetailsInputs", "Inputs").ToString());
		TSharedRef<FBlueprintGraphArgumentGroupLayout> InputArgumentGroup = MakeShareable(new FBlueprintGraphArgumentGroupLayout(SharedThis(this), FunctionEntryNode));
		InputsCategory.AddCustomBuilder(InputArgumentGroup);

		InputsCategory.AddCustomRow( LOCTEXT("FunctionNewInputArg", "New").ToString() )
		[
			SNew(SBox)
			.HAlign(HAlign_Right)
			[
				SNew(SButton)
				.Text(LOCTEXT("FunctionNewInputArg", "New"))
				.OnClicked(this, &FBlueprintDelegateActionDetails::OnAddNewInputClicked)
			]
		];

		CollectAvailibleSignatures();

		InputsCategory.AddCustomRow( LOCTEXT("CopySignatureFrom", "Copy signature from").ToString() )
		.NameContent()
		[
			SNew(STextBlock)
				.Text(LOCTEXT("CopySignatureFrom", "Copy signature from"))
				.Font( DetailFontInfo )
		]
		.ValueContent()
		[
			SAssignNew(CopySignatureComboButton, STextComboBox)
				.OptionsSource(&FunctionsToCopySignatureFrom)
				.OnSelectionChanged(this, &FBlueprintDelegateActionDetails::OnFunctionSelected)
		];
	}
}

void FBlueprintDelegateActionDetails::CollectAvailibleSignatures()
{
	FunctionsToCopySignatureFrom.Empty();
	if (UMulticastDelegateProperty* Property = GetDelegatePoperty())
	{
		if (UClass* ScopeClass = Cast<UClass>(Property->GetOuterUField()))
		{
			for(TFieldIterator<UFunction> It(ScopeClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
			{
				UFunction* Func = *It;
				if (UEdGraphSchema_K2::FunctionCanBeUsedInDelegate(Func))
				{
					TSharedPtr<FString> ItemData = MakeShareable(new FString(Func->GetName()));
					FunctionsToCopySignatureFrom.Add(ItemData);
				}
			}
		}
	}
}

void FBlueprintDelegateActionDetails::OnFunctionSelected(TSharedPtr<FString> FunctionName, ESelectInfo::Type SelectInfo)
{
	UK2Node_EditablePinBase* FunctionEntryNode = FunctionEntryNodePtr.Get();
	UMulticastDelegateProperty* Property = GetDelegatePoperty();
	UClass* ScopeClass = Property ? Cast<UClass>(Property->GetOuterUField()) : NULL;
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	if (FunctionEntryNode && FunctionName.IsValid() && ScopeClass)
	{
		const FName Name( *(*FunctionName) );
		if (UFunction* NewSignature = ScopeClass->FindFunctionByName(Name))
		{
			while (FunctionEntryNode->UserDefinedPins.Num())
			{
				auto Pin = FunctionEntryNode->UserDefinedPins[0];
				FunctionEntryNode->RemoveUserDefinedPin(Pin);
			}

			for (TFieldIterator<UProperty> PropIt(NewSignature); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
			{
				UProperty* FuncParam = *PropIt;
				FEdGraphPinType TypeOut;
				Schema->ConvertPropertyToPinType(FuncParam, TypeOut);
				FunctionEntryNode->CreateUserDefinedPin(FuncParam->GetName(), TypeOut);
			}

			OnParamsChanged(FunctionEntryNode);
		}
	}
}

struct FBasePinChangeHelper
{
	static bool NodeIsNotTransient(const UK2Node* Node)
	{
		return (NULL != Node)
			&& !Node->HasAnyFlags(RF_Transient) 
			&& (NULL != Cast<UEdGraph>(Node->GetOuter()));
	}

	virtual void EditCompositeTunnelNode(UK2Node_Tunnel* TunnelNode) {}

	virtual void EditMacroInstance(UK2Node_MacroInstance* MacroInstance, UBlueprint* Blueprint) {}

	virtual void EditCallSite(UK2Node_CallFunction* CallSite, UBlueprint* Blueprint) {}

	virtual void EditDelegates(UK2Node_BaseMCDelegate* CallSite, UBlueprint* Blueprint) {}

	virtual void EditCreateDelegates(UK2Node_CreateDelegate* CallSite) {}

	void Broadcast(UK2Node_EditablePinBase* InTargetNode, FBaseBlueprintGraphActionDetails& BlueprintGraphActionDetails, UEdGraph* Graph)
	{
		if (UK2Node_Tunnel* TunnelNode = Cast<UK2Node_Tunnel>(InTargetNode))
		{
			UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraphChecked(Graph);

			const bool bIsTopLevelFunctionGraph = Blueprint->MacroGraphs.Contains(Graph);

			if (bIsTopLevelFunctionGraph)
			{
				// Editing a macro, hit all loaded instances (in open blueprints)
				for (TObjectIterator<UK2Node_MacroInstance> It(RF_Transient); It; ++It)
				{
					UK2Node_MacroInstance* MacroInstance = *It;
					if (NodeIsNotTransient(MacroInstance) && (MacroInstance->GetMacroGraph() == Graph))
					{
						EditMacroInstance(MacroInstance, FBlueprintEditorUtils::FindBlueprintForNode(MacroInstance));
					}
				}
			}
			else if(NodeIsNotTransient(TunnelNode))
			{
				// Editing a composite node, hit the single instance in the parent graph		
				EditCompositeTunnelNode(TunnelNode);
			}
		}
		else if (UK2Node_FunctionTerminator* FunctionDefNode = Cast<UK2Node_FunctionTerminator>(BlueprintGraphActionDetails.GetFunctionEntryNode().Get()))
		{
			// Reconstruct all function call sites that call this function (in open blueprints)
			for (TObjectIterator<UK2Node_CallFunction> It(RF_Transient); It; ++It)
			{
				UK2Node_CallFunction* CallSite = *It;
				if (NodeIsNotTransient(CallSite))
				{
					UBlueprint* CallSiteBlueprint = FBlueprintEditorUtils::FindBlueprintForNode(CallSite);

					const bool bNameMatches = (CallSite->FunctionReference.GetMemberName() == FunctionDefNode->SignatureName);
					const UClass* MemberParentClass = CallSite->FunctionReference.GetMemberParentClass(CallSite);
					const bool bClassMatchesEasy = (MemberParentClass != NULL) && (MemberParentClass->IsChildOf(FunctionDefNode->SignatureClass));
					const bool bClassMatchesHard = (CallSiteBlueprint != NULL) && (CallSite->FunctionReference.IsSelfContext()) && (FunctionDefNode->SignatureClass == NULL) && (CallSiteBlueprint == BlueprintGraphActionDetails.GetBlueprintObj());
					const bool bValidSchema = CallSite->GetSchema() != NULL;

					if (bNameMatches && bValidSchema && (bClassMatchesEasy || bClassMatchesHard))
					{
						EditCallSite(CallSite, CallSiteBlueprint);
					}
				}
			}

			if(FBlueprintEditorUtils::IsDelegateSignatureGraph(Graph))
			{
				FName GraphName = Graph->GetFName();
				for (TObjectIterator<UK2Node_BaseMCDelegate> It(RF_Transient); It; ++It)
				{
					if(NodeIsNotTransient(*It) && (GraphName == It->GetPropertyName()))
					{
						UBlueprint* CallSiteBlueprint = FBlueprintEditorUtils::FindBlueprintForNode(*It);
						EditDelegates(*It, CallSiteBlueprint);
					}
				}
			}

			for (TObjectIterator<UK2Node_CreateDelegate> It(RF_Transient); It; ++It)
			{
				if(NodeIsNotTransient(*It))
				{
					EditCreateDelegates(*It);
				}
			}
		}
	}
};

struct FParamsChangedHelper : public FBasePinChangeHelper
{
	TSet<UBlueprint*> ModifiedBlueprints;
	TSet<UEdGraph*> ModifiedGraphs;

	virtual void EditCompositeTunnelNode(UK2Node_Tunnel* TunnelNode) OVERRIDE
	{
		if (TunnelNode->InputSinkNode != NULL)
		{
			TunnelNode->InputSinkNode->ReconstructNode();
		}

		if (TunnelNode->OutputSourceNode != NULL)
		{
			TunnelNode->OutputSourceNode->ReconstructNode();
		}
	}

	virtual void EditMacroInstance(UK2Node_MacroInstance* MacroInstance, UBlueprint* Blueprint) OVERRIDE
	{
		MacroInstance->ReconstructNode();
		if (Blueprint)
		{
			ModifiedBlueprints.Add(Blueprint);
		}
	}

	virtual void EditCallSite(UK2Node_CallFunction* CallSite, UBlueprint* Blueprint) OVERRIDE
	{
		CallSite->Modify();
		CallSite->ReconstructNode();
		if (Blueprint != NULL)
		{
			ModifiedBlueprints.Add(Blueprint);
		}
	}

	virtual void EditDelegates(UK2Node_BaseMCDelegate* CallSite, UBlueprint* Blueprint) OVERRIDE
	{
		CallSite->Modify();
		CallSite->ReconstructNode();
		if (auto AssignNode = Cast<UK2Node_AddDelegate>(CallSite))
		{
			if (auto DelegateInPin = AssignNode->GetDelegatePin())
			{
				for(auto DelegateOutPinIt = DelegateInPin->LinkedTo.CreateIterator(); DelegateOutPinIt; ++DelegateOutPinIt)
				{
					UEdGraphPin* DelegateOutPin = *DelegateOutPinIt;
					if (auto CustomEventNode = (DelegateOutPin ? Cast<UK2Node_CustomEvent>(DelegateOutPin->GetOwningNode()) : NULL))
					{
						CustomEventNode->ReconstructNode();
					}
				}
			}
		}
		if (Blueprint != NULL)
		{
			ModifiedBlueprints.Add(Blueprint);
		}
	}

	virtual void EditCreateDelegates(UK2Node_CreateDelegate* CallSite) OVERRIDE
	{
		UBlueprint* Blueprint = NULL;
		UEdGraph* Graph = NULL;
		CallSite->HandleAnyChange(Graph, Blueprint);
		if(Blueprint)
		{
			ModifiedBlueprints.Add(Blueprint);
		}
		if(Graph)
		{
			ModifiedGraphs.Add(Graph);
		}
	}

};

void FBaseBlueprintGraphActionDetails::OnParamsChanged(UK2Node_EditablePinBase* TargetNode, bool bForceRefresh)
{
	UEdGraph* Graph = GetGraph();

	// TargetNode can be null, if we just removed the result node because there are no more out params
	if (TargetNode)
	{
		RegenerateInputsChildrenDelegate.ExecuteIfBound();
		RegenerateOutputsChildrenDelegate.ExecuteIfBound();

		// Reconstruct the entry/exit definition and recompile the blueprint to make sure the signature has changed before any fixups
		TargetNode->ReconstructNode();
		FParamsChangedHelper ParamsChangedHelper;
		ParamsChangedHelper.ModifiedBlueprints.Add(GetBlueprintObj());
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprintObj());

		ParamsChangedHelper.Broadcast(TargetNode, *this, Graph);

		for (auto GraphIt = ParamsChangedHelper.ModifiedGraphs.CreateIterator(); GraphIt; ++GraphIt)
		{
			if(UEdGraph* ModifiedGraph = *GraphIt)
			{
				ModifiedGraph->NotifyGraphChanged();
			}
		}

		// Now update all the blueprints that got modified
		for (auto BlueprintIt = ParamsChangedHelper.ModifiedBlueprints.CreateIterator(); BlueprintIt; ++BlueprintIt)
		{
			if(UBlueprint* Blueprint = *BlueprintIt)
			{
				FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
				Blueprint->BroadcastChanged();
			}
		}
	}
}

struct FPinRenamedHelper : public FBasePinChangeHelper
{
	TSet<UBlueprint*> ModifiedBlueprints;
	TSet<UK2Node*> NodesToRename;

	virtual void EditMacroInstance(UK2Node_MacroInstance* MacroInstance, UBlueprint* Blueprint) OVERRIDE
	{
		NodesToRename.Add(MacroInstance);
		if (Blueprint)
		{
			ModifiedBlueprints.Add(Blueprint);
		}
	}

	virtual void EditCallSite(UK2Node_CallFunction* CallSite, UBlueprint* Blueprint) OVERRIDE
	{
		NodesToRename.Add(CallSite);
		if (Blueprint)
		{
			ModifiedBlueprints.Add(Blueprint);
		}
	}
};

bool FBaseBlueprintGraphActionDetails::OnPinRenamed(UK2Node_EditablePinBase* TargetNode, const FString& OldName, const FString& NewName)
{
	UEdGraph* Graph = GetGraph();

	if (TargetNode)
	{
		FPinRenamedHelper PinRenamedHelper;

		if( Graph )
		{
			for( TArray<UEdGraphNode*>::TConstIterator NodeIt(Graph->Nodes); NodeIt; ++NodeIt )
			{
				UK2Node_EditablePinBase* PinNode = Cast<UK2Node_EditablePinBase>(*NodeIt);
				if( PinNode )
				{
					PinRenamedHelper.NodesToRename.Add(PinNode);
				}
			}
			check( PinRenamedHelper.NodesToRename.Find(TargetNode) );
		}
		else
		{
			PinRenamedHelper.NodesToRename.Add(TargetNode);
		}

		PinRenamedHelper.ModifiedBlueprints.Add(GetBlueprintObj());

		// GATHER 
		PinRenamedHelper.Broadcast(TargetNode, *this, Graph);

		// TEST
		for(auto NodeIter = PinRenamedHelper.NodesToRename.CreateIterator(); NodeIter; ++NodeIter)
		{
			if(ERenamePinResult::ERenamePinResult_NameCollision == (*NodeIter)->RenameUserDefinedPin(OldName, NewName, true))
			{
				// log 
				return false;
			}
		}

		// UPDATE
		for(auto NodeIter = PinRenamedHelper.NodesToRename.CreateIterator(); NodeIter; ++NodeIter)
		{
			(*NodeIter)->RenameUserDefinedPin(OldName, NewName, false);
		}

		for (auto BlueprintIt = PinRenamedHelper.ModifiedBlueprints.CreateIterator(); BlueprintIt; ++BlueprintIt)
		{
			FBlueprintEditorUtils::MarkBlueprintAsModified(*BlueprintIt);
		}
	}
	return true;
}

void FBlueprintGraphActionDetails::SetEntryAndResultNodes()
{
	// Clear the entry and exit nodes to the graph
	FunctionEntryNodePtr = NULL;
	FunctionResultNodePtr = NULL;

	if (UEdGraph* NewTargetGraph = GetGraph())
	{
		// There are a few different potential configurations for editable graphs (FunctionEntry/Result, Tunnel Pairs, etc).
		// Step through each case until we find one that matches what appears to be in the graph.  This could be improved if
		// we want to add more robust typing to the graphs themselves

		// Case 1:  Function Entry / Result Pair ------------------
		TArray<UK2Node_FunctionEntry*> EntryNodes;
		NewTargetGraph->GetNodesOfClass(EntryNodes);

		if (EntryNodes.Num() > 0)
		{
			if (EntryNodes[0]->IsEditable())
			{
				FunctionEntryNodePtr = EntryNodes[0];

				// Find a result node
				TArray<UK2Node_FunctionResult*> ResultNodes;
				NewTargetGraph->GetNodesOfClass(ResultNodes);

				check(ResultNodes.Num() <= 1);
				UK2Node_FunctionResult* ResultNode = ResultNodes.Num() ? ResultNodes[0] : NULL;
				// Note:  we assume that if the entry is editable, the result is too (since the entry node is guaranteed to be there on graph creation, but the result isn't)
				if( ResultNode )
				{
					FunctionResultNodePtr = ResultNode;
				}
			}
		}
		else
		{
			// Case 2:  Tunnel Pair -----------------------------------
			TArray<UK2Node_Tunnel*> TunnelNodes;
			NewTargetGraph->GetNodesOfClass(TunnelNodes);

			if (TunnelNodes.Num() > 0)
			{
				// Iterate over the tunnel nodes, and try to find an entry and exit
				for (int32 i = 0; i < TunnelNodes.Num(); i++)
				{
					UK2Node_Tunnel* Node = TunnelNodes[i];
					// Composite nodes should never be considered for function entry / exit, since we're searching for a graph's terminals
					if (Node->IsEditable() && !Node->IsA(UK2Node_Composite::StaticClass()))
					{
						if (Node->bCanHaveOutputs)
						{
							ensure(!FunctionEntryNodePtr.IsValid());
							FunctionEntryNodePtr = Node;
						}
						else if (Node->bCanHaveInputs)
						{
							ensure(!FunctionResultNodePtr.IsValid());
							FunctionResultNodePtr = Node;
						}
					}
				}
			}
		}
	}
	else if (UK2Node_EditablePinBase* Node = GetEditableNode())
	{
		FunctionEntryNodePtr = Node;
	}
}

UEdGraph* FBaseBlueprintGraphActionDetails::GetGraph() const
{
	check(ObjectsBeingEdited.Num() > 0);

	if (ObjectsBeingEdited.Num() == 1)
	{
		UObject* const Object = ObjectsBeingEdited[0].Get();
		if (!Object)
		{
			return nullptr;
		}

		if (Object->IsA<UK2Node_Composite>())
		{
			return Cast<UK2Node_Composite>(Object)->BoundGraph;
		}
		else if (Object->IsA<UK2Node_Tunnel>() || Object->IsA<UK2Node_FunctionTerminator>())
		{
			return Cast<UK2Node>(Object)->GetGraph();
		}
		else if (UK2Node_CallFunction* FunctionCall = Cast<UK2Node_CallFunction>(Object))
		{
			return FindObject<UEdGraph>(FunctionCall->GetBlueprint(), *(FunctionCall->FunctionReference.GetMemberName().ToString()));
		}
		else if (Object->IsA<UEdGraph>())
		{
			return Cast<UEdGraph>(Object);
		}
	}

	return nullptr;
}

UK2Node_EditablePinBase* FBlueprintGraphActionDetails::GetEditableNode() const
{
	check(ObjectsBeingEdited.Num() > 0);

	if (ObjectsBeingEdited.Num() == 1)
	{
		UObject* const Object = ObjectsBeingEdited[0].Get();
		if (!Object)
		{
			return nullptr;
		}

		if (Object->IsA<UK2Node_CustomEvent>())
		{
			return Cast<UK2Node_CustomEvent>(Object);
		}
	}

	return nullptr;
}

UFunction* FBlueprintGraphActionDetails::FindFunction() const
{
	UEdGraph* Graph = GetGraph();
	if(Graph)
	{
		if(UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph))
		{
			UClass* Class = Blueprint->SkeletonGeneratedClass;

			for (TFieldIterator<UFunction> FunctionIt(Class, EFieldIteratorFlags::IncludeSuper); FunctionIt; ++FunctionIt)
			{
				UFunction* Function = *FunctionIt;
				if (Function->GetName() == Graph->GetName())
				{
					return Function;
				}
			}
		}
	}
	return NULL;
}

FKismetUserDeclaredFunctionMetadata* FBlueprintGraphActionDetails::GetMetadataBlock() const
{
	UK2Node_EditablePinBase * FunctionEntryNode = FunctionEntryNodePtr.Get();
	if (UK2Node_FunctionEntry* TypedEntryNode = Cast<UK2Node_FunctionEntry>(FunctionEntryNode))
	{
		return &(TypedEntryNode->MetaData);
	}
	else if (UK2Node_Tunnel* TunnelNode = ExactCast<UK2Node_Tunnel>(FunctionEntryNode))
	{
		// Must be exactly a tunnel, not a macro instance
		return &(TunnelNode->MetaData);
	}

	return NULL;
}

FText FBlueprintGraphActionDetails::OnGetTooltipText() const
{
	if (FKismetUserDeclaredFunctionMetadata* Metadata = GetMetadataBlock())
	{
		return FText::FromString(Metadata->ToolTip);
	}
	else
	{
		return LOCTEXT( "NoTooltip", "(None)" );
	}
}

void FBlueprintGraphActionDetails::OnTooltipTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
{
	if (FKismetUserDeclaredFunctionMetadata* Metadata = GetMetadataBlock())
	{
		Metadata->ToolTip = NewText.ToString();
		if(auto Function = FindFunction())
		{
			Function->Modify();
			Function->SetMetaData(FBlueprintMetadata::MD_Tooltip, *NewText.ToString());
		}
	}
}

FText FBlueprintGraphActionDetails::OnGetCategoryText() const
{
	if (FKismetUserDeclaredFunctionMetadata* Metadata = GetMetadataBlock())
	{
		return FText::FromString(Metadata->Category);
	}
	else
	{
		return LOCTEXT( "NoFunctionCategory", "(None)" );
	}
}

void FBlueprintGraphActionDetails::OnCategoryTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
{
	if (FKismetUserDeclaredFunctionMetadata* Metadata = GetMetadataBlock())
	{
		Metadata->Category = NewText.ToString();
		if(auto Function = FindFunction())
		{
			Function->Modify();
			Function->SetMetaData(FBlueprintMetadata::MD_FunctionCategory, *NewText.ToString());
		}
	}
}

FText FBlueprintGraphActionDetails::AccessSpecifierProperName( uint32 AccessSpecifierFlag ) const
{
	switch(AccessSpecifierFlag)
	{
	case FUNC_Public:
		return LOCTEXT( "Public", "Public" );
	case FUNC_Private:
		return LOCTEXT( "Private", "Private" );
	case FUNC_Protected:
		return LOCTEXT( "Protected", "Protected" );
	case 0:
		return LOCTEXT( "Unknown", "Unknown" ); // Default?
	}
	return LOCTEXT( "Error", "Error" );
}

FText FBlueprintGraphActionDetails::ReplicationSpecifierProperName( uint32 ReplicationSpecifierFlag ) const
{
	switch(ReplicationSpecifierFlag)
	{
	case FUNC_NetMulticast:
		return LOCTEXT( "MulticastDropDown", "Multicast" );
	case FUNC_NetServer:
		return LOCTEXT( "ServerDropDown", "Run on Server" );
	case FUNC_NetClient:
		return LOCTEXT( "ClientDropDown", "Run on owning Client" );
	case 0:
		return LOCTEXT( "NotReplicatedDropDown", "Not Replicated" );
	}
	return LOCTEXT( "Error", "Error" );
}

TSharedRef<ITableRow> FBlueprintGraphActionDetails::HandleGenerateRowAccessSpecifier( TSharedPtr<FAccessSpecifierLabel> SpecifierName, const TSharedRef<STableViewBase>& OwnerTable )
{
	return SNew(STableRow< TSharedPtr<FAccessSpecifierLabel> >, OwnerTable)
		.Content()
		[
			SNew( STextBlock ) 
				.Text( SpecifierName.IsValid() ? SpecifierName->LocalizedName.ToString() : FString() )
		];
}

FString FBlueprintGraphActionDetails::GetCurrentAccessSpecifierName() const
{
	uint32 AccessSpecifierFlag = 0;
	UK2Node_EditablePinBase * FunctionEntryNode = FunctionEntryNodePtr.Get();
	if(UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(FunctionEntryNode))
	{
		AccessSpecifierFlag = FUNC_AccessSpecifiers & EntryNode->ExtraFlags;
	}
	else if(UK2Node_CustomEvent* CustomEventNode = Cast<UK2Node_CustomEvent>(FunctionEntryNode))
	{
		AccessSpecifierFlag = FUNC_AccessSpecifiers & CustomEventNode->FunctionFlags;
	}
	return AccessSpecifierProperName( AccessSpecifierFlag ).ToString();
}

bool FBlueprintGraphActionDetails::IsAccessSpecifierVisible() const
{
	bool bSupportedType = false;
	bool bIsEditable = false;
	UK2Node_EditablePinBase * FunctionEntryNode = FunctionEntryNodePtr.Get();
	if(FunctionEntryNode)
	{
		UBlueprint* Blueprint = FunctionEntryNode->GetBlueprint();
		const bool bIsInterface = FBlueprintEditorUtils::IsInterfaceBlueprint(Blueprint);

		bSupportedType = !bIsInterface && (FunctionEntryNode->IsA<UK2Node_FunctionEntry>() || FunctionEntryNode->IsA<UK2Node_Event>());
		bIsEditable = FunctionEntryNode->IsEditable();
	}
	return bSupportedType && bIsEditable;
}

void FBlueprintGraphActionDetails::OnAccessSpecifierSelected( TSharedPtr<FAccessSpecifierLabel> SpecifierName, ESelectInfo::Type SelectInfo )
{
	if(AccessSpecifierComboButton.IsValid())
	{
		AccessSpecifierComboButton->SetIsOpen(false);
	}

	UK2Node_EditablePinBase* FunctionEntryNode = FunctionEntryNodePtr.Get();
	if(FunctionEntryNode && SpecifierName.IsValid())
	{
		const FScopedTransaction Transaction( LOCTEXT( "ChangeAccessSpecifier", "Change Access Specifier" ) );

		FunctionEntryNode->Modify();
		auto Function = FindFunction();
		if(Function)
		{
			Function->Modify();
		}

		const uint32 ClearAccessSpecifierMask = ~FUNC_AccessSpecifiers;
		if(UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(FunctionEntryNode))
		{
			EntryNode->ExtraFlags &= ClearAccessSpecifierMask;
			EntryNode->ExtraFlags |= SpecifierName->SpecifierFlag;
		}
		else if(UK2Node_Event* EventNode = Cast<UK2Node_Event>(FunctionEntryNode))
		{
			EventNode->FunctionFlags &= ClearAccessSpecifierMask;
			EventNode->FunctionFlags |= SpecifierName->SpecifierFlag;
		}
		if(Function)
		{
			Function->FunctionFlags &= ClearAccessSpecifierMask;
			Function->FunctionFlags |= SpecifierName->SpecifierFlag;
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprintObj());
	}
}

bool FBlueprintGraphActionDetails::GetInstanceColorVisibility() const
{
	// Hide the color editor if it's a top level function declaration.
	// Show it if we're editing a collapsed graph or macro
	UEdGraph* Graph = GetGraph();
	if (Graph)
	{
		const UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
		if (Blueprint)
		{
			const bool bIsTopLevelFunctionGraph = Blueprint->FunctionGraphs.Contains(Graph);
			const bool bIsTopLevelMacroGraph = Blueprint->MacroGraphs.Contains(Graph);
			const bool bIsMacroGraph = Blueprint->BlueprintType == BPTYPE_MacroLibrary;
			return ((bIsMacroGraph || bIsTopLevelMacroGraph) || !bIsTopLevelFunctionGraph);
		}

	}
	
	return false;
}

FLinearColor FBlueprintGraphActionDetails::GetNodeTitleColor() const
{
	if (FKismetUserDeclaredFunctionMetadata* Metadata = GetMetadataBlock())
	{
		return Metadata->InstanceTitleColor;
	}
	else
	{
		return FLinearColor::White;
	}
}

FReply FBlueprintGraphActionDetails::ColorBlock_OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (FKismetUserDeclaredFunctionMetadata* Metadata = GetMetadataBlock())
		{
			TArray<FLinearColor*> LinearColorArray;
			LinearColorArray.Add(&(Metadata->InstanceTitleColor));

			FColorPickerArgs PickerArgs;
			PickerArgs.bIsModal = true;
			PickerArgs.ParentWidget = ColorBlock;
			PickerArgs.DisplayGamma = TAttribute<float>::Create( TAttribute<float>::FGetter::CreateUObject(GEngine, &UEngine::GetDisplayGamma) );
			PickerArgs.LinearColorArray = &LinearColorArray;

			OpenColorPicker(PickerArgs);
		}

		return FReply::Handled();
	}
	else
	{
		return FReply::Unhandled();
	}
}

bool FBlueprintGraphActionDetails::IsCustomEvent() const
{
	return (NULL != Cast<UK2Node_CustomEvent>(FunctionEntryNodePtr.Get()));
}

void FBlueprintGraphActionDetails::OnIsReliableReplicationFunctionModified(const ESlateCheckBoxState::Type NewCheckedState)
{
	UK2Node_EditablePinBase * FunctionEntryNode = FunctionEntryNodePtr.Get();
	UK2Node_CustomEvent* CustomEvent = Cast<UK2Node_CustomEvent>(FunctionEntryNode);
	if( CustomEvent )
	{
		if (NewCheckedState == ESlateCheckBoxState::Checked)
		{
			if (UK2Node_FunctionEntry* TypedEntryNode = Cast<UK2Node_FunctionEntry>(FunctionEntryNode))
			{
				TypedEntryNode->ExtraFlags |= FUNC_NetReliable;
			}
			if (UK2Node_CustomEvent * CustomEventNode = Cast<UK2Node_CustomEvent>(FunctionEntryNode))
			{
				CustomEventNode->FunctionFlags |= FUNC_NetReliable;
			}
		}
		else
		{
			if (UK2Node_FunctionEntry* TypedEntryNode = Cast<UK2Node_FunctionEntry>(FunctionEntryNode))
			{
				TypedEntryNode->ExtraFlags &= ~FUNC_NetReliable;
			}
			if (UK2Node_CustomEvent * CustomEventNode = Cast<UK2Node_CustomEvent>(FunctionEntryNode))
			{
				CustomEventNode->FunctionFlags &= ~FUNC_NetReliable;
			}
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprintObj());
	}
}

ESlateCheckBoxState::Type FBlueprintGraphActionDetails::GetIsReliableReplicatedFunction() const
{
	const UK2Node_EditablePinBase * FunctionEntryNode = FunctionEntryNodePtr.Get();
	const UK2Node_CustomEvent* CustomEvent = Cast<const UK2Node_CustomEvent>(FunctionEntryNode);
	if(!CustomEvent)
	{
		ESlateCheckBoxState::Undetermined;
	}

	if (((CustomEvent->FunctionFlags & FUNC_Net) != 0) && ((CustomEvent->FunctionFlags & FUNC_NetReliable) != 0))
	{
		return ESlateCheckBoxState::Checked;
	}
	
	return ESlateCheckBoxState::Unchecked;
}

bool FBlueprintGraphActionDetails::GetIsReplicatedFunction() const
{
	const UK2Node_EditablePinBase * FunctionEntryNode = FunctionEntryNodePtr.Get();
	const UK2Node_CustomEvent* CustomEvent = Cast<const UK2Node_CustomEvent>(FunctionEntryNode);
	if(!CustomEvent)
	{
		false;
	}

	if (((CustomEvent->FunctionFlags & FUNC_Net) != 0))
	{
		return true;
	}

	return false;
}

bool FBlueprintGraphActionDetails::IsPureFunctionVisible() const
{
	bool bSupportedType = false;
	bool bIsEditable = false;
	UK2Node_EditablePinBase * FunctionEntryNode = FunctionEntryNodePtr.Get();
	if(FunctionEntryNode)
	{
		UBlueprint* Blueprint = FunctionEntryNode->GetBlueprint();
		const bool bIsInterface = FBlueprintEditorUtils::IsInterfaceBlueprint(Blueprint);

		bSupportedType = !bIsInterface && FunctionEntryNode->IsA<UK2Node_FunctionEntry>();
		bIsEditable = FunctionEntryNode->IsEditable();
	}
	return bSupportedType && bIsEditable;
}

void FBlueprintGraphActionDetails::OnIsPureFunctionModified( const ESlateCheckBoxState::Type NewCheckedState )
{
	UK2Node_EditablePinBase * FunctionEntryNode = FunctionEntryNodePtr.Get();
	auto Function = FindFunction();
	auto EntryNode = Cast<UK2Node_FunctionEntry>(FunctionEntryNode);
	if(EntryNode && Function)
	{
		const FScopedTransaction Transaction( LOCTEXT( "ChangePure", "Change Pure" ) );
		EntryNode->Modify();
		Function->Modify();

		//set flags on function entry node also
		EntryNode->ExtraFlags	^= FUNC_BlueprintPure;
		Function->FunctionFlags ^= FUNC_BlueprintPure;
		OnParamsChanged(FunctionEntryNode);
	}
}

ESlateCheckBoxState::Type FBlueprintGraphActionDetails::GetIsPureFunction() const
{
	UK2Node_EditablePinBase * FunctionEntryNode = FunctionEntryNodePtr.Get();
	auto EntryNode = Cast<UK2Node_FunctionEntry>(FunctionEntryNode);
	if(!EntryNode)
	{
		return ESlateCheckBoxState::Undetermined;
	}
	return (EntryNode->ExtraFlags & FUNC_BlueprintPure) ? ESlateCheckBoxState::Checked :  ESlateCheckBoxState::Unchecked;
}

bool FBaseBlueprintGraphActionDetails::IsPinNameUnique(const FString& TestName) const
{
	UK2Node_EditablePinBase * FunctionEntryNode = FunctionEntryNodePtr.Get();
	UK2Node_EditablePinBase * FunctionResultNode = FunctionResultNodePtr.Get();

	// Check the built in pins
 	for( TArray<UEdGraphPin*>::TIterator it(FunctionEntryNode->Pins); it; ++it )
 	{
		if( (*it)->PinName == TestName )
 		{
 			return false;
 		}
 	}

	// If there is a result node, that isn't the same as the entry node, check it as well
	if( FunctionResultNode && (FunctionResultNode != FunctionEntryNode) )
	{
		for( TArray<UEdGraphPin*>::TIterator it(FunctionResultNode->Pins); it; ++it )
		{
			if( (*it)->PinName == TestName )
			{
				return false;
			}
		}
	}

	return true;
}

void FBaseBlueprintGraphActionDetails::GenerateUniqueParameterName( const FString &BaseName, FString &Result ) const
{
	Result = BaseName;
	int UniqueNum = 0;
	while(!IsPinNameUnique(Result))
	{
		Result = FString::Printf(TEXT("%s%d"), *BaseName, ++UniqueNum);
	}
}

FReply FBaseBlueprintGraphActionDetails::OnAddNewInputClicked()
{
	UK2Node_EditablePinBase * FunctionEntryNode = FunctionEntryNodePtr.Get();

	if( FunctionEntryNode )
	{
		const FScopedTransaction Transaction( LOCTEXT( "AddInParam", "Add In Parameter" ) );
		FunctionEntryNode->Modify();

		FEdGraphPinType PinType = MyBlueprint.Pin()->GetLastFunctionPinTypeUsed();

		// Make sure that if this is an exec node we are allowed one.
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
		if ((PinType.PinCategory == Schema->PC_Exec) && (!FunctionEntryNode->CanModifyExecutionWires()))
		{
			MyBlueprint.Pin()->ResetLastPinType();
			PinType = MyBlueprint.Pin()->GetLastFunctionPinTypeUsed();
		}
		FString NewPinName;
		GenerateUniqueParameterName(TEXT("NewParam"), NewPinName);
		FunctionEntryNode->CreateUserDefinedPin(NewPinName, PinType);

		OnParamsChanged(FunctionEntryNode, true);
	}

	return FReply::Handled();
}

FReply FBlueprintGraphActionDetails::OnAddNewOutputClicked()
{
	FScopedTransaction Transaction( LOCTEXT( "AddOutParam", "Add Out Parameter" ) );
	
	GetBlueprintObj()->Modify();
	GetGraph()->Modify();
	UK2Node_EditablePinBase* EntryPin = FunctionEntryNodePtr.Get();	
	EntryPin->Modify();
	for (int32 iPin = 0; iPin < EntryPin->Pins.Num() ; iPin++)
	{
		EntryPin->Pins[iPin]->Modify();
	}
	
	UK2Node_EditablePinBase* PreviousResultNode = FunctionResultNodePtr.Get();

	AttemptToCreateResultNode();

	UK2Node_EditablePinBase * FunctionResultNode = FunctionResultNodePtr.Get();
	if( FunctionResultNode )
	{
		FunctionResultNode->Modify();
		FEdGraphPinType PinType = MyBlueprint.Pin()->GetLastFunctionPinTypeUsed();

		// Make sure that if this is an exec node we are allowed one.
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
		if ((PinType.PinCategory == Schema->PC_Exec) && (!FunctionResultNode->CanModifyExecutionWires()))
		{
			MyBlueprint.Pin()->ResetLastPinType();
			PinType = MyBlueprint.Pin()->GetLastFunctionPinTypeUsed();
		}
		FString NewPinName;
		GenerateUniqueParameterName(TEXT("NewParam"), NewPinName);
		FunctionResultNode->CreateUserDefinedPin(NewPinName, PinType);
		
		OnParamsChanged(FunctionResultNode, true);
	}
	else
	{
		Transaction.Cancel();
	}

	return FReply::Handled();
}



void FBlueprintInterfaceLayout::GenerateHeaderRowContent( FDetailWidgetRow& NodeRow )
{
	NodeRow
	[
		SNew(STextBlock)
			.Text( bShowsInheritedInterfaces ?
			LOCTEXT("BlueprintInheritedInterfaceTitle", "Inherited Interfaces").ToString() :
			LOCTEXT("BlueprintImplementedInterfaceTitle", "Implemented Interfaces").ToString() )
			.Font( IDetailLayoutBuilder::GetDetailFont() )
	];
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FBlueprintInterfaceLayout::GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder )
{
	UBlueprint* Blueprint = GlobalOptionsDetailsPtr.Pin()->GetBlueprintObj();
	check(Blueprint);

	TArray<FString> Interfaces;

	if (!bShowsInheritedInterfaces)
	{
		// Generate a list of interfaces already implemented
		for (TArray<FBPInterfaceDescription>::TConstIterator It(Blueprint->ImplementedInterfaces); It; ++It)
		{
			Interfaces.AddUnique( (*It).Interface->GetName() );
		}
	}
	else
	{
		// Generate a list of interfaces implemented by classes this blueprint inherited from
		UClass* BlueprintParent = Blueprint->ParentClass;
		while (BlueprintParent)
		{
			for (TArray<FImplementedInterface>::TIterator It(BlueprintParent->Interfaces); It; ++It)
			{
				FImplementedInterface& CurrentInterface = *It;
				if( CurrentInterface.Class )
				{
					Interfaces.Add( CurrentInterface.Class->GetName() );
				}
			}
			BlueprintParent = BlueprintParent->GetSuperClass();
		}
	}

	for (int32 i = 0; i < Interfaces.Num(); ++i)
	{
		TSharedPtr<SHorizontalBox> Box;
		ChildrenBuilder.AddChildContent( *LOCTEXT( "BlueprintInterfaceValue", "Interface Value" ).ToString() )
		[
			SAssignNew(Box, SHorizontalBox)
			+SHorizontalBox::Slot()
			[
				SNew(STextBlock)
					.Text(Interfaces[i])
					.Font( IDetailLayoutBuilder::GetDetailFont() )
			]
		];

		// See if we need to add a button for opening this interface
		if (!bShowsInheritedInterfaces)
		{
			UBlueprintGeneratedClass* Class = Cast<UBlueprintGeneratedClass>(*Blueprint->ImplementedInterfaces[i].Interface);
			if (Class)
			{
				TWeakObjectPtr<UObject> Asset = Class->ClassGeneratedBy;
		
				const TSharedRef<SWidget> BrowseButton = PropertyCustomizationHelpers::MakeBrowseButton(FSimpleDelegate::CreateSP(this, &FBlueprintInterfaceLayout::OnBrowseToInterface, Asset));
				BrowseButton->SetToolTipText( LOCTEXT("BlueprintInterfaceBrowseTooltip", "Opens this interface") );

				Box->AddSlot()
				.AutoWidth()
				.Padding(2.0f, 0.0f)
				[
					BrowseButton
				];
			}
		}

		if (!bShowsInheritedInterfaces)
		{
			Box->AddSlot()
			.AutoWidth()
			[
				PropertyCustomizationHelpers::MakeClearButton(FSimpleDelegate::CreateSP(this, &FBlueprintInterfaceLayout::OnRemoveInterface, Interfaces[i]))
			];
		}
	}

	// Add message if no interfaces are being used
	if (Interfaces.Num() == 0)
	{
		ChildrenBuilder.AddChildContent(*LOCTEXT("BlueprintInterfaceValue", "Interface Value").ToString())
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoBlueprintInterface", "No Interfaces"))
			.Font(IDetailLayoutBuilder::GetDetailFontItalic())
		];
	}

	if (!bShowsInheritedInterfaces)
	{
		ChildrenBuilder.AddChildContent( *LOCTEXT( "BlueprintAddInterface", "Add Interface" ).ToString() )
		[
			SNew(SBox)
			.HAlign(HAlign_Right)
			[
				SAssignNew(AddInterfaceComboButton, SComboButton)
				.ButtonContent()
				[
					SNew(STextBlock)
						.Text(LOCTEXT("BlueprintAddInterfaceButton", "Add"))
				]
				.OnGetMenuContent(this, &FBlueprintInterfaceLayout::OnGetAddInterfaceMenuContent)
			]
		];
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FBlueprintInterfaceLayout::OnBrowseToInterface(TWeakObjectPtr<UObject> Asset)
{
	if (Asset.IsValid())
	{
		FAssetEditorManager::Get().OpenEditorForAsset(Asset.Get());
	}
}

void FBlueprintInterfaceLayout::OnRemoveInterface(FString InterfaceName)
{
	UBlueprint* Blueprint = GlobalOptionsDetailsPtr.Pin()->GetBlueprintObj();
	check(Blueprint);

	const FName InterfaceFName = *InterfaceName;

	// Close all graphs that are about to be removed
	TArray<UEdGraph*> Graphs;
	FBlueprintEditorUtils::GetInterfaceGraphs(Blueprint, InterfaceFName, Graphs);
	for( TArray<UEdGraph*>::TIterator GraphIt(Graphs); GraphIt; ++GraphIt )
	{
		GlobalOptionsDetailsPtr.Pin()->GetBlueprintEditorPtr().Pin()->CloseDocumentTab(*GraphIt);
	}

	const bool bPreserveInterfaceFunctions = (EAppReturnType::Yes == FMessageDialog::Open( EAppMsgType::YesNo, NSLOCTEXT("UnrealEd", "TransferInterfaceFunctionsToBlueprint", "Would you like to transfer the interface functions to be part of your blueprint?") ));

	// Do the work of actually removing the interface
	FBlueprintEditorUtils::RemoveInterface(Blueprint, InterfaceFName, bPreserveInterfaceFunctions);

	RegenerateChildrenDelegate.ExecuteIfBound();
}

/** Helper function for the interface menu */
bool IsInterfaceImplemented(const UBlueprint* Blueprint, const UClass* TestInterface)
{
	const FString InterfaceName = TestInterface->GetName();

	// First look in the blueprint's ImplementedInterfaces list
	for(TArray<FBPInterfaceDescription>::TConstIterator it(Blueprint->ImplementedInterfaces); it; ++it)
	{
		const FBPInterfaceDescription& CurrentInterface = *it;
		if( CurrentInterface.Interface && CurrentInterface.Interface->GetName() == InterfaceName )
		{
			return true;
		}
	}

	// Now go up the inheritance tree and see if the interface in implemented via its' ancestry
	UClass* BlueprintParent =  Blueprint->ParentClass;
	while (BlueprintParent)
	{
		for (TArray<FImplementedInterface>::TIterator It(BlueprintParent->Interfaces); It; ++It)
		{
			const FImplementedInterface& CurrentInterface = *It;
			if (CurrentInterface.Class && (CurrentInterface.Class->GetName() == InterfaceName))
			{
				return true;
			}
		}
		BlueprintParent = BlueprintParent->GetSuperClass();
	}

	return false;
}

TSharedRef<SWidget> FBlueprintInterfaceLayout::OnGetAddInterfaceMenuContent()
{
	UBlueprint* Blueprint = GlobalOptionsDetailsPtr.Pin()->GetBlueprintObj();

	// Generate a list of interfaces that can still be implemented
	UnimplementedInterfaces.Empty();
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* CurrentInterface = *It;
		if (FKismetEditorUtilities::CanBlueprintImplementInterface(Blueprint, CurrentInterface) &&
			!IsInterfaceImplemented(Blueprint, CurrentInterface) &&
			!FKismetEditorUtilities::IsClassABlueprintSkeleton(CurrentInterface))
		{
			UnimplementedInterfaces.Add( MakeShareable( new FString(CurrentInterface->GetName()) ));
		}
	}

	if (UnimplementedInterfaces.Num() > 0)
	{
		return SNew(SListView<TSharedPtr<FString>>)
			.ListItemsSource(&UnimplementedInterfaces)
			.OnGenerateRow(this, &FBlueprintInterfaceLayout::GenerateInterfaceListRow)
			.OnSelectionChanged(this, &FBlueprintInterfaceLayout::OnInterfaceListSelectionChanged);
	}
	else
	{
		return SNew(STextBlock)
			.Text(LOCTEXT("BlueprintNoInterfacesAvailable", "No interfaces available to implement."));
	}
}

TSharedRef<ITableRow> FBlueprintInterfaceLayout::GenerateInterfaceListRow( TSharedPtr<FString> InterfaceName, const TSharedRef<STableViewBase>& OwningList )
{
	return SNew(STableRow< TSharedPtr<FString> >, OwningList)
		[
			SNew(STextBlock)
			.Text(*InterfaceName)
		];
}

void FBlueprintInterfaceLayout::OnInterfaceListSelectionChanged(TSharedPtr<FString> Selection, ESelectInfo::Type SelectInfo)
{
	if (Selection.IsValid())
	{
		UBlueprint* Blueprint = GlobalOptionsDetailsPtr.Pin()->GetBlueprintObj();
		check(Blueprint);

		FName SelectedInterface = FName(*(*Selection.Get()));
		if (!FBlueprintEditorUtils::ImplementNewInterface( Blueprint, SelectedInterface ) )
		{
			GlobalOptionsDetailsPtr.Pin()->GetBlueprintEditorPtr().Pin()->LogSimpleMessage( LOCTEXT("ImplementInterface_Error", "Unable to implement interface. Check log for details") );
		}
		
		if (AddInterfaceComboButton.IsValid())
		{
			AddInterfaceComboButton->SetIsOpen(false);
		}

		RegenerateChildrenDelegate.ExecuteIfBound();
	}
}



UBlueprint* FBlueprintGlobalOptionsDetails::GetBlueprintObj() const
{
	if(BlueprintEditorPtr.IsValid())
	{
		return BlueprintEditorPtr.Pin()->GetBlueprintObj();
	}

	return NULL;
}

FString FBlueprintGlobalOptionsDetails::GetParentClassName() const
{
	FName ParentClassName = NAME_None;
	const UBlueprint* Blueprint = GetBlueprintObj();
	if(Blueprint != NULL && Blueprint->ParentClass != NULL)
	{
		ParentClassName = Blueprint->ParentClass->GetFName();
	}

	return ParentClassName.ToString();
}

bool FBlueprintGlobalOptionsDetails::CanReparent() const
{
	return BlueprintEditorPtr.IsValid() && BlueprintEditorPtr.Pin()->ReparentBlueprint_IsVisible();
}

TSharedRef<SWidget> FBlueprintGlobalOptionsDetails::GetParentClassMenuContent()
{
	TArray<UBlueprint*> Blueprints;
	Blueprints.Add(GetBlueprintObj());
	TSharedRef<SWidget> ClassPicker = FBlueprintEditorUtils::ConstructBlueprintParentClassPicker(Blueprints, FOnClassPicked::CreateSP(this, &FBlueprintGlobalOptionsDetails::OnClassPicked));

	return
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
		[
			// Achieving fixed width by nesting items within a fixed width box.
			SNew(SBox)
			.WidthOverride(350.0f)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.MaxHeight(400.0f)
				.AutoHeight()
				[
					ClassPicker
				]
			]
		];
}

void FBlueprintGlobalOptionsDetails::OnClassPicked(UClass* PickedClass)
{
	ParentClassComboButton->SetIsOpen(false);
	if(BlueprintEditorPtr.IsValid())
	{
		BlueprintEditorPtr.Pin()->ReparentBlueprint_NewParentChosen(PickedClass);
	}
}

void FBlueprintGlobalOptionsDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	const UBlueprint* Blueprint = GetBlueprintObj();
	if(Blueprint != NULL)
	{
		// Hide any properties that aren't included in the "Option" category
		for (TFieldIterator<UProperty> PropertyIt(Blueprint->GetClass(), EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			UProperty* Property = *PropertyIt;
			if(FObjectEditorUtils::GetCategory(Property) != TEXT("BlueprintOption"))
			{
				DetailLayout.HideProperty(DetailLayout.GetProperty(Property->GetFName()));
			}
		}

		// Display the parent class and set up the menu for reparenting
		IDetailCategoryBuilder& Category = DetailLayout.EditCategory("Globals", LOCTEXT("BlueprintGlobalDetailsCategory", "Globals").ToString());
		Category.AddCustomRow( LOCTEXT("BlueprintGlobalDetailsCategory", "Globals").ToString() )
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BlueprintDetails_ParentClass", "Parent Class").ToString())
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		[
			SAssignNew(ParentClassComboButton, SComboButton)
			.IsEnabled(this, &FBlueprintGlobalOptionsDetails::CanReparent)
			.OnGetMenuContent(this, &FBlueprintGlobalOptionsDetails::GetParentClassMenuContent)
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(this, &FBlueprintGlobalOptionsDetails::GetParentClassName)
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		];
		
		const bool bIsInterfaceBP = FBlueprintEditorUtils::IsInterfaceBlueprint(Blueprint);
		const bool bIsMacroLibrary = Blueprint->BlueprintType == BPTYPE_MacroLibrary;
		const bool bIsLevelScriptBP = FBlueprintEditorUtils::IsLevelScriptBlueprint(Blueprint);
		const bool bSupportsInterfaces = !bIsLevelScriptBP && !bIsInterfaceBP && !bIsMacroLibrary;

		if (bSupportsInterfaces)
		{
			// Interface details customization
			IDetailCategoryBuilder& InterfacesCategory = DetailLayout.EditCategory("Interfaces", LOCTEXT("BlueprintInterfacesDetailsCategory", "Interfaces").ToString());
		
			TSharedRef<FBlueprintInterfaceLayout> InterfaceLayout = MakeShareable(new FBlueprintInterfaceLayout(SharedThis(this), false));
			InterfacesCategory.AddCustomBuilder(InterfaceLayout);
		
			TSharedRef<FBlueprintInterfaceLayout> InheritedInterfaceLayout = MakeShareable(new FBlueprintInterfaceLayout(SharedThis(this), true));
			InterfacesCategory.AddCustomBuilder(InheritedInterfaceLayout);
		}

		// Hide 'run on drag' for LevelBP
		if (bIsLevelScriptBP)
		{
			static FName RunOnDragPropName(TEXT("bRunConstructionScriptOnDrag"));
			DetailLayout.HideProperty(DetailLayout.GetProperty(RunOnDragPropName));
		}
	}
}



void FBlueprintComponentDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	check( BlueprintEditorPtr.IsValid() );
	TSharedPtr<SSCSEditor> Editor = BlueprintEditorPtr.Pin()->GetSCSEditor();
	check( Editor.IsValid() );
	const UBlueprint* Blueprint = GetBlueprintObj();
	check(Blueprint != NULL);

	TArray<FSCSEditorTreeNodePtrType> Nodes = Editor->GetSelectedNodes();
	check(Nodes.Num() > 0);

	if (Nodes.Num() == 1)
	{
		CachedNodePtr = Nodes[0];
	}

	if( CachedNodePtr.IsValid() )
	{
		IDetailCategoryBuilder& VariableCategory = DetailLayout.EditCategory("Variable", LOCTEXT("VariableDetailsCategory", "Variable").ToString(), ECategoryPriority::Variable);

		VariableCategory.AddCustomRow(TEXT("Variable Name"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BlueprintComponentDetails_VariableNameLabel", "Variable Name").ToString())
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		[
			SAssignNew(VariableNameEditableTextBox, SEditableTextBox)
			.Text(this, &FBlueprintComponentDetails::OnGetVariableText)
			.OnTextChanged( this, &FBlueprintComponentDetails::OnVariableTextChanged)
			.OnTextCommitted( this, &FBlueprintComponentDetails::OnVariableTextCommitted)
			.IsReadOnly(CachedNodePtr->IsNative() || CachedNodePtr->IsInherited() || CachedNodePtr->IsDefaultSceneRoot())
			.Font(IDetailLayoutBuilder::GetDetailFont())
		];

		VariableCategory.AddCustomRow(TEXT("Tooltip"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BlueprintComponentDetails_VariableTooltipLabel", "Tooltip").ToString())
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		[
			SNew(SEditableTextBox)
			.Text(this, &FBlueprintComponentDetails::OnGetTooltipText)
			.OnTextCommitted(this, &FBlueprintComponentDetails::OnTooltipTextCommitted, CachedNodePtr->GetVariableName())
			.Font(IDetailLayoutBuilder::GetDetailFont())
		];

		PopulateVariableCategories();
		const FString CategoryTooltip = LOCTEXT("EditCategoryName_Tooltip", "The category of the variable; editing this will place the variable into another category or create a new one.").ToString();

		VariableCategory.AddCustomRow( TEXT("Category") )
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BlueprintComponentDetails_VariableCategoryLabel", "Category").ToString())
			.ToolTipText(CategoryTooltip)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		[
			SAssignNew(VariableCategoryComboButton, SComboButton)
			.ContentPadding(FMargin(0,0,5,0))
			.IsEnabled(this, &FBlueprintComponentDetails::OnVariableCategoryChangeEnabled)
			.ButtonContent()
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("NoBorder"))
				.Padding(FMargin(0, 0, 5, 0))
				[
					SNew(SEditableTextBox)
					.Text(this, &FBlueprintComponentDetails::OnGetVariableCategoryText)
					.OnTextCommitted(this, &FBlueprintComponentDetails::OnVariableCategoryTextCommitted, CachedNodePtr->GetVariableName())
					.ToolTipText(CategoryTooltip)
					.SelectAllTextWhenFocused(true)
					.RevertTextOnEscape(true)
					.Font(IDetailLayoutBuilder::GetDetailFont())
				]
			]
			.MenuContent()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				.MaxHeight(400.0f)
				[
					SAssignNew(VariableCategoryListView, SListView<TSharedPtr<FString>>)
					.ListItemsSource(&VariableCategorySource)
					.OnGenerateRow(this, &FBlueprintComponentDetails::MakeVariableCategoryViewWidget)
					.OnSelectionChanged(this, &FBlueprintComponentDetails::OnVariableCategorySelectionChanged)
				]
			]
		];

		IDetailCategoryBuilder& SocketsCategory = DetailLayout.EditCategory("Sockets", LOCTEXT("BlueprintComponentDetailsCategory", "Sockets").ToString(), ECategoryPriority::Important);

		SocketsCategory.AddCustomRow(LOCTEXT("BlueprintComponentDetails_Sockets", "Sockets").ToString())
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BlueprintComponentDetails_ParentSocket", "Parent Socket").ToString())
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SEditableTextBox)
				.Text(this, &FBlueprintComponentDetails::GetSocketName)
				.IsReadOnly(true)
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 1.0f)
			[
				PropertyCustomizationHelpers::MakeBrowseButton(
					FSimpleDelegate::CreateSP(this, &FBlueprintComponentDetails::OnBrowseSocket), LOCTEXT( "SocketBrowseButtonToolTipText", "Browse available Bones and Sockets")
				)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 1.0f)
			[
				PropertyCustomizationHelpers::MakeClearButton(FSimpleDelegate::CreateSP(this, &FBlueprintComponentDetails::OnClearSocket))
			]
		];
	}
	// Add Events Section if we have a common base class that has valid events
	UClass* CommonEventsClass = FindCommonBaseClassFromSelected();

	if( FBlueprintEditor::CanClassGenerateEvents( CommonEventsClass ))
	{
		IDetailCategoryBuilder& EventCategory = DetailLayout.EditCategory("Component", LOCTEXT("ComponentDetailsCategory", "Events").ToString(), ECategoryPriority::Important);

		FText NodeName = ( Nodes.Num() > 1 ) ? LOCTEXT( "ScriptingEvents_SelectedComponents", "Selected Components" ) : FText::FromName( CachedNodePtr->GetVariableName() );
		const FText AddEventLabel = FText::Format( LOCTEXT( "ScriptingEvents_AddEvent", "Add Event For {0}" ), NodeName );
		const FText AddEventToolTip = FText::Format( LOCTEXT( "ScriptingEvents_AddOrView", "Adds or views events for {0} in the Component Blueprint" ), NodeName );

		EventCategory.AddCustomRow( LOCTEXT("AddEventHeader", "Add Event").ToString())
			.NameContent()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.Padding( FMargin( 2.0f, 0, 0, 0 ) )
				.VAlign( VAlign_Center )
				.AutoWidth()
				[
					SNew( STextBlock )
						.Text( LOCTEXT("ScriptingEvents_Label", "Add Event").ToString()  )
						.Font( IDetailLayoutBuilder::GetDetailFont() )
				]
			]
			.ValueContent()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth( 1 )
				.VAlign( VAlign_Center )
				[
					SNew( SComboButton )
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Center)
						.OnGetMenuContent( this, &FBlueprintComponentDetails::BuildEventsMenuForComponents)
						.ButtonContent()
					[
						SNew( STextBlock )
							.Text( AddEventLabel ) 
							.ToolTipText( AddEventToolTip )
							.Font( IDetailLayoutBuilder::GetDetailFont() )
					]
				]
			];
	}
}

UClass* FBlueprintComponentDetails::FindCommonBaseClassFromSelected() const
{
	UClass* Result = NULL;

	if( BlueprintEditorPtr.IsValid() )
	{
		TSharedPtr<SSCSEditor> Editor = BlueprintEditorPtr.Pin()->GetSCSEditor();
		check( Editor.IsValid() );
		// Find common component class from selected
		TArray<UClass*> SelectionClasses;
		TArray<FSCSEditorTreeNodePtrType> SelectedNodes = Editor->GetSelectedNodes();

		for (auto NodeIter = SelectedNodes.CreateConstIterator(); NodeIter; ++NodeIter)
		{
			auto Node = *NodeIter;
			if( Node.IsValid() && Node->GetComponentTemplate() )
			{
				SelectionClasses.Add( Node->GetComponentTemplate()->GetClass() );
			}
		}
		Result = UClass::FindCommonBase( SelectionClasses );
	}
	return Result;
}

TSharedRef<SWidget> FBlueprintComponentDetails::BuildEventsMenuForComponents() const
{
	// See if the component class has event properties
	if( BlueprintEditorPtr.IsValid() )
	{
		TSharedPtr<SSCSEditor> Editor = BlueprintEditorPtr.Pin()->GetSCSEditor();

		if( Editor.IsValid() )
		{
			UClass* CommonComponentClass = FindCommonBaseClassFromSelected();
			if( CommonComponentClass )
			{
				FMenuBuilder MenuBuilder( true, NULL );
				Editor->BuildMenuEventsSection( MenuBuilder, BlueprintEditorPtr.Pin(), CommonComponentClass, 
												FGetSelectedObjectsDelegate::CreateSP(Editor.Get(), &SSCSEditor::GetSelectedItemsForContextMenu));
				return MenuBuilder.MakeWidget();
			}
		}
	}
	return SNullWidget::NullWidget;
}

FText FBlueprintComponentDetails::OnGetVariableText() const
{
	check(CachedNodePtr.IsValid());

	return FText::FromName(CachedNodePtr->GetVariableName());
}

void FBlueprintComponentDetails::OnVariableTextChanged(const FText& InNewText)
{
	check(CachedNodePtr.IsValid());

	bIsVariableNameInvalid = true;

	USCS_Node* SCS_Node = CachedNodePtr->GetSCSNode();
	if(SCS_Node != NULL && !InNewText.IsEmpty() && !SCS_Node->IsValidVariableNameString(InNewText.ToString()))
	{
		VariableNameEditableTextBox->SetError(LOCTEXT("ComponentVariableRenameFailed_NotValid", "This name is reserved for engine use."));
		return;
	}

	TSharedPtr<INameValidatorInterface> VariableNameValidator = MakeShareable(new FKismetNameValidator(GetBlueprintObj(), CachedNodePtr->GetVariableName()));

	EValidatorResult ValidatorResult = VariableNameValidator->IsValid(InNewText.ToString());
	if(ValidatorResult == EValidatorResult::AlreadyInUse)
	{
		VariableNameEditableTextBox->SetError(FText::Format(LOCTEXT("ComponentVariableRenameFailed_InUse", "{0} is in use by another variable or function!"), InNewText));
	}
	else if(ValidatorResult == EValidatorResult::EmptyName)
	{
		VariableNameEditableTextBox->SetError(LOCTEXT("RenameFailed_LeftBlank", "Names cannot be left blank!"));
	}
	else if(ValidatorResult == EValidatorResult::TooLong)
	{
		VariableNameEditableTextBox->SetError(LOCTEXT("RenameFailed_NameTooLong", "Names must have fewer than 100 characters!"));
	}
	else
	{
		bIsVariableNameInvalid = false;
		VariableNameEditableTextBox->SetError(FText::GetEmpty());
	}
}

void FBlueprintComponentDetails::OnVariableTextCommitted(const FText& InNewName, ETextCommit::Type InTextCommit)
{
	if ( !bIsVariableNameInvalid )
	{
		check(CachedNodePtr.IsValid());

		USCS_Node* SCS_Node = CachedNodePtr->GetSCSNode();
		if(SCS_Node != NULL)
		{
			const FScopedTransaction Transaction( LOCTEXT("RenameComponentVariable", "Rename Component Variable") );
			FBlueprintEditorUtils::RenameComponentMemberVariable(GetBlueprintObj(), CachedNodePtr->GetSCSNode(), FName( *InNewName.ToString() ));
		}
	}

	bIsVariableNameInvalid = false;
	VariableNameEditableTextBox->SetError(FText::GetEmpty());
}

FText FBlueprintComponentDetails::OnGetTooltipText() const
{
	check(CachedNodePtr.IsValid());

	FName VarName = CachedNodePtr->GetVariableName();
	if (VarName != NAME_None)
	{
		FString Result;
		FBlueprintEditorUtils::GetBlueprintVariableMetaData(GetBlueprintObj(), VarName, TEXT("tooltip"), Result);
		return FText::FromString(Result);
	}

	return FText();
}

void FBlueprintComponentDetails::OnTooltipTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit, FName VarName)
{
	FBlueprintEditorUtils::SetBlueprintVariableMetaData(GetBlueprintObj(), VarName, TEXT("tooltip"), NewText.ToString() );
}

bool FBlueprintComponentDetails::OnVariableCategoryChangeEnabled() const
{
	check(CachedNodePtr.IsValid());

	return !CachedNodePtr->IsNative() && !CachedNodePtr->IsInherited();
}

FText FBlueprintComponentDetails::OnGetVariableCategoryText() const
{
	check(CachedNodePtr.IsValid());

	FName VarName = CachedNodePtr->GetVariableName();
	if (VarName != NAME_None)
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

		FName Category = FBlueprintEditorUtils::GetBlueprintVariableCategory(GetBlueprintObj(), VarName);

		// Older blueprints will have their name as the default category
		if( Category == GetBlueprintObj()->GetFName() )
		{
			return FText::FromName(K2Schema->VR_DefaultCategory);
		}
		else
		{
			return FText::FromName(Category);
		}
	}

	return FText();
}

void FBlueprintComponentDetails::OnVariableCategoryTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit, FName VarName)
{
	check(CachedNodePtr.IsValid());

	if (InTextCommit == ETextCommit::OnEnter || InTextCommit == ETextCommit::OnUserMovedFocus)
	{
		FString NewCategory = NewText.ToString();

		FBlueprintEditorUtils::SetBlueprintVariableCategory(GetBlueprintObj(), CachedNodePtr->GetVariableName(), FName( *NewCategory ));
		PopulateVariableCategories();
	}
}

void FBlueprintComponentDetails::OnVariableCategorySelectionChanged( TSharedPtr<FString> ProposedSelection, ESelectInfo::Type /*SelectInfo*/ )
{
	check(CachedNodePtr.IsValid());

	FName VarName = CachedNodePtr->GetVariableName();
	if (ProposedSelection.IsValid() && VarName != NAME_None)
	{
		FString NewCategory = *ProposedSelection.Get();
		FBlueprintEditorUtils::SetBlueprintVariableCategory(GetBlueprintObj(), VarName, FName( *NewCategory ));

		check(VariableCategoryListView.IsValid());
		check(VariableCategoryComboButton.IsValid());

		VariableCategoryListView->ClearSelection();
		VariableCategoryComboButton->SetIsOpen(false);
	}
}

TSharedRef< ITableRow > FBlueprintComponentDetails::MakeVariableCategoryViewWidget( TSharedPtr<FString> Item, const TSharedRef< STableViewBase >& OwnerTable )
{
	return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
	[
		SNew(STextBlock)
		.Text(*Item.Get())
	];
}

void FBlueprintComponentDetails::PopulateVariableCategories()
{
	UBlueprint* Blueprint = GetBlueprintObj();

	check(Blueprint);
	check(Blueprint->SkeletonGeneratedClass);

	TArray<FName> VisibleVariables;
	for (TFieldIterator<UProperty> PropertyIt(Blueprint->SkeletonGeneratedClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		UProperty* Property = *PropertyIt;

		if ((!Property->HasAnyPropertyFlags(CPF_Parm) && Property->HasAllPropertyFlags(CPF_BlueprintVisible)))
		{
			VisibleVariables.Add(Property->GetFName());
		}
	}

	FBlueprintEditorUtils::GetSCSVariableNameList(Blueprint, VisibleVariables);

	VariableCategorySource.Empty();
	VariableCategorySource.Add(MakeShareable(new FString(TEXT("Default"))));
	for (int32 i = 0; i < VisibleVariables.Num(); ++i)
	{
		FName Category = FBlueprintEditorUtils::GetBlueprintVariableCategory(Blueprint, VisibleVariables[i]);
		if (Category != NAME_None && Category != Blueprint->GetFName())
		{
			bool bNewCategory = true;
			for (int32 j = 0; j < VariableCategorySource.Num() && bNewCategory; ++j)
			{
				bNewCategory &= *VariableCategorySource[j].Get() != Category.ToString();
			}
			if (bNewCategory)
			{
				VariableCategorySource.Add(MakeShareable(new FString(Category.ToString())));
			}
		}
	}
}

bool FBlueprintComponentDetails::IsNodeAttachable() const
{
	check(CachedNodePtr.IsValid());

	// See if node is an attachable type
	bool bCanAttach = false;	
	if (!CachedNodePtr->IsRoot())
	{
		// check component is the correct type
		if (USceneComponent* SceneComponent = Cast<USceneComponent>(CachedNodePtr->GetComponentTemplate()))
		{
			check(BlueprintEditorPtr.IsValid());
			TSharedPtr<SSCSEditor> Editor = BlueprintEditorPtr.Pin()->GetSCSEditor();
			check( Editor.IsValid() );
			check( Editor->SCS );

			USCS_Node* SCS_Node = CachedNodePtr->GetSCSNode();
			if (SCS_Node != NULL && Editor->IsNodeInSimpleConstructionScript(SCS_Node))
			{
				FSCSEditorTreeNodePtrType ParentFNode = CachedNodePtr->GetParent();
				
				// check parent is of the right type
				if (ParentFNode.IsValid())
				{
					if (USceneComponent* ParentComponent = Cast<USceneComponent>(ParentFNode->GetComponentTemplate()))
					{
						bCanAttach = ParentComponent->HasAnySockets();
					}
				}
			}
		}
	}
	
	return bCanAttach;
}

FText FBlueprintComponentDetails::GetSocketName() const
{
	check(CachedNodePtr.IsValid());

	if (CachedNodePtr->GetSCSNode() != NULL)
	{
		return FText::FromString(CachedNodePtr->GetSCSNode()->AttachToName.ToString());
	}
	return FText();
}

void FBlueprintComponentDetails::OnBrowseSocket()
{
	check(CachedNodePtr.IsValid());

	if (CachedNodePtr->GetSCSNode() != NULL)
	{
		TSharedPtr<SSCSEditor> Editor = BlueprintEditorPtr.Pin()->GetSCSEditor();
		check( Editor.IsValid() );
		check( Editor->SCS );

		FSCSEditorTreeNodePtrType ParentFNode = CachedNodePtr->GetParent();

		if (ParentFNode.IsValid())
		{
			if (USceneComponent* ParentSceneComponent = Cast<USceneComponent>(ParentFNode->GetComponentTemplate()))
			{
				if (ParentSceneComponent->HasAnySockets())
				{
					// Pop up a combo box to pick socket from mesh
					FSlateApplication::Get().PushMenu(
						Editor.ToSharedRef(),
						SNew(SSocketChooserPopup)
						.SceneComponent( ParentSceneComponent )
						.OnSocketChosen( this, &FBlueprintComponentDetails::OnSocketSelection ),
						FSlateApplication::Get().GetCursorPos(),
						FPopupTransitionEffect( FPopupTransitionEffect::TypeInPopup )
						);
				}
			}
		}
	}
}

void FBlueprintComponentDetails::OnClearSocket()
{
	check(CachedNodePtr.IsValid());

	if (CachedNodePtr->GetSCSNode() != NULL)
	{
		CachedNodePtr->GetSCSNode()->AttachToName = NAME_None;
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprintObj());
	}
}

void FBlueprintComponentDetails::OnSocketSelection( FName SocketName )
{
	check(CachedNodePtr.IsValid());

	USCS_Node* SCS_Node = CachedNodePtr->GetSCSNode();
	if (SCS_Node != NULL)
	{
		// Record selection if there is an actual asset attached
		SCS_Node->AttachToName = SocketName;
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprintObj());
	}
}

#undef LOCTEXT_NAMESPACE
