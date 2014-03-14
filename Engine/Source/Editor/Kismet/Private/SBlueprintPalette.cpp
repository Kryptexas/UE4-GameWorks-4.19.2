// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "SBlueprintPalette.h"
#include "UnrealEd.h"
#include "BPFunctionDragDropAction.h"
#include "BPVariableDragDropAction.h"
#include "BPDelegateDragDropAction.h"
#include "AnimGraphDefinitions.h"
#include "BlueprintEditorUtils.h"
#include "Kismet2NameValidators.h"
#include "ScopedTransaction.h"
#include "EditorWidgets.h"
#include "AssetRegistryModule.h"
#include "Editor/GraphEditor/Public/SGraphActionMenu.h"
#include "SMyBlueprint.h"
#include "AssetToolsModule.h"
#include "AssetRegistryModule.h"
#include "IDocumentation.h"
#include "SBlueprintLibraryPalette.h"
#include "SBlueprintFavoritesPalette.h"
#include "BlueprintPaletteFavorites.h"
#include "STutorialWrapper.h"
#include "ClassIconFinder.h"

#define LOCTEXT_NAMESPACE "BlueprintPalette"

/*******************************************************************************
* Static File Helpers
*******************************************************************************/

/** namespace'd to avoid collisions during unified builds */
namespace BlueprintPalette
{
	static FString const ConfigSection("BlueprintEditor.Palette");
	static FString const FavoritesHeightConfigKey("FavoritesHeightRatio");
	static FString const LibraryHeightConfigKey("LibraryHeightRatio");
}

/**
 * A helper method intended for constructing tooltips on palette items 
 * associated with specific blueprint variables (gets a string representing the 
 * specified variable's type)
 * 
 * @param  VarClass	The class that owns the variable in question.
 * @param  VarName	The name of the variable you want the type of.
 * @return A string representing the variable's type (empty if the variable couldn't be found).
 */
static FString GetVarType(UClass* VarClass, FName VarName, bool bUseObjToolTip)
{
	FString VarDesc;

	if(VarClass != NULL)
	{
		UProperty* Property = FindField<UProperty>(VarClass, VarName);
		if(Property != NULL)
		{
			// If it is an object property, see if we can get a nice class description instead of just the name
			UObjectProperty* ObjProp = Cast<UObjectProperty>(Property);
			if (bUseObjToolTip && ObjProp && ObjProp->PropertyClass)
			{
				VarDesc = ObjProp->PropertyClass->GetToolTipText().ToString();
			}

			// Name of type
			if (VarDesc.Len() == 0)
			{
				const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

				FEdGraphPinType PinType;
				if (K2Schema->ConvertPropertyToPinType(Property, PinType)) // use schema to get the color
				{
					VarDesc = UEdGraphSchema_K2::TypeToString(PinType);
				}
			}
		}
	}

	return VarDesc;
}

/**
 * Util function that helps construct a tooltip for a specific variable action 
 * (attempts to grab the variable's "tooltip" metadata).
 * 
 * @param  InBlueprint	The blueprint that the palette is associated.
 * @param  VarClass		The class that owns the variable.
 * @param  VarName		The variable you want a tooltip for.
 * @return A string from the variable's "tooltip" metadata (empty if the variable wasn't found, or it didn't have the metadata).
 */
static FString GetVarTooltip(UBlueprint* InBlueprint, UClass* VarClass, FName VarName)
{
	FString ResultTooltip;
	if(VarClass != NULL)
	{
		UProperty* Property = FindField<UProperty>(VarClass, VarName);
		if(Property != NULL)
		{
			// discover if the variable property is a non blueprint user variable
			UClass* SourceClass = Property->GetOwnerClass();
			if( SourceClass && SourceClass->ClassGeneratedBy == NULL )
			{
				ResultTooltip = Property->GetToolTipText().ToString();
			}
			else
			{
				FBlueprintEditorUtils::GetBlueprintVariableMetaData(InBlueprint, VarName, TEXT("tooltip"), ResultTooltip);
			}
		}
	}

	return ResultTooltip;
}

/**
 * A utility function intended to aid the construction of a specific blueprint 
 * palette item (specifically FEdGraphSchemaAction_K2Graph palette items). Based 
 * off of the sub-graph's type, this gets an icon representing said sub-graph.
 * 
 * @param  ActionIn		The FEdGraphSchemaAction_K2Graph action that the palette item represents.
 * @param  BlueprintIn	The blueprint currently being edited (that the action is for).
 * @param  IconOut		An icon denoting the sub-graph's type.
 * @param  ToolTipOut	The tooltip to display when the icon is hovered over (describing the sub-graph type).
 */
static void GetSubGraphIcon(FEdGraphSchemaAction_K2Graph const* const ActionIn, UBlueprint const* BlueprintIn, FSlateBrush const*& IconOut, FString& ToolTipOut)
{
	check(BlueprintIn != NULL);

	switch (ActionIn->GraphType)
	{
	case EEdGraphSchemaAction_K2Graph::Graph:
		{
			if (ActionIn->EdGraph != NULL)
			{
				IconOut = FBlueprintEditor::GetGlyphForGraph(ActionIn->EdGraph);
			}
			else
			{
				IconOut = FEditorStyle::GetBrush(TEXT("GraphEditor.EventGraph_16x"));
			}

			ToolTipOut = FString::Printf( *LOCTEXT("EventGraph_ToolTip", "Event Graph").ToString() );
		}
		break;
	case EEdGraphSchemaAction_K2Graph::Subgraph:
		{
			if ( ActionIn->EdGraph != NULL && ActionIn->EdGraph->IsA(UAnimationStateMachineGraph::StaticClass()) )
			{
				IconOut = FEditorStyle::GetBrush(TEXT("GraphEditor.StateMachine_16x") );
				ToolTipOut = FString::Printf( *LOCTEXT("AnimationStateMachineGraph_ToolTip", "Animation State Machine").ToString() );
			}
			else if ( ActionIn->EdGraph != NULL && ActionIn->EdGraph->IsA(UAnimationStateGraph::StaticClass()) )
			{
				IconOut = FEditorStyle::GetBrush(TEXT("GraphEditor.State_16x") );
				ToolTipOut = FString::Printf( *LOCTEXT("AnimationState_ToolTip", "Animation State").ToString() );
			}
			else if ( ActionIn->EdGraph != NULL && ActionIn->EdGraph->IsA(UAnimationTransitionGraph::StaticClass()) )
			{
				UObject* EdGraphOuter = ActionIn->EdGraph->GetOuter();
				if ( EdGraphOuter != NULL && EdGraphOuter->IsA(UAnimStateConduitNode::StaticClass()) )
				{
					IconOut = FEditorStyle::GetBrush(TEXT("GraphEditor.Conduit_16x"));
					ToolTipOut = FString::Printf( *LOCTEXT("ConduitGraph_ToolTip", "Conduit").ToString() );
				}
				else
				{
					IconOut = FEditorStyle::GetBrush(TEXT("GraphEditor.Rule_16x"));
					ToolTipOut = FString::Printf( *LOCTEXT("AnimationTransitionGraph_ToolTip", "Animation Transition Rule").ToString() );
				}
			}
			else
			{
				IconOut = FEditorStyle::GetBrush(TEXT("GraphEditor.SubGraph_16x") );
				ToolTipOut = FString::Printf( *LOCTEXT("EventSubgraph_ToolTip", "Event Subgraph").ToString() );
			}
		}
		break;
	case EEdGraphSchemaAction_K2Graph::Macro:
		{
			IconOut = FEditorStyle::GetBrush(TEXT("GraphEditor.Macro_16x"));
			if ( ActionIn->EdGraph == NULL )
			{
				ToolTipOut = FString::Printf( *LOCTEXT("PotentialOverride_Tooltip", "Potential Override").ToString() );	
			}
			else
			{
				// Need to see if this is a function overriding something in the parent, or 
				UFunction* OverrideFunc = FindField<UFunction>(BlueprintIn->ParentClass, ActionIn->FuncName);
				if ( OverrideFunc == NULL )
				{
					ToolTipOut = FString::Printf( *LOCTEXT("Macro_Tooltip", "Macro").ToString() );
				}
				else 
				{
					ToolTipOut = FString::Printf( *LOCTEXT("Override_Tooltip", "Override").ToString() );
				}
			}
		}
		break;
	case EEdGraphSchemaAction_K2Graph::Interface:
		{
			IconOut = FEditorStyle::GetBrush(TEXT("GraphEditor.InterfaceFunction_16x"));
			ToolTipOut = FString::Printf(*LOCTEXT("FunctionFromInterface_Tooltip", "Function (from Interface '%s')").ToString(), *ActionIn->FuncName.ToString());
		}
		break;
	case EEdGraphSchemaAction_K2Graph::Function:
		{
			if ( ActionIn->EdGraph == NULL )
			{
				IconOut = FEditorStyle::GetBrush(TEXT("GraphEditor.PotentialOverrideFunction_16x"));
				ToolTipOut = FString::Printf( *LOCTEXT("PotentialOverride_Tooltip", "Potential Override").ToString() );	
			}
			else
			{
				if ( ActionIn->EdGraph->IsA(UAnimationGraph::StaticClass()) )
				{
					IconOut = FEditorStyle::GetBrush(TEXT("GraphEditor.Animation_16x"));
				}
				else
				{
					UFunction* OverrideFunc = FindField<UFunction>(BlueprintIn->ParentClass, ActionIn->FuncName);
					if ( OverrideFunc == NULL )
					{
						IconOut = FEditorStyle::GetBrush(TEXT("GraphEditor.Function_16x"));
						if ( ActionIn->EdGraph != NULL && ActionIn->EdGraph->IsA(UAnimationGraph::StaticClass()) )
						{
							ToolTipOut = FString::Printf( *LOCTEXT("AnimationGraph_Tooltip", "Animation Graph").ToString() );
						}
						else
						{
							ToolTipOut = FString::Printf( *LOCTEXT("Function_Tooltip", "Function").ToString() );
						}
					}
					else
					{
						IconOut = FEditorStyle::GetBrush(TEXT("GraphEditor.OverrideFunction_16x"));
						ToolTipOut = FString::Printf( *LOCTEXT("Override_Tooltip", "Override").ToString() );
					}
				}
			}
		}
		break;
	}
}

/**
 * A utility function intended to aid the construction of a specific blueprint 
 * palette item. This looks at the item's associated action, and based off its  
 * type, retrieves an icon, color and tooltip for the slate widget.
 * 
 * @param  ActionIn		The action associated with the palette item you want an icon for.
 * @param  BlueprintIn	The blueprint currently being edited (that the action is for).
 * @param  BrushOut		An output of the icon, best representing the specified action.
 * @param  ColorOut		An output color, further denoting the specified action.
 * @param  ToolTipOut	An output tooltip, best describing the specified action type.
 */
static void GetPaletteItemIcon(TSharedPtr<FEdGraphSchemaAction> ActionIn, UBlueprint const* BlueprintIn, FSlateBrush const*& BrushOut, FSlateColor& ColorOut, FString& ToolTipOut, FString& DocLinkOut, FString& DocExcerptOut)
{
	// Default to tooltip based on action supplied
	ToolTipOut = (ActionIn->TooltipDescription.Len() > 0) ? ActionIn->TooltipDescription : ActionIn->MenuDescription;

	if (ActionIn->GetTypeId() == FEdGraphSchemaAction_K2TargetNode::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2TargetNode* TargetNodeAction = (FEdGraphSchemaAction_K2TargetNode*)ActionIn.Get();
		if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(TargetNodeAction->NodeTemplate))
		{
			BrushOut = FEditorStyle::GetBrush(TEXT("GraphEditor.Event_16x"));
		}
		else if (UK2Node_LocalVariable* LocalVariableNode = Cast<UK2Node_LocalVariable>(TargetNodeAction->NodeTemplate))
		{
			FLinearColor LinearColor;
			BrushOut = FEditorStyle::GetBrush(UK2Node_Variable::GetVarIconFromPinType(LocalVariableNode->VariableType, LinearColor));
			ColorOut = LinearColor;
		}
	}
	else if (ActionIn->GetTypeId() == FEdGraphSchemaAction_K2AddComponent::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2AddComponent* AddCompAction = (FEdGraphSchemaAction_K2AddComponent*)ActionIn.Get();
		// Get icon from class
		UClass* ComponentClass = *(AddCompAction->ComponentClass);
		if (ComponentClass)
		{
			BrushOut = FClassIconFinder::FindIconForClass(ComponentClass);
			ToolTipOut = ComponentClass->GetName();
		}
	}
	else if (UK2Node const* const NodeTemplate = FK2SchemaActionUtils::ExtractNodeTemplateFromAction(ActionIn))
	{
		// If the node wants to create tooltip text, use that instead, because its probably more detailed
		FString NodeToolTipText = NodeTemplate->GetTooltip();
		if (NodeToolTipText.Len() > 0)
		{
			ToolTipOut = NodeToolTipText;
		}

		// Ask node for a palette icon
		FLinearColor IconLinearColor = FLinearColor::White;
		BrushOut = FEditorStyle::GetBrush(NodeTemplate->GetPaletteIcon(IconLinearColor));
		ColorOut = IconLinearColor;
	}
	else if(ActionIn->GetTypeId() == FEdGraphSchemaAction_K2Graph::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2Graph const* GraphAction = (FEdGraphSchemaAction_K2Graph const*)ActionIn.Get();
		GetSubGraphIcon(GraphAction, BlueprintIn, BrushOut, ToolTipOut);
	}
	else if(ActionIn->GetTypeId() == FEdGraphSchemaAction_K2Delegate::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2Delegate* DelegateAction = (FEdGraphSchemaAction_K2Delegate*)ActionIn.Get();

		BrushOut = FEditorStyle::GetBrush(TEXT("GraphEditor.Delegate_16x"));
		ToolTipOut = FString::Printf( *LOCTEXT( "Delegate_Tooltip", "Event Dispatcher '%s'" ).ToString(), *DelegateAction->GetDelegateName().ToString());
	}
	else if(ActionIn->GetTypeId() == FEdGraphSchemaAction_K2Var::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2Var* VarAction = (FEdGraphSchemaAction_K2Var*)ActionIn.Get();

		UClass* VarClass = VarAction->GetVariableClass();
		BrushOut = FBlueprintEditor::GetVarIconAndColor(VarClass, VarAction->GetVariableName(), ColorOut);
		ToolTipOut = GetVarType(VarClass, VarAction->GetVariableName(), true);

		DocLinkOut = TEXT("Shared/Editor/Blueprint/VariableTypes");
		DocExcerptOut = GetVarType(VarClass, VarAction->GetVariableName(), false);
	}
	else if(ActionIn->GetTypeId() == FEdGraphSchemaAction_K2Enum::StaticGetTypeId())
	{
		BrushOut = FEditorStyle::GetBrush(TEXT("GraphEditor.EnumGlyph"));
		ToolTipOut = FString::Printf( *LOCTEXT("Enum_Tooltip", "Enum Asset").ToString() );
	}
	else if (ActionIn->GetTypeId() == FEdGraphSchemaAction_K2AddComment::StaticGetTypeId() ||
		ActionIn->GetTypeId() == FEdGraphSchemaAction_NewStateComment::StaticGetTypeId())
	{
		ToolTipOut = LOCTEXT("Comment_Tooltip", "Create a resizable comment box.").ToString();
		BrushOut = FEditorStyle::GetBrush(TEXT("GraphEditor.Comment_16x"));
	}
}

/**
 * Takes the existing tooltip and concats a path id (for the specified action) 
 * to the end.
 * 
 * @param  ActionIn		The action you want to show the path for.
 * @param  OldToolTip	The tooltip that you're replacing (we fold it into the new one)/
 * @return The newly created tooltip (now with the action's path tacked on the bottom).
 */
static TSharedRef<SToolTip> ConstructToolTipWithActionPath(TSharedPtr<FEdGraphSchemaAction> ActionIn, TSharedPtr<SToolTip> OldToolTip)
{
	TSharedRef<SToolTip> NewToolTip = OldToolTip.ToSharedRef();

	FFavoritedBlueprintPaletteItem ActionItem(ActionIn);
	if (ActionItem.IsValid())
	{
		FTextBlockStyle PathStyle = FTextBlockStyle()
			.SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 8))
			.SetColorAndOpacity(FLinearColor(0.4f, 0.4f, 0.4f));

		NewToolTip = SNew(SToolTip)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			[
				OldToolTip->GetContent()
			]

			+SVerticalBox::Slot()
				.HAlign(EHorizontalAlignment::HAlign_Right)
			[
				SNew(STextBlock)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.Text(FText::FromString(ActionItem.ToString()))
					.TextStyle(&PathStyle)
			]
		];
	}

	return NewToolTip;
}

/*******************************************************************************
* FBlueprintPaletteItemRenameUtils
*******************************************************************************/

/** A set of utilities to aid SBlueprintPaletteItem when the user attempts to rename one. */
struct FBlueprintPaletteItemRenameUtils
{
	/**
	 * Determines whether the enum node, associated with the selected action, 
	 * can be renamed with the specified text.
	 * 
	 * @param  InNewText		The text you want to verify.
	 * @param  OutErrorMessage	Text explaining why the associated node couldn't be renamed (if the return value is false).
	 * @param  ActionPtr		The selected action that the calling palette item represents.
	 * @return True if it is ok to rename the associated node with the given string (false if not).
	 */
	static bool VerifyNewEnumName(const FText& InNewText, FText& OutErrorMessage, TWeakPtr<FEdGraphSchemaAction> ActionPtr)
	{
		// Should never make it here with anything but an enum action
		check(ActionPtr.Pin()->GetTypeId() == FEdGraphSchemaAction_K2Enum::StaticGetTypeId());

		FEdGraphSchemaAction_K2Enum* EnumAction = (FEdGraphSchemaAction_K2Enum*)ActionPtr.Pin().Get();

		if(EnumAction->Enum->GetName() == InNewText.ToString())
		{
			return true;
		}

		TArray<FAssetData> AssetData;
		FAssetRegistryModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetToolsModule.Get().GetAssetsByPath(FName(*FPaths::GetPath(EnumAction->Enum->GetOutermost()->GetPathName())), AssetData);

		if(!FEditorFileUtils::IsFilenameValidForSaving(InNewText.ToString(), OutErrorMessage) || !FName(*InNewText.ToString()).IsValidObjectName( OutErrorMessage ))
		{
			return false;
		}
		else if( InNewText.ToString().Len() > NAME_SIZE )
		{
			OutErrorMessage = LOCTEXT("RenameFailed_NameTooLong", "Names must have fewer than 100 characters!");
		}
		else
		{
			// Check to see if the name conflicts
			for( auto Iter = AssetData.CreateConstIterator(); Iter; ++Iter)
			{
				if(Iter->AssetName.ToString() == InNewText.ToString())
				{
					OutErrorMessage = FText::FromString(TEXT("Asset name already in use!"));
					return false;
				}
			}
		}

		return true;
	}

	/**
	 * Take the verified text and renames the enum node associated with the 
	 * selected action.
	 * 
	 * @param  NewText				The new (verified) text to rename the node with.
	 * @param  InTextCommit			A value denoting how the text was entered.
	 * @param  ActionPtr			The selected action that the calling palette item represents.
	 * @param  BlueprintEditorPtr	A pointer to the blueprint editor that the palette belongs to.
	 */
	static void CommitNewEnumName(const FText& NewText, ETextCommit::Type InTextCommit, TWeakPtr<FEdGraphSchemaAction> ActionPtr, TWeakPtr<FBlueprintEditor> BlueprintEditorPtr)
	{
		// Should never make it here with anything but an enum action
		check(ActionPtr.Pin()->GetTypeId() == FEdGraphSchemaAction_K2Enum::StaticGetTypeId());

		FEdGraphSchemaAction_K2Enum* EnumAction = (FEdGraphSchemaAction_K2Enum*)ActionPtr.Pin().Get();

		if(EnumAction->Enum->GetName() != NewText.ToString())
		{
			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
			TArray<FAssetRenameData> AssetsAndNames;
			const FString PackagePath = FPackageName::GetLongPackagePath(EnumAction->Enum->GetOutermost()->GetName());
			new(AssetsAndNames) FAssetRenameData(EnumAction->Enum, PackagePath, NewText.ToString());

			BlueprintEditorPtr.Pin()->GetMyBlueprintWidget()->SelectItemByName(FName("ConstructionScript"));

			AssetToolsModule.Get().RenameAssets(AssetsAndNames);
		}

		BlueprintEditorPtr.Pin()->GetMyBlueprintWidget()->SelectItemByName(FName(*EnumAction->Enum->GetPathName()));
	}

	/**
	 * Determines whether the event node, associated with the selected action, 
	 * can be renamed with the specified text.
	 * 
	 * @param  InNewText		The text you want to verify.
	 * @param  OutErrorMessage	Text explaining why the associated node couldn't be renamed (if the return value is false).
	 * @param  ActionPtr		The selected action that the calling palette item represents.
	 * @return True if it is ok to rename the associated node with the given string (false if not).
	 */
	static bool VerifyNewEventName(const FText& InNewText, FText& OutErrorMessage, TWeakPtr<FEdGraphSchemaAction> ActionPtr)
	{
		bool bIsNameValid = false;
		OutErrorMessage = LOCTEXT("RenameFailed_NodeRename", "Cannot rename associated node!");

		check(ActionPtr.Pin()->GetTypeId() == FEdGraphSchemaAction_K2Event::StaticGetTypeId());
		FEdGraphSchemaAction_K2Event* EventAction = (FEdGraphSchemaAction_K2Event*)ActionPtr.Pin().Get();

		UK2Node* AssociatedNode = EventAction->NodeTemplate;
		if ((AssociatedNode != NULL) && AssociatedNode->bCanRenameNode)
		{
			TSharedPtr<INameValidatorInterface> NodeNameValidator = FNameValidatorFactory::MakeValidator(AssociatedNode);
			bIsNameValid = (NodeNameValidator->IsValid(InNewText.ToString(), true) == EValidatorResult::Ok);
		}
		return bIsNameValid;
	}

	/**
	 * Take the verified text and renames the event node associated with the 
	 * selected action.
	 * 
	 * @param  NewText		The new (verified) text to rename the node with.
	 * @param  InTextCommit	A value denoting how the text was entered.
	 * @param  ActionPtr	The selected action that the calling palette item represents.
	 */
	static void CommitNewEventName(const FText& NewText, ETextCommit::Type InTextCommit, TWeakPtr<FEdGraphSchemaAction> ActionPtr)
	{
		check(ActionPtr.Pin()->GetTypeId() == FEdGraphSchemaAction_K2Event::StaticGetTypeId());

		FEdGraphSchemaAction_K2Event* EventAction = (FEdGraphSchemaAction_K2Event*)ActionPtr.Pin().Get();
		if (EventAction->NodeTemplate != NULL)
		{
			EventAction->NodeTemplate->OnRenameNode(NewText.ToString());
		}
	}

	/**
	 * Determines whether the target node, associated with the selected action, 
	 * can be renamed with the specified text.
	 * 
	 * @param  InNewText		The text you want to verify.
	 * @param  OutErrorMessage	Text explaining why the associated node couldn't be renamed (if the return value is false).
	 * @param  ActionPtr		The selected action that the calling palette item represents.
	 * @return True if it is ok to rename the associated node with the given string (false if not).
	 */
	static bool VerifyNewTargetNodeName(const FText& InNewText, FText& OutErrorMessage, TWeakPtr<FEdGraphSchemaAction> ActionPtr)
	{

		bool bIsNameValid = false;
		OutErrorMessage = LOCTEXT("RenameFailed_NodeRename", "Cannot rename associated node!");

		check(ActionPtr.Pin()->GetTypeId() == FEdGraphSchemaAction_K2TargetNode::StaticGetTypeId());
		FEdGraphSchemaAction_K2TargetNode* TargetNodeAction = (FEdGraphSchemaAction_K2TargetNode*)ActionPtr.Pin().Get();

		UK2Node* AssociatedNode = TargetNodeAction->NodeTemplate;
		if ((AssociatedNode != NULL) && AssociatedNode->bCanRenameNode)
		{
			TSharedPtr<INameValidatorInterface> NodeNameValidator = FNameValidatorFactory::MakeValidator(AssociatedNode);
			bIsNameValid = (NodeNameValidator->IsValid(InNewText.ToString(), true) == EValidatorResult::Ok);
		}
		return bIsNameValid;
	}

	/**
	 * Take the verified text and renames the target node associated with the 
	 * selected action.
	 * 
	 * @param  NewText		The new (verified) text to rename the node with.
	 * @param  InTextCommit	A value denoting how the text was entered.
	 * @param  ActionPtr	The selected action that the calling palette item represents.
	 */
	static void CommitNewTargetNodeName(const FText& NewText, ETextCommit::Type InTextCommit, TWeakPtr<FEdGraphSchemaAction> ActionPtr)
	{
		check(ActionPtr.Pin()->GetTypeId() == FEdGraphSchemaAction_K2TargetNode::StaticGetTypeId());

		FEdGraphSchemaAction_K2TargetNode* TargetNodeAction = (FEdGraphSchemaAction_K2TargetNode*)ActionPtr.Pin().Get();
		if (TargetNodeAction->NodeTemplate != NULL)
		{
			TargetNodeAction->NodeTemplate->OnRenameNode(NewText.ToString());
		}
	}
};

/*******************************************************************************
* SPaletteItemVisibilityToggle
*******************************************************************************/

class SPaletteItemVisibilityToggle : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SPaletteItemVisibilityToggle ) {}
	SLATE_END_ARGS()

	/**
	 * Constructs a visibility-toggle widget (for variable actions only, so that 
	 * the user can modify the variable's "edit-on-instance" state).
	 * 
	 * @param  InArgs			A set of slate arguments, defined above.
	 * @param  ActionPtrIn		The FEdGraphSchemaAction that the parent item represents.
	 * @param  BlueprintEdPtrIn	A pointer to the blueprint editor that the palette belongs to.
	 */
	void Construct(const FArguments& InArgs, TWeakPtr<FEdGraphSchemaAction> ActionPtrIn, TWeakPtr<FBlueprintEditor> BlueprintEdPtrIn)
	{
		ActionPtr = ActionPtrIn;
		BlueprintEditorPtr = BlueprintEdPtrIn;
		TSharedPtr<FEdGraphSchemaAction> PaletteAction = ActionPtrIn.Pin();

		bool bShouldHaveAVisibilityToggle = false;
		if (PaletteAction->GetTypeId() == FEdGraphSchemaAction_K2Var::StaticGetTypeId())
		{
			UProperty* VariableProp = StaticCastSharedPtr<FEdGraphSchemaAction_K2Var>(PaletteAction)->GetProperty();
			UObjectProperty* VariableObjProp = Cast<UObjectProperty>(VariableProp);

			UClass* VarSourceClass = (VariableProp != NULL) ? CastChecked<UClass>(VariableProp->GetOuter()) : NULL;
			const bool bIsBlueprintVariable = (VarSourceClass == BlueprintEditorPtr.Pin()->GetBlueprintObj()->SkeletonGeneratedClass);
			const bool bIsComponentVar = (VariableObjProp != NULL) && (VariableObjProp->PropertyClass != NULL) && (VariableObjProp->PropertyClass->IsChildOf(UActorComponent::StaticClass()));
			bShouldHaveAVisibilityToggle = bIsBlueprintVariable && !bIsComponentVar;
		}

		this->ChildSlot[
			SNew(SBorder)
				.Padding( 0.0f )
				.BorderImage(FEditorStyle::GetBrush("NoBorder"))
				.ColorAndOpacity(this, &SPaletteItemVisibilityToggle::GetVisibilityToggleColor)
			[
				SNew( SCheckBox )
					.ToolTipText(this, &SPaletteItemVisibilityToggle::GetVisibilityToggleToolTip)
					.Visibility(bShouldHaveAVisibilityToggle ? EVisibility::Visible : EVisibility::Collapsed)
					.OnCheckStateChanged(this, &SPaletteItemVisibilityToggle::OnVisibilityToggleFlipped)
					.IsChecked(this, &SPaletteItemVisibilityToggle::GetVisibilityToggleState)
					// a style using the normal checkbox images but with the toggle button layout
					.Style( FEditorStyle::Get(), "CheckboxLookToggleButtonCheckbox")	
				[
					SNew( SVerticalBox )
					+SVerticalBox::Slot()
						.AutoHeight()
						.VAlign( VAlign_Center )
						.HAlign( HAlign_Center )
					[
						SNew( SImage )
							.Image( this, &SPaletteItemVisibilityToggle::GetVisibilityIcon )
							.ColorAndOpacity( FLinearColor::Black )
					]
				]
			]
		];
	}

private:
	/**
	 * Used by this visibility-toggle widget to see if the property represented 
	 * by this item is visible outside of Kismet.
	 * 
	 * @return ESlateCheckBoxState::Checked if the property is visible, false if not (or if the property wasn't found)
	 */
	ESlateCheckBoxState::Type GetVisibilityToggleState() const
	{
		TSharedPtr<FEdGraphSchemaAction_K2Var> VarAction = StaticCastSharedPtr<FEdGraphSchemaAction_K2Var>(ActionPtr.Pin());
		UProperty* VariableProperty = VarAction->GetProperty();
		if (VariableProperty != NULL)
		{
			return VariableProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance) ? ESlateCheckBoxState::Unchecked : ESlateCheckBoxState::Checked;
		}
		else
		{
			return ESlateCheckBoxState::Unchecked;
		}
	}

	/**
	 * Used by this visibility-toggle widget when the user makes a change to the
	 * checkbox (modifies the property represented by this item by flipping its
	 * edit-on-instance flag).
	 * 
	 * @param  InNewState	The new state that the user set the checkbox to.
	 */
	void OnVisibilityToggleFlipped(ESlateCheckBoxState::Type InNewState)
	{
		TSharedPtr<FEdGraphSchemaAction_K2Var> VarAction = StaticCastSharedPtr<FEdGraphSchemaAction_K2Var>(ActionPtr.Pin());

		// Toggle the flag on the blueprint's version of the variable description, based on state
		const bool bVariableIsExposed = (InNewState == ESlateCheckBoxState::Checked);

		UBlueprint* Blueprint = BlueprintEditorPtr.Pin()->GetBlueprintObj();
		FBlueprintEditorUtils::SetBlueprintOnlyEditableFlag(Blueprint, VarAction->GetVariableName(), !bVariableIsExposed);
	}

	/**
	 * Used by this visibility-toggle widget to convey the visibility of the 
	 * property represented by this item.
	 * 
	 * @return A image representing the variable's "edit-on-instance" state.
	 */
	const FSlateBrush* GetVisibilityIcon() const
	{
		return GetVisibilityToggleState() == ESlateCheckBoxState::Checked ?
			FEditorStyle::GetBrush( "Kismet.VariableList.ExposeForInstance" ) :
			FEditorStyle::GetBrush( "Kismet.VariableList.HideForInstance" );
	}

	/**
	 * Used by this visibility-toggle widget to convey the visibility of the 
	 * property represented by this item (as well as the status of the 
	 * variable's tooltip).
	 * 
	 * @return A color denoting the item's visibility and tootip status.
	 */
	FLinearColor GetVisibilityToggleColor() const 
	{
		if(GetVisibilityToggleState() != ESlateCheckBoxState::Checked)
		{
			return FColor(64,64,64).ReinterpretAsLinear();
		}
		else
		{
			TSharedPtr<FEdGraphSchemaAction_K2Var> VarAction = StaticCastSharedPtr<FEdGraphSchemaAction_K2Var>(ActionPtr.Pin());

			FString Result;
			FBlueprintEditorUtils::GetBlueprintVariableMetaData( BlueprintEditorPtr.Pin()->GetBlueprintObj(), VarAction->GetVariableName(), TEXT("tooltip"), Result);

			if(!Result.IsEmpty())
			{
				return FColor(130,219,119).ReinterpretAsLinear(); //pastel green when tooltip exists
			}
			else
			{
				return FColor(215,219,119).ReinterpretAsLinear(); //pastel yellow if no tooltip to alert designer 
			}
		}
	}

	/**
	 * Used by this visibility-toggle widget to supply the toggle with a tooltip
	 * representing the "edit-on-instance" state of the variable represented by 
	 * this item.
	 * 
	 * @return Tooltip text for this toggle.
	 */
	FString GetVisibilityToggleToolTip() const
	{
		FString ToolTip;
		if(GetVisibilityToggleState() != ESlateCheckBoxState::Checked)
		{
			ToolTip =  LOCTEXT("VariablePrivacy_not_public_Tooltip", "Variable is not public and will not be editable on an instance of this Blueprint.").ToString();
		}
		else
		{
			TSharedPtr<FEdGraphSchemaAction_K2Var> VarAction = StaticCastSharedPtr<FEdGraphSchemaAction_K2Var>(ActionPtr.Pin());

			FString Result;
			FBlueprintEditorUtils::GetBlueprintVariableMetaData( BlueprintEditorPtr.Pin()->GetBlueprintObj(), VarAction->GetVariableName(), TEXT("tooltip"), Result);
			if(!Result.IsEmpty())
			{
				ToolTip = LOCTEXT("VariablePrivacy_is_public_Tooltip", "Variable is public and is editable on each instance of this Blueprint.").ToString();
			}
			else
			{
				ToolTip = LOCTEXT("VariablePrivacy_is_public_no_tooltip_Tooltip", "Variable is public but MISSING TOOLTIP.").ToString();
			}
		}
		return ToolTip;
	}

private:
	/** The action that the owning palette entry represents */
	TWeakPtr<FEdGraphSchemaAction> ActionPtr;

	/** Pointer back to the blueprint editor that owns this */
	TWeakPtr<FBlueprintEditor>     BlueprintEditorPtr;
};

/*******************************************************************************
* SBlueprintPaletteItem Public Interface
*******************************************************************************/

//------------------------------------------------------------------------------
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SBlueprintPaletteItem::Construct(const FArguments& InArgs, FCreateWidgetForActionData* const InCreateData, TWeakPtr<FBlueprintEditor> InBlueprintEditor)
{
	check(InCreateData->Action.IsValid());
	check(InBlueprintEditor.IsValid());

	bShowClassInTooltip = InArgs._ShowClassInTooltip;	

	TSharedPtr<FEdGraphSchemaAction> GraphAction = InCreateData->Action;
	ActionPtr = InCreateData->Action;
	BlueprintEditorPtr = InBlueprintEditor;

	// construct the icon widget
	FSlateBrush const* IconBrush   = FEditorStyle::GetBrush(TEXT("NoBrush"));
	FSlateColor        IconColor   = FSlateColor::UseForeground();
	FString            IconToolTip = GraphAction->TooltipDescription;
	FString			   IconDocLink, IconDocExcerpt;
	GetPaletteItemIcon(GraphAction, InBlueprintEditor.Pin()->GetBlueprintObj(), IconBrush, IconColor, IconToolTip, IconDocLink, IconDocExcerpt);
	TSharedRef<SWidget> IconWidget = CreateIconWidget(IconToolTip, IconBrush, IconColor, IconDocLink, IconDocExcerpt);

	// construct the text widget
	FSlateFontInfo NameFont = FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 10);
	bool bIsReadOnly = FBlueprintEditorUtils::IsPaletteActionReadOnly(GraphAction, InBlueprintEditor.Pin());
	TSharedRef<SWidget> NameSlotWidget = CreateTextSlotWidget( NameFont, InCreateData, bIsReadOnly );
	
	// now, create the actual widget
	ChildSlot
	[
		SNew(SHorizontalBox)
			
		// icon slot
		+SHorizontalBox::Slot()
			.AutoWidth()
		[
			IconWidget
		]
		// name slot
		+SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			.Padding(3,0)
		[
			NameSlotWidget
		]
		// optional visibility slot
		+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(3,0))
			.VAlign(VAlign_Center)
		[
			SNew(SPaletteItemVisibilityToggle, ActionPtr, BlueprintEditorPtr)
		]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

/*******************************************************************************
* SBlueprintPaletteItem Private Methods
*******************************************************************************/

//------------------------------------------------------------------------------
TSharedRef<SWidget> SBlueprintPaletteItem::CreateTextSlotWidget(const FSlateFontInfo& NameFont, FCreateWidgetForActionData* const InCreateData, bool const bIsReadOnlyIn)
{
	FString const ActionTypeId = InCreateData->Action->GetTypeId();

	FOnVerifyTextChanged OnVerifyTextChanged;
	FOnTextCommitted     OnTextCommitted;
		
	// enums have different rules for renaming that exist outside the bounds of other items.
	if (ActionTypeId == FEdGraphSchemaAction_K2Enum::StaticGetTypeId())
	{
		OnVerifyTextChanged.BindStatic(&FBlueprintPaletteItemRenameUtils::VerifyNewEnumName, ActionPtr);
		OnTextCommitted.BindStatic(&FBlueprintPaletteItemRenameUtils::CommitNewEnumName, ActionPtr, BlueprintEditorPtr);
	}
	else if (ActionTypeId == FEdGraphSchemaAction_K2Event::StaticGetTypeId())
	{
		OnVerifyTextChanged.BindStatic(&FBlueprintPaletteItemRenameUtils::VerifyNewEventName, ActionPtr);
		OnTextCommitted.BindStatic(&FBlueprintPaletteItemRenameUtils::CommitNewEventName, ActionPtr);
	}
	else if (ActionTypeId == FEdGraphSchemaAction_K2TargetNode::StaticGetTypeId())
	{
		OnVerifyTextChanged.BindStatic(&FBlueprintPaletteItemRenameUtils::VerifyNewTargetNodeName, ActionPtr);
		OnTextCommitted.BindStatic(&FBlueprintPaletteItemRenameUtils::CommitNewTargetNodeName, ActionPtr);
	}
	else
	{
		// default to our own rename methods
		OnVerifyTextChanged.BindSP(this, &SBlueprintPaletteItem::OnNameTextVerifyChanged);
		OnTextCommitted.BindSP(this, &SBlueprintPaletteItem::OnNameTextCommitted);
	}

	// Copy the mouse delegate binding if we want it
	if( InCreateData->bHandleMouseButtonDown )
	{
		MouseButtonDownDelegate = InCreateData->MouseButtonDownDelegate;
	}

	TSharedPtr<SToolTip> ToolTipWidget = ConstructToolTipWidget();
	// If the creation data says read only, then it must be read only
	bool bIsReadOnly = (bIsReadOnlyIn || InCreateData->bIsReadOnly);

	TSharedPtr<SOverlay> DisplayWidget;
	TSharedPtr<SInlineEditableTextBlock> EditableTextElement;
	SAssignNew(DisplayWidget, SOverlay)
		+SOverlay::Slot()
		[
			SAssignNew(EditableTextElement, SInlineEditableTextBlock)
				.Text(this, &SBlueprintPaletteItem::GetDisplayText)
				.Font(NameFont)
				.HighlightText(InCreateData->HighlightText)
				.ToolTip(ToolTipWidget)
				.OnVerifyTextChanged(OnVerifyTextChanged)
				.OnTextCommitted(OnTextCommitted)
				.IsSelected(InCreateData->IsRowSelectedDelegate)
				.IsReadOnly(bIsReadOnly)
		];
	InlineRenameWidget = EditableTextElement.ToSharedRef();

	if(!bIsReadOnly)
	{
		InCreateData->OnRenameRequest->BindSP(InlineRenameWidget.Get(), &SInlineEditableTextBlock::EnterEditingMode);
	}

	if (GEditor->EditorUserSettings->bDisplayActionListItemRefIds && ActionPtr.IsValid())
	{
		check(InlineRenameWidget.IsValid());
		TSharedPtr<SToolTip> ExistingToolTip = InlineRenameWidget->GetToolTip();

		DisplayWidget->AddSlot(0)
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::Visible)
				.ToolTip(ConstructToolTipWithActionPath(ActionPtr.Pin(), ExistingToolTip))
			];
	}

	return DisplayWidget.ToSharedRef();
}

//------------------------------------------------------------------------------
FText SBlueprintPaletteItem::GetDisplayText() const
{
	FString DisplayText = ActionPtr.Pin()->MenuDescription;

	TSharedPtr< FEdGraphSchemaAction > GraphAction = ActionPtr.Pin();
	if (GraphAction->GetTypeId() == FEdGraphSchemaAction_K2Enum::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2Enum* EnumAction = (FEdGraphSchemaAction_K2Enum*)GraphAction.Get();
		DisplayText = EnumAction->Enum->GetName();
	}
	return FText::FromString(DisplayText);
}

//------------------------------------------------------------------------------
bool SBlueprintPaletteItem::OnNameTextVerifyChanged(const FText& InNewText, FText& OutErrorMessage)
{
	FString TextAsString = InNewText.ToString();

	FName OriginalName;

	// Check if certain action names are unchanged.
	if (ActionPtr.Pin()->GetTypeId() == FEdGraphSchemaAction_K2Var::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2Var* VarAction = (FEdGraphSchemaAction_K2Var*)ActionPtr.Pin().Get();
		OriginalName = (VarAction->GetVariableName());
	}
	else
	{
		UEdGraph* Graph = NULL;

		if(ActionPtr.Pin()->GetTypeId() == FEdGraphSchemaAction_K2Graph::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2Graph* GraphAction = (FEdGraphSchemaAction_K2Graph*)ActionPtr.Pin().Get();
			Graph = GraphAction->EdGraph;
		}
		else if(ActionPtr.Pin()->GetTypeId() == FEdGraphSchemaAction_K2Delegate::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2Delegate* DelegateAction = (FEdGraphSchemaAction_K2Delegate*)ActionPtr.Pin().Get();
			Graph = DelegateAction->EdGraph;
		}

		if (Graph)
		{
			OriginalName = Graph->GetFName();
		}
	}

	UBlueprint* Blueprint = BlueprintEditorPtr.Pin()->GetBlueprintObj();
	check(Blueprint != NULL);

	if(Blueprint->SimpleConstructionScript != NULL)
	{
		TArray<USCS_Node*> Nodes = Blueprint->SimpleConstructionScript->GetAllNodes();
		for (TArray<USCS_Node*>::TConstIterator NodeIt(Nodes); NodeIt; ++NodeIt)
		{
			USCS_Node* Node = *NodeIt;
			if (Node->VariableName == OriginalName && !Node->IsValidVariableNameString(InNewText.ToString()))
			{
				OutErrorMessage = LOCTEXT("RenameFailed_NotValid", "This name is reserved for engine use.");
				return false;
			}
		}
	}

	TSharedPtr<INameValidatorInterface> NameValidator = MakeShareable(new FKismetNameValidator(Blueprint, OriginalName));

	EValidatorResult ValidatorResult = NameValidator->IsValid(TextAsString);
	if(ValidatorResult == EValidatorResult::AlreadyInUse)
	{
		OutErrorMessage = FText::Format(LOCTEXT("RenameFailed_InUse", "{0} is in use by another variable or function!"), InNewText);
	}
	else if(ValidatorResult == EValidatorResult::EmptyName)
	{
		OutErrorMessage = LOCTEXT("RenameFailed_LeftBlank", "Names cannot be left blank!");
	}
	else if(ValidatorResult == EValidatorResult::TooLong)
	{
		OutErrorMessage = LOCTEXT("RenameFailed_NameTooLong", "Names must have fewer than 100 characters!");
	}

	if(OutErrorMessage.IsEmpty())
	{
		return true;
	}

	return false;
}

//------------------------------------------------------------------------------
void SBlueprintPaletteItem::OnNameTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
{
	if(ActionPtr.Pin()->GetTypeId() == FEdGraphSchemaAction_K2Graph::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2Graph* GraphAction = (FEdGraphSchemaAction_K2Graph*)ActionPtr.Pin().Get();

		UEdGraph* Graph = GraphAction->EdGraph;
		if ((Graph != NULL) && (Graph->bAllowDeletion || Graph->bAllowRenaming))
		{
			if (GraphAction->EdGraph)
			{
				FGraphDisplayInfo DisplayInfo;
				GraphAction->EdGraph->GetSchema()->GetGraphDisplayInformation(*GraphAction->EdGraph, DisplayInfo);

				// Check if the name is unchanged
				if( NewText.EqualTo( DisplayInfo.DisplayName ) )
				{
					return;
				}
			}

			// Make sure we aren't renaming the graph into something
			UEdGraph* ExistingGraph = FindObject<UEdGraph>(Graph->GetOuter(), *NewText.ToString() );
			if (ExistingGraph == NULL)
			{
				const FScopedTransaction Transaction( LOCTEXT( "Rename Function", "Rename Function" ) );
				FString NewNameString = NewText.ToString();
				FBlueprintEditorUtils::RenameGraph(Graph, *NewNameString );
			}
		}
	}
	else if(ActionPtr.Pin()->GetTypeId() == FEdGraphSchemaAction_K2Delegate::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2Delegate* DelegateAction = (FEdGraphSchemaAction_K2Delegate*)ActionPtr.Pin().Get();

		UEdGraph* Graph = DelegateAction->EdGraph;
		if ((Graph != NULL) && (Graph->bAllowDeletion || Graph->bAllowRenaming))
		{
			if (Graph)
			{
				FGraphDisplayInfo DisplayInfo;
				Graph->GetSchema()->GetGraphDisplayInformation(*Graph, DisplayInfo);

				// Check if the name is unchanged
				if( NewText.EqualTo( DisplayInfo.DisplayName ) )
				{
					return;
				}
			}

			// Make sure we aren't renaming the graph into something
			UEdGraph* ExistingGraph = FindObject<UEdGraph>(Graph->GetOuter(), *NewText.ToString() );
			if (ExistingGraph == NULL)
			{
				const FScopedTransaction Transaction( LOCTEXT( "Rename Delegate", "Rename Event Dispatcher" ) );
				const FString NewNameString = NewText.ToString();
				const FName NewName = FName(*NewNameString);
				const FName OldName =  Graph->GetFName();

				UBlueprint* Blueprint = BlueprintEditorPtr.Pin()->GetBlueprintObj();
				FBlueprintEditorUtils::RenameMemberVariable(Blueprint, OldName, NewName);

				TArray<UK2Node_BaseMCDelegate*> NodeUsingDelegate;
				FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_BaseMCDelegate>(Blueprint, NodeUsingDelegate);
				for (auto FuncIt = NodeUsingDelegate.CreateIterator(); FuncIt; ++FuncIt)
				{
					UK2Node_BaseMCDelegate* FunctionNode = *FuncIt;
					if (FunctionNode->DelegateReference.IsSelfContext() && (FunctionNode->DelegateReference.GetMemberName() == OldName))
					{
						FunctionNode->Modify();
						FunctionNode->DelegateReference.SetSelfMember(NewName);
					}
				}

				FBlueprintEditorUtils::RenameGraph(Graph, *NewNameString );
			}
		}
	}
	else if (ActionPtr.Pin()->GetTypeId() == FEdGraphSchemaAction_K2Var::StaticGetTypeId())
	{
		FEdGraphSchemaAction_K2Var* VarAction = (FEdGraphSchemaAction_K2Var*)ActionPtr.Pin().Get();

		// Check if the name is unchanged
		if(NewText.ToString() == VarAction->GetVariableName().ToString())
		{
			return;
		}

		const FScopedTransaction Transaction( LOCTEXT( "RenameVariable", "Rename Variable" ) );

		BlueprintEditorPtr.Pin()->GetBlueprintObj()->Modify();

		FName NewVarName = FName(*NewText.ToString());

		// Double check we're not renaming a timeline disguised as a variable
		bool bIsTimeline = false;
		UProperty* VariableProperty = VarAction->GetProperty();
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
			FBlueprintEditorUtils::RenameTimeline(BlueprintEditorPtr.Pin()->GetBlueprintObj(), VarAction->GetVariableName(), NewVarName);
		}
		else
		{
			FBlueprintEditorUtils::RenameMemberVariable(BlueprintEditorPtr.Pin()->GetBlueprintObj(), VarAction->GetVariableName(), NewVarName);
		}
	}
	BlueprintEditorPtr.Pin()->GetMyBlueprintWidget()->SelectItemByName(FName(*NewText.ToString()), ESelectInfo::OnMouseClick);
}

//------------------------------------------------------------------------------
FText SBlueprintPaletteItem::GetToolTipText() const
{
	TSharedPtr<FEdGraphSchemaAction> PaletteAction = ActionPtr.Pin();

	FString ToolTipText;
	FString ClassDisplayName;

	if (PaletteAction.IsValid())
	{
		// Default tooltip is taken from the action
		ToolTipText = (PaletteAction->TooltipDescription.Len() > 0) ? PaletteAction->TooltipDescription : PaletteAction->MenuDescription;

		if(PaletteAction->GetTypeId() == FEdGraphSchemaAction_K2AddComponent::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2AddComponent* AddCompAction = (FEdGraphSchemaAction_K2AddComponent*)PaletteAction.Get();
			// Show component-specific tooltip
			UClass* ComponentClass = *(AddCompAction->ComponentClass);
			if (ComponentClass)
			{
				ToolTipText = ComponentClass->GetToolTipText().ToString();
			}
		}
		else if (UK2Node const* const NodeTemplate = FK2SchemaActionUtils::ExtractNodeTemplateFromAction(PaletteAction))
		{
			// If the node wants to create tooltip text, use that instead, because its probably more detailed
			FString NodeToolTipText = NodeTemplate->GetTooltip();
			if (NodeToolTipText.Len() > 0)
			{
				ToolTipText = NodeToolTipText;
			}

			if (UK2Node_CallFunction const* CallFuncNode = Cast<UK2Node_CallFunction const>(NodeTemplate))
			{			
				if(UClass* ParentClass = CallFuncNode->FunctionReference.GetMemberParentClass(CallFuncNode))
				{
					UBlueprint* Blueprint = UBlueprint::GetBlueprintFromClass(ParentClass);
					ClassDisplayName = (Blueprint != NULL) ? Blueprint->GetName() : ParentClass->GetName();
				}
			}
		}
		else if (PaletteAction->GetTypeId() == FEdGraphSchemaAction_K2Graph::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2Graph* GraphAction = (FEdGraphSchemaAction_K2Graph*)PaletteAction.Get();
			if (GraphAction->EdGraph != NULL)
			{
				FGraphDisplayInfo DisplayInfo;
				GraphAction->EdGraph->GetSchema()->GetGraphDisplayInformation(*(GraphAction->EdGraph), DisplayInfo);

				ToolTipText = DisplayInfo.Tooltip;
			}
		}
		else if (PaletteAction->GetTypeId() == FEdGraphSchemaAction_K2Var::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2Var* VarAction = (FEdGraphSchemaAction_K2Var*)PaletteAction.Get();
			UClass* VarClass = VarAction->GetVariableClass();
			if (bShowClassInTooltip && (VarClass != NULL))
			{
				UBlueprint* Blueprint = UBlueprint::GetBlueprintFromClass(VarClass);
				ClassDisplayName = (Blueprint != NULL) ? Blueprint->GetName() : VarClass->GetName();
			}
			else
			{
				FString Result = GetVarTooltip(BlueprintEditorPtr.Pin()->GetBlueprintObj(), VarClass, VarAction->GetVariableName());
				// Only use the variable tooltip if it has been filled out.
				ToolTipText = !Result.IsEmpty() ? Result : GetVarType(VarClass, VarAction->GetVariableName(), true);
			}
		}
		else if (PaletteAction->GetTypeId() == FEdGraphSchemaAction_K2Delegate::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2Delegate* DelegateAction = (FEdGraphSchemaAction_K2Delegate*)PaletteAction.Get();
			
			FString Result = GetVarTooltip(BlueprintEditorPtr.Pin()->GetBlueprintObj(), DelegateAction->GetDelegateClass(), DelegateAction->GetDelegateName());
			ToolTipText = !Result.IsEmpty() ? Result : DelegateAction->GetDelegateName().ToString();
		}
		else if (PaletteAction->GetTypeId() == FEdGraphSchemaAction_K2Enum::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2Enum* EnumAction = (FEdGraphSchemaAction_K2Enum*)PaletteAction.Get();
			if (EnumAction->Enum)
			{
				ToolTipText = EnumAction->Enum->GetName();
			}
		}
		else if (PaletteAction->GetTypeId() == FEdGraphSchemaAction_K2TargetNode::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2TargetNode* TargetNodeAction = (FEdGraphSchemaAction_K2TargetNode*)PaletteAction.Get();
			if (TargetNodeAction->NodeTemplate)
			{
				ToolTipText = TargetNodeAction->NodeTemplate->GetTooltip();
			}
		}
	}

	if (bShowClassInTooltip && !ClassDisplayName.IsEmpty())
	{
		FString Format = LOCTEXT("BlueprintItemClassTooltip", "%s\nClass: %s").ToString();
		ToolTipText = FString::Printf(*Format, *ToolTipText, *ClassDisplayName);
	}

	return FText::FromString(ToolTipText);
}

TSharedPtr<SToolTip> SBlueprintPaletteItem::ConstructToolTipWidget() const
{
	TSharedPtr<FEdGraphSchemaAction> PaletteAction = ActionPtr.Pin();

	FString DocLink, DocExcerptName;

	if (PaletteAction.IsValid())
	{
		if (UEdGraphNode const* const NodeTemplate = FK2SchemaActionUtils::ExtractNodeTemplateFromAction(PaletteAction))
		{
			// Take rich tooltip from node
			DocLink = NodeTemplate->GetDocumentationLink();
			DocExcerptName = NodeTemplate->GetDocumentationExcerptName();
		}
		else if (PaletteAction->GetTypeId() == FEdGraphSchemaAction_K2Graph::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2Graph* GraphAction = (FEdGraphSchemaAction_K2Graph*)PaletteAction.Get();
			if (GraphAction->EdGraph != NULL)
			{
				FGraphDisplayInfo DisplayInfo;
				GraphAction->EdGraph->GetSchema()->GetGraphDisplayInformation(*(GraphAction->EdGraph), DisplayInfo);

				DocLink = DisplayInfo.DocLink;
				DocExcerptName = DisplayInfo.DocExcerptName;
			}
		}
		else if (PaletteAction->GetTypeId() == FEdGraphSchemaAction_K2Var::StaticGetTypeId())
		{
			FEdGraphSchemaAction_K2Var* VarAction = (FEdGraphSchemaAction_K2Var*)PaletteAction.Get();
			UClass* VarClass = VarAction->GetVariableClass();
			if (!bShowClassInTooltip || VarClass == NULL)
			{
				// Don't show big tooltip if we are showing class as well (means we are not in MyBlueprint)
				DocLink = TEXT("Shared/Editors/BlueprintEditor/GraphTypes");
				DocExcerptName = TEXT("Variable");
			}
		}
		else if (PaletteAction->GetTypeId() == FEdGraphSchemaAction_K2Event::StaticGetTypeId())
		{
			DocLink = TEXT("Shared/Editors/BlueprintEditor/GraphTypes");
			DocExcerptName = TEXT("Event");
		}
		else if (PaletteAction->GetTypeId() == FEdGraphSchemaAction_K2AddComment::StaticGetTypeId() ||
			PaletteAction->GetTypeId() == FEdGraphSchemaAction_NewStateComment::StaticGetTypeId())
		{
			// Taking tooltip from action is fine
			const UEdGraphNode_Comment* DefaultComment = GetDefault<UEdGraphNode_Comment>();
			DocLink = DefaultComment->GetDocumentationLink();
			DocExcerptName = DefaultComment->GetDocumentationExcerptName();
		}
	}

	// Setup the attribute for dynamically pulling the tooltip
	TAttribute<FText> TextAttribute;
	TextAttribute.Bind(this, &SBlueprintPaletteItem::GetToolTipText);

	return IDocumentation::Get()->CreateToolTip(TextAttribute, NULL, DocLink, DocExcerptName);
}

/*******************************************************************************
* SBlueprintPalette
*******************************************************************************/

//------------------------------------------------------------------------------
void SBlueprintPalette::Construct(const FArguments& InArgs, TWeakPtr<FBlueprintEditor> InBlueprintEditor)
{
	// Create the asset discovery indicator
	FEditorWidgetsModule& EditorWidgetsModule = FModuleManager::LoadModuleChecked<FEditorWidgetsModule>("EditorWidgets");
	TSharedRef<SWidget> AssetDiscoveryIndicator = EditorWidgetsModule.CreateAssetDiscoveryIndicator(EAssetDiscoveryIndicatorScaleMode::Scale_Vertical);

	float FavoritesHeightRatio = 0.33f;
	GConfig->GetFloat(*BlueprintPalette::ConfigSection, *BlueprintPalette::FavoritesHeightConfigKey, FavoritesHeightRatio, GEditorUserSettingsIni);
	float LibraryHeightRatio = 1.f - FavoritesHeightRatio;
	GConfig->GetFloat(*BlueprintPalette::ConfigSection, *BlueprintPalette::LibraryHeightConfigKey, LibraryHeightRatio, GEditorUserSettingsIni);

	bool bUseLegacyLayout = false;
	GConfig->GetBool(*BlueprintPalette::ConfigSection, TEXT("bUseLegacyLayout"), bUseLegacyLayout, GEditorIni);

	if (bUseLegacyLayout)
	{
		this->ChildSlot
		[
			SAssignNew(LibraryWrapper, SBlueprintLibraryPalette, InBlueprintEditor)
				.UseLegacyLayout(bUseLegacyLayout)
		];
	}
	else 
	{
		this->ChildSlot
		[
			SNew(STutorialWrapper)
			.Name(TEXT("FullBlueprintPalette"))
			.Content()
			[
				SAssignNew(PaletteSplitter, SSplitter)
					.Orientation(Orient_Vertical)
					.OnSplitterFinishedResizing(this, &SBlueprintPalette::OnSplitterResized)

				+SSplitter::Slot()
					.Value(FavoritesHeightRatio)
				[
					SAssignNew(FavoritesWrapper, STutorialWrapper)
					.Name(TEXT("BlueprintPaletteFavorites"))
					.Content()
					[
						SNew(SBlueprintFavoritesPalette, InBlueprintEditor)
					]
				]

				+SSplitter::Slot()
					.Value(LibraryHeightRatio)
				[
					SAssignNew(LibraryWrapper, STutorialWrapper)
					.Name(TEXT("BlueprintPaletteLibrary"))
					.Content()
					[
						SNew(SBlueprintLibraryPalette, InBlueprintEditor)
					]
				]
			]
		];
	}	
}

//------------------------------------------------------------------------------
void SBlueprintPalette::OnSplitterResized() const
{
	FChildren const* const SplitterChildren = PaletteSplitter->GetChildren();
	for (int32 SlotIndex = 0; SlotIndex < SplitterChildren->Num(); ++SlotIndex)
	{
		SSplitter::FSlot const& SplitterSlot = PaletteSplitter->SlotAt(SlotIndex);

		if (SplitterSlot.Widget == FavoritesWrapper)
		{
			GConfig->SetFloat(*BlueprintPalette::ConfigSection, *BlueprintPalette::FavoritesHeightConfigKey, SplitterSlot.SizeValue.Get(), GEditorUserSettingsIni);
		}
		else if (SplitterSlot.Widget == LibraryWrapper)
		{
			GConfig->SetFloat(*BlueprintPalette::ConfigSection, *BlueprintPalette::LibraryHeightConfigKey, SplitterSlot.SizeValue.Get(), GEditorUserSettingsIni);
		}

	}
}

#undef LOCTEXT_NAMESPACE
