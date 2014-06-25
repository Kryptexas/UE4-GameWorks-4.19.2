// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "WidgetBlueprintEditorUtils.h"
#include "WidgetBlueprintEditor.h"
#include "Kismet2NameValidators.h"
#include "BlueprintEditorUtils.h"
#include "K2Node_Variable.h"
#include "WidgetTemplateClass.h"
#include "Factories.h"
#include "UnrealExporter.h"

#define LOCTEXT_NAMESPACE "UMG"

class FWidgetObjectTextFactory : public FCustomizableTextObjectFactory
{
public:
	FWidgetObjectTextFactory()
		: FCustomizableTextObjectFactory(GWarn)
	{
	}

	// FCustomizableTextObjectFactory implementation

	virtual bool CanCreateClass(UClass* ObjectClass) const override
	{
		const bool bIsWidget = ObjectClass->IsChildOf(UWidget::StaticClass());
		const bool bIsSlot = ObjectClass->IsChildOf(UPanelSlot::StaticClass());

		return bIsWidget || bIsSlot;
	}

	virtual void ProcessConstructedObject(UObject* NewObject) override
	{
		check(NewObject);

		// Add it to the new object map
		NewObjectMap.Add(NewObject->GetFName(), Cast<UWidget>(NewObject));

		// If this is a scene component and it has a parent
		UWidget* Widget = Cast<UWidget>(NewObject);
		if ( Widget && Widget->Slot )
		{
			// Add an entry to the child->parent name map
//			ParentMap.Add(NewObject->GetFName(), Widget->AttachParent->GetFName());

			// Clear this so it isn't used when constructing the new SCS node
			//Widget->AttachParent = NULL;
		}
	}

	// FCustomizableTextObjectFactory (end)

public:

	// Child->Parent name map
	TMap<FName, FName> ParentMap;

	// Name->Instance object mapping
	TMap<FName, UWidget*> NewObjectMap;
};

bool FWidgetBlueprintEditorUtils::RenameWidget(TSharedRef<FWidgetBlueprintEditor> BlueprintEditor, const FName& OldName, const FName& NewName)
{
	UWidgetBlueprint* Blueprint = BlueprintEditor->GetWidgetBlueprintObj();
	check(Blueprint);

	bool bRenamed = false;

	TSharedPtr<INameValidatorInterface> NameValidator = MakeShareable(new FKismetNameValidator(Blueprint));

	// NewName should be already validated. But one must make sure that NewTemplateName is also unique.
	const bool bUniqueNameForTemplate = ( EValidatorResult::Ok == NameValidator->IsValid(NewName) );

	const FString NewNameStr = NewName.ToString();
	const FString OldNameStr = OldName.ToString();

	UWidget* Widget = Blueprint->WidgetTree->FindWidget(OldNameStr);
	check(Widget);

	if ( Widget )
	{
		// Rename Template
		Blueprint->Modify();
		Widget->Modify();
		Widget->Rename(*NewNameStr);

		// Rename Preview
		UWidget* WidgetPreview = FWidgetReference::FromTemplate(BlueprintEditor, Widget).GetPreview();
		if ( WidgetPreview )
		{
			WidgetPreview->Rename(*NewNameStr);
		}

		TArray<UK2Node_Variable*> WidgetVarNodes;
		FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_Variable>(Blueprint, WidgetVarNodes);
		for ( int32 It = 0; It < WidgetVarNodes.Num(); It++ )
		{
			UK2Node_Variable* TestNode = WidgetVarNodes[It];
			if ( TestNode && ( OldName == TestNode->GetVarName() ) )
			{
				UEdGraphPin* TestPin = TestNode->FindPin(OldNameStr);
				if ( TestPin && ( Widget->GetClass() == TestPin->PinType.PinSubCategoryObject.Get() ) )
				{
					TestNode->Modify();
					if ( TestNode->VariableReference.IsSelfContext() )
					{
						TestNode->VariableReference.SetSelfMember(NewName);
					}
					else
					{
						//TODO:
						UClass* ParentClass = TestNode->VariableReference.GetMemberParentClass((UClass*)NULL);
						TestNode->VariableReference.SetExternalMember(NewName, ParentClass);
					}
					TestPin->Modify();
					TestPin->PinName = NewNameStr;
				}
			}
		}

		// Validate child blueprints and adjust variable names to avoid a potential name collision
		FBlueprintEditorUtils::ValidateBlueprintChildVariables(Blueprint, NewName);

		// Refresh references and flush editors
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		bRenamed = true;

		// Refresh references and flush editors
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		bRenamed = true;
	}

	return bRenamed;
}

void FWidgetBlueprintEditorUtils::CreateWidgetContextMenu(FMenuBuilder& MenuBuilder, TSharedRef<FWidgetBlueprintEditor> BlueprintEditor, FVector2D TargetLocation)
{
	BlueprintEditor->PasteDropLocation = TargetLocation;

	TSet<FWidgetReference> Widgets = BlueprintEditor->GetSelectedWidgets();
	UWidgetBlueprint* BP = BlueprintEditor->GetWidgetBlueprintObj();

	MenuBuilder.PushCommandList(BlueprintEditor->WidgetCommandList.ToSharedRef());

	MenuBuilder.BeginSection("Actions");
	{
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Copy);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Paste);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Delete);

		MenuBuilder.AddSubMenu(
			LOCTEXT("WidgetTree_WrapWith", "Wrap With..."),
			LOCTEXT("WidgetTree_WrapWithToolTip", "Wraps the currently selected widgets inside of another container widget"),
			FNewMenuDelegate::CreateStatic(&FWidgetBlueprintEditorUtils::BuildWrapWithMenu, BP, Widgets)
			);
	}
	MenuBuilder.EndSection();

	MenuBuilder.PopCommandList();
}

void FWidgetBlueprintEditorUtils::DeleteWidgets(UWidgetBlueprint* BP, TSet<FWidgetReference> Widgets)
{
	if ( Widgets.Num() > 0 )
	{
		const FScopedTransaction Transaction(LOCTEXT("RemoveWidget", "Remove Widget"));
		BP->WidgetTree->SetFlags(RF_Transactional);
		BP->WidgetTree->Modify();

		bool bRemoved = false;
		for ( FWidgetReference& Item : Widgets )
		{
			bRemoved = BP->WidgetTree->RemoveWidget(Item.GetTemplate());
		}

		//TODO UMG There needs to be an event for widget removal so that caches can be updated, and selection

		if ( bRemoved )
		{
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		}
	}
}

void FWidgetBlueprintEditorUtils::BuildWrapWithMenu(FMenuBuilder& Menu, UWidgetBlueprint* BP, TSet<FWidgetReference> Widgets)
{
	Menu.BeginSection("WrapWith", LOCTEXT("WidgetTree_WrapWith", "Wrap With..."));
	{
		for ( TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt )
		{
			UClass* WidgetClass = *ClassIt;
			if ( WidgetClass->IsChildOf(UPanelWidget::StaticClass()) && WidgetClass->HasAnyClassFlags(CLASS_Abstract) == false )
			{
				Menu.AddMenuEntry(
					WidgetClass->GetDisplayNameText(),
					FText::GetEmpty(),
					FSlateIcon(),
					FUIAction(
					FExecuteAction::CreateStatic(&FWidgetBlueprintEditorUtils::WrapWidgets, BP, Widgets, WidgetClass),
					FCanExecuteAction()
					)
					);
			}
		}
	}
	Menu.EndSection();
}

void FWidgetBlueprintEditorUtils::WrapWidgets(UWidgetBlueprint* BP, TSet<FWidgetReference> Widgets, UClass* WidgetClass)
{
	TSharedPtr<FWidgetTemplateClass> Template = MakeShareable(new FWidgetTemplateClass(WidgetClass));

	UPanelWidget* NewWrapperWidget = CastChecked<UPanelWidget>(Template->Create(BP->WidgetTree));

	//TODO UMG When wrapping multiple widgets, how will that work?
	for ( FWidgetReference& Item : Widgets )
	{
		int32 OutIndex;
		UPanelWidget* CurrentParent = BP->WidgetTree->FindWidgetParent(Item.GetTemplate(), OutIndex);
		if ( CurrentParent )
		{
			CurrentParent->ReplaceChildAt(OutIndex, NewWrapperWidget);

			NewWrapperWidget->AddChild(Item.GetTemplate());
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
}

void FWidgetBlueprintEditorUtils::CopyWidgets(UWidgetBlueprint* BP, TSet<FWidgetReference> Widgets)
{
	TSet<UWidget*> CopyableWidets;
	for ( const FWidgetReference& Widget : Widgets )
	{
		UWidget* ParentWidget = Widget.GetTemplate();
		CopyableWidets.Add(ParentWidget);

		UWidget::GatherAllChildren(ParentWidget, CopyableWidets);
	}

	FString ExportedText;
	FWidgetBlueprintEditorUtils::ExportWidgetsToText(CopyableWidets, /*out*/ ExportedText);
	FPlatformMisc::ClipboardCopy(*ExportedText);
}

void FWidgetBlueprintEditorUtils::ExportWidgetsToText(TSet<UWidget*> WidgetsToExport, /*out*/ FString& ExportedText)
{
	// Clear the mark state for saving.
	UnMarkAllObjects(EObjectMark(OBJECTMARK_TagExp | OBJECTMARK_TagImp));

	FStringOutputDevice Archive;
	const FExportObjectInnerContext Context;

	// Export each of the selected nodes
	UObject* LastOuter = NULL;
	for ( UWidget* Widget : WidgetsToExport )
	{
		// The nodes should all be from the same scope
		UObject* ThisOuter = Widget->GetOuter();
		check(( LastOuter == ThisOuter ) || ( LastOuter == NULL ));
		LastOuter = ThisOuter;

		UExporter::ExportToOutputDevice(&Context, Widget, NULL, Archive, TEXT("copy"), 0, PPF_ExportsNotFullyQualified | PPF_Copy | PPF_Delimited, false, ThisOuter);
	}

	ExportedText = Archive;
}

void FWidgetBlueprintEditorUtils::PasteWidgets(UWidgetBlueprint* BP, FWidgetReference ParentWidgetRef, FVector2D PasteLocation)
{
	const FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());

	UPanelWidget* ParentWidget = CastChecked<UPanelWidget>(ParentWidgetRef.GetTemplate());

	if ( ParentWidget )
	{
		ParentWidget->Modify();
	}

	// Grab the text to paste from the clipboard.
	FString TextToImport;
	FPlatformMisc::ClipboardPaste(TextToImport);

	// Import the nodes
	TSet<UWidget*> PastedWidgets;
	FWidgetBlueprintEditorUtils::ImportWidgetsFromText(BP, TextToImport, /*out*/ PastedWidgets);

	for ( UWidget* NewWidget : PastedWidgets )
	{
		// Widgets with a null parent mean that they were the root most widget of their selection set when
		// they were copied and thus we need to paste only the root most widgets.  All their children will be added
		// automatically.
		if ( NewWidget->GetParent() == NULL )
		{
			UPanelSlot* Slot = ParentWidget->AddChild(NewWidget);
			Slot->SetDesiredPosition(PasteLocation);
		}

		//TODO UMG HACK Because the WidgetTree isn't a real tree, add every widget.
		NewWidget->SetFlags( RF_Transactional );
		BP->WidgetTree->WidgetTemplates.Add( NewWidget );
	}

	//TODO UMG Reorder the live slot without rebuilding the structure
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
}

void FWidgetBlueprintEditorUtils::ImportWidgetsFromText(UWidgetBlueprint* BP, const FString& TextToImport, /*out*/ TSet<UWidget*>& ImportedWidgetSet)
{
	// Turn the text buffer into objects
	FWidgetObjectTextFactory Factory;
	Factory.ProcessBuffer(BP->WidgetTree, RF_Transactional, TextToImport);

	for ( auto& Entry : Factory.NewObjectMap )
	{
		ImportedWidgetSet.Add(Entry.Value);
	}
}

#undef LOCTEXT_NAMESPACE
