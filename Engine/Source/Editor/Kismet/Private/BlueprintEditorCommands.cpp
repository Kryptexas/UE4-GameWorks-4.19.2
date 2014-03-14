// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintEditorPrivatePCH.h"
#include "Slate.h"
#include "BlueprintEditorCommands.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "Editor/BlueprintGraph/Public/K2ActionMenuBuilder.h" // for AddEventForFunction(), ...

struct FBlueprintPaletteListBuilder;

/** UI_COMMAND takes long for the compile to optimize */
PRAGMA_DISABLE_OPTIMIZATION
void FBlueprintEditorCommands::RegisterCommands()
{
	// Edit commands
	UI_COMMAND( FindInBlueprint, "Search", "Finds references to functions, events, variables, and pins in the current Blueprint", EUserInterfaceActionType::Button, FInputGesture(EModifierKey::Control, EKeys::F) );
	UI_COMMAND( FindInBlueprints, "Find in Blueprints", "Find references to functions, events and variables in ALL Blueprints", EUserInterfaceActionType::Button, FInputGesture(EModifierKey::Control | EModifierKey::Shift, EKeys::F) );
	UI_COMMAND( ReparentBlueprint, "Reparent Blueprint", "Change the parent of this Blueprint", EUserInterfaceActionType::Button, FInputGesture() );

	UI_COMMAND( CompileBlueprint, "Compile", "Compile the blueprint", EUserInterfaceActionType::Button, FInputGesture(EKeys::F7) );
	UI_COMMAND( RefreshAllNodes, "Refresh All nodes", "Refreshes all nodes in the graph to account for external changes", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( DeleteUnusedVariables, "Delete Unused Variables", "Deletes any variables that are never used", EUserInterfaceActionType::Button, FInputGesture() );

	UI_COMMAND( FindReferencesFromClass, "List references (from class)", "Find all objects that the class references", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( FindReferencesFromBlueprint, "List referenced (from blueprint)", "Find all objects that the blueprint references", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( RepairCorruptedBlueprint, "Repair corrupted blueprint", "Attempts to repair a corrupted blueprint that cannot be saved", EUserInterfaceActionType::Button, FInputGesture() );

	// View commands 
	UI_COMMAND( ZoomToWindow, "Zoom to Graph Extents", "Fit the current view to the entire graph", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( ZoomToSelection, "Zoom to Selection", "Fit the current view to the selection", EUserInterfaceActionType::Button, FInputGesture(EKeys::Home) );
	UI_COMMAND( NavigateToParent, "Go to parent graph", "Open the parent graph", EUserInterfaceActionType::Button, FInputGesture(EKeys::PageUp) );
	UI_COMMAND( NavigateToParentBackspace, "Go to parent graph", "Open the parent graph", EUserInterfaceActionType::Button, FInputGesture(EKeys::BackSpace) );
	UI_COMMAND( NavigateToChild, "Go to child graph", "Open the child graph", EUserInterfaceActionType::Button, FInputGesture(EKeys::PageDown) );

	// Preview commands
	UI_COMMAND( ResetCamera, "Reset Camera", "Resets the camera to focus on the mesh", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( EnableSimulation, "Simulation", "Enables the simulation of the blueprint and ticking", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( ShowFloor, "Show Floor", "Toggles a ground mesh for collision", EUserInterfaceActionType::ToggleButton, FInputGesture() );
	UI_COMMAND( ShowGrid, "Show Grid", "Toggles viewport grid", EUserInterfaceActionType::ToggleButton, FInputGesture() );

	// Debugging commands
	UI_COMMAND( EnableAllBreakpoints,"Enable All Breakpoints", "Enable all breakpoints", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( DisableAllBreakpoints, "Disable All Breakpoints", "Disable all breakpoints", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( ClearAllBreakpoints, "Delete All Breakpoints", "Delete all breakpoints", EUserInterfaceActionType::Button, FInputGesture(EModifierKey::Control | EModifierKey::Shift, EKeys::F9) );
	UI_COMMAND(	ClearAllWatches, "Delete All Watches", "Delete all watches", EUserInterfaceActionType::Button, FInputGesture() );

	// New documents
	UI_COMMAND( AddNewVariable, "Variable", "Adds a new variable to this blueprint.", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( AddNewLocalVariable, "Local Variable", "Adds a new local variable to this graph.", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( AddNewFunction, "Function", "Add a new function graph", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND(	AddNewMacroDeclaration, "Macro", "Add a new macro declaration graph", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( AddNewAnimationGraph, "Anim Graph", "Add a new animation graph", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( AddNewEventGraph, "Graph", "Add a new event graph", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( AddNewDelegate, "Event Dispatcher", "Add a new event dispatcher", EUserInterfaceActionType::Button, FInputGesture() );

	// Development commands
	UI_COMMAND( SaveIntermediateBuildProducts, "Save Intermediate Build Products", "Should the compiler save intermediate build products for debugging.", EUserInterfaceActionType::Check, FInputGesture() );

	UI_COMMAND( RecompileGraphEditor, "Recompile Graph Editor", "Recompiles and reloads C++ code for the graph editor", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( RecompileKismetCompiler, "Recompile Blueprint Compiler", "Recompiles and reloads C++ code for the blueprint compiler", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( RecompileBlueprintEditor, "Recompile Blueprint Editor", "Recompiles and reloads C++ code for the blueprint editor", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( RecompilePersona, "Recompile Persona", "Recompiles and reloads C++ code for Persona", EUserInterfaceActionType::Button, FInputGesture() );
}

PRAGMA_ENABLE_OPTIMIZATION

namespace NodeSpawnInfoHelpers
{
	/**
	 * Checks if the passed in function is available as an event for the Blueprint
	 *
	 * @param InBlueprint		The Blueprint to check for validity with
	 * @param InFunction		The Function to check for being available as an event
	 *
	 * @return					Returns true if the function is available as an event in the Blueprint
	 */
	bool IsFunctionAvailableAsEvent(const UBlueprint* InBlueprint, const UFunction* InFunction)
	{
		// Build a list of all interface classes either implemented by this blueprint or through inheritance
		TArray<UClass*> InterfaceClasses;
		FBlueprintEditorUtils::FindImplementedInterfaces(InBlueprint, true, InterfaceClasses);
		InterfaceClasses.Add(InBlueprint->ParentClass);

		// Grab the list of events to be excluded from the override list
		const FString ExclusionListKeyName = TEXT("KismetHideOverrides");
		TArray<FString> ExcludedEventNames;
		if( InBlueprint->ParentClass->HasMetaData(*ExclusionListKeyName) )
		{
			const FString ExcludedEventNameString = InBlueprint->ParentClass->GetMetaData(*ExclusionListKeyName);
			ExcludedEventNameString.ParseIntoArray(&ExcludedEventNames, TEXT(","), true);
		}

		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		if (K2Schema->FunctionCanBePlacedAsEvent(InFunction) && !ExcludedEventNames.Contains(InFunction->GetName()))
		{
			// Check all potential interface events using the class list we build above
			for(auto It = InterfaceClasses.CreateConstIterator(); It; It++)
			{
				const UClass* Interface = (*It);
				for (TFieldIterator<UFunction> FunctionIt(Interface, EFieldIteratorFlags::IncludeSuper); FunctionIt; ++FunctionIt)
				{
					UFunction* Function = *FunctionIt;

					if(Function->GetName() == InFunction->GetName())
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	/**
	 * Finds the event version of a UFunction for a given Blueprint
	 *
	 * @param InBlueprint			The Blueprint to find the function within
	 * @param InFunction			The Function to find an event version of
	 *
	 * @return						Event Function used by the given Blueprint for the given Function
	 */
	UFunction* FindEventFunctionForClass(const UBlueprint* InBlueprint, const UFunction* InFunction)
	{
		// Look at all of the Blueprint parent's functions for an event
		for (TFieldIterator<UFunction> FunctionIt(InBlueprint->ParentClass, EFieldIteratorFlags::IncludeSuper); FunctionIt; ++FunctionIt)
		{
			UFunction* Function = *FunctionIt;
			if(Function->GetName() == InFunction->GetName())
			{
				return Function;
			}
		}

		return NULL;
	}
}

class FNodeSpawnInfo
{
public:
	/** Holds the UI Command to verify gestures for this action are held */
	TSharedPtr< FUICommandInfo > CommandInfo;

	/**
	 * Creates an action to be used for placing a node into the graph
	 *
	 * @param	InPaletteBuilder	The Blueprint palette the action should be created for
	 * @param	InDestGraph			The graph the action should be created for
	 *
	 * @return						A fully prepared action containing the information to spawn the node
	 */
	virtual TSharedPtr< FEdGraphSchemaAction > GetAction(FBlueprintPaletteListBuilder& InPaletteBuilder, UEdGraph* InDestGraph) = 0;
};

class FEdGraphNodeSpawnInfo : public FNodeSpawnInfo
{
public:
	FEdGraphNodeSpawnInfo(UClass* InClass) : NodeClass(InClass), GraphNode(NULL) {}

	// FNodeSpawnInfo interface
	virtual TSharedPtr< FEdGraphSchemaAction > GetAction(FBlueprintPaletteListBuilder& InPaletteBuilder, UEdGraph* InDestGraph) OVERRIDE
	{
		TSharedPtr<FEdGraphSchemaAction_NewNode> NewActionNode = TSharedPtr<FEdGraphSchemaAction_NewNode>(new FEdGraphSchemaAction_NewNode);

		if (!GraphNode.IsValid())
		{
			GraphNode = InPaletteBuilder.CreateTemplateNode<UEdGraphNode>(NodeClass);
		}
		ensureMsgf( GraphNode->GetClass() == NodeClass, TEXT("FEdGraphNodeSpawnInfo::GetAction() class mismatch (GraphNode class %s, NodeClass %s"), *GraphNode->GetClass()->GetName(), *NodeClass->GetName() );

		// the NodeTemplate UPROPERTY takes ownership of GraphNode's lifetime (hence it being a weak-pointer here)
		NewActionNode->NodeTemplate = GraphNode.Get();

		return NewActionNode;
	}
	// End of FNodeSpawnInfo interface

private:
	/** The class type the node should be */
	UClass* NodeClass;

	/** The graph node to place into the action to spawn it in the graph */
	TWeakObjectPtr<UEdGraphNode> GraphNode;
};

class FFunctionNodeSpawnInfo : public FNodeSpawnInfo
{
public:
	FFunctionNodeSpawnInfo(UFunction* InFunctionPtr) : FunctionPtr(InFunctionPtr) {}

	// FNodeSpawnInfo interface
	virtual TSharedPtr< FEdGraphSchemaAction > GetAction(FBlueprintPaletteListBuilder& InPaletteBuilder, UEdGraph* InDestGraph) OVERRIDE
	{
		const UBlueprint* Blueprint = InPaletteBuilder.Blueprint;
		FBlueprintPaletteListBuilder TempListBuilder(Blueprint);

		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		if(NodeSpawnInfoHelpers::IsFunctionAvailableAsEvent(Blueprint, FunctionPtr))
		{
			UFunction* FunctionEvent = NodeSpawnInfoHelpers::FindEventFunctionForClass(Blueprint, FunctionPtr);
			if(FunctionEvent)
			{
				FK2ActionMenuBuilder::AddEventForFunction(TempListBuilder, FunctionEvent);	
			}
		}
		else
		{
			const bool bAllowImpureFuncs = K2Schema->DoesGraphSupportImpureFunctions(InDestGraph);

			uint32 FunctionTypes = UEdGraphSchema_K2::EFunctionType::FT_Pure | UEdGraphSchema_K2::EFunctionType::FT_Const | UEdGraphSchema_K2::EFunctionType::FT_Protected;
			if(bAllowImpureFuncs)
			{
				FunctionTypes |= UEdGraphSchema_K2::EFunctionType::FT_Imperative;
			}

			if(K2Schema->CanFunctionBeUsedInClass(Blueprint->GeneratedClass, FunctionPtr, InDestGraph, FunctionTypes, true, false, FFunctionTargetInfo()))
			{
				FK2ActionMenuBuilder::AddSpawnInfoForFunction(FunctionPtr, false, FFunctionTargetInfo(), FMemberReference(), TEXT(""), K2Schema->AG_LevelReference, TempListBuilder);
			}
		}

		TSharedPtr<FEdGraphSchemaAction> NewActionNode;
		if(TempListBuilder.GetNumActions() == 1)
		{
			TArray<UEdGraphPin*> DummyPins;
			FGraphActionListBuilderBase::ActionGroup& Action = TempListBuilder.GetAction(0);
			NewActionNode = Action.Actions[0];
		}

		return NewActionNode;
	}
	// End of FNodeSpawnInfo interface

private:
	/** The function used to create the action to spawn the graph node */
	UFunction* FunctionPtr;
};

class FMacroNodeSpawnInfo : public FNodeSpawnInfo
{
public:
	FMacroNodeSpawnInfo(UEdGraph* InMacroGraph) : MacroGraph(InMacroGraph) {}

	// FNodeSpawnInfo interface
	virtual TSharedPtr< FEdGraphSchemaAction > GetAction(FBlueprintPaletteListBuilder& InPaletteBuilder, UEdGraph* InDestGraph) OVERRIDE
	{
		const UBlueprint* Blueprint = InPaletteBuilder.Blueprint;
		FBlueprintPaletteListBuilder TempListBuilder(Blueprint);

		FK2ActionMenuBuilder::AttachMacroGraphAction(TempListBuilder, MacroGraph);

		FGraphActionListBuilderBase::ActionGroup& Action = TempListBuilder.GetAction(0);
		return Action.Actions[0];
	}
	// End of FNodeSpawnInfo interface

private:
	/** The macro graph used to create the action to spawn the graph node */
	UEdGraph* MacroGraph;
};

//////////////////////////////////////////////////////////////////////////
// FBlueprintSpawnNodeCommands

void FBlueprintSpawnNodeCommands::RegisterCommands()
{
	const FString ConfigSection = TEXT("BlueprintSpawnNodes");
	const FString SettingName = TEXT("Node");
	TArray< FString > NodeSpawns;
	GConfig->GetArray(*ConfigSection, *SettingName, NodeSpawns, GEditorUserSettingsIni);

	for(int32 x = 0; x < NodeSpawns.Num(); ++x)
	{
		FString ClassName;
		if(!FParse::Value(*NodeSpawns[x], TEXT("Class="), ClassName))
		{
			// Could not find a class name, cannot continue with this line
			continue;
		}

		FString CommandLabel;
		UClass* FoundClass = FindObject<UClass>(ANY_PACKAGE, *ClassName, true);
		TSharedPtr< FNodeSpawnInfo > InfoPtr;

		if(FoundClass && FoundClass->IsChildOf(UEdGraphNode::StaticClass()))
		{
			// The class name matches that of a UEdGraphNode, so setup a spawn info that can generate UEdGraphNode graph actions
			UEdGraphNode* GraphNode = Cast<UEdGraphNode>(FoundClass->GetDefaultObject());

			CommandLabel = GraphNode->GetNodeTitle(ENodeTitleType::ListView);

			if(CommandLabel.Len() == 0)
			{
				CommandLabel = FoundClass->GetName();
			}

			InfoPtr = MakeShareable( new FEdGraphNodeSpawnInfo( FoundClass ) );
		}
		else if(UFunction* FoundFunction = FindObject<UFunction>(ANY_PACKAGE, *ClassName, true))
		{
			// The class name matches that of a function, so setup a spawn info that can generate function graph actions
			InfoPtr = MakeShareable( new FFunctionNodeSpawnInfo((UFunction*)FoundFunction));

			CommandLabel = FoundFunction->GetName();
		}
		else
		{
			// Check for a macro graph that matches the passed in class name
			for (TObjectIterator<UBlueprint> BlueprintIt; BlueprintIt; ++BlueprintIt)
			{
				UBlueprint* MacroBP = *BlueprintIt;
				if(MacroBP->BlueprintType == BPTYPE_MacroLibrary)
				{
					// getting 'top-level' of the macros
					for (TArray<UEdGraph*>::TIterator GraphIt(MacroBP->MacroGraphs); GraphIt; ++GraphIt)
					{
						UEdGraph* MacroGraph = *GraphIt;
						// The class name matches that of a macro, so setup a spawn info that can generate macro graph actions
						if(MacroGraph->GetName() == ClassName)
						{
							CommandLabel = MacroGraph->GetName();

							InfoPtr = MakeShareable( new FMacroNodeSpawnInfo(MacroGraph));
						}

					}
				}
			}
		}

		// If spawn info was created, setup a UI Command for keybinding.
		if(InfoPtr.IsValid())
		{
			TSharedPtr< FUICommandInfo > CommandInfo;

			FKey Key;
			bool bShift = false;
			bool bCtrl = false;
			bool bAlt = false;
			
			// Parse the keybinding information
			FString KeyString;
			if( FParse::Value(*NodeSpawns[x], TEXT("Key="), KeyString) )
			{
				Key = *KeyString;
			}

			if( Key.IsValid() )
			{
				FParse::Bool(*NodeSpawns[x], TEXT("Shift="), bShift);
				FParse::Bool(*NodeSpawns[x], TEXT("Alt="), bAlt);
				FParse::Bool(*NodeSpawns[x], TEXT("Ctrl="), bCtrl);
			}

			FInputGesture Gesture(Key);
			Gesture.bAlt = bAlt;
			Gesture.bCtrl = bCtrl;
			Gesture.bShift = bShift;

			FText CommandLabelText = FText::FromString( CommandLabel );
			FText Description = FText::Format( NSLOCTEXT("BlueprintEditor", "NodeSpawnDescription", "Hold down the bound keys and left click in the graph panel to spawn a {0} node."), CommandLabelText );

			FUICommandInfo::MakeCommandInfo( this->AsShared(), CommandInfo, FName(*NodeSpawns[x]), CommandLabelText, Description, FSlateIcon(FEditorStyle::GetStyleSetName(), *FString::Printf(TEXT("%s.%s"), *this->GetContextName().ToString(), *NodeSpawns[x])), EUserInterfaceActionType::Button, Gesture );

			InfoPtr->CommandInfo = CommandInfo;

			NodeCommands.Add(InfoPtr);
		}
	}
}

TSharedPtr< FEdGraphSchemaAction > FBlueprintSpawnNodeCommands::GetGraphActionByGesture(FInputGesture& InGesture, FBlueprintPaletteListBuilder& InPaletteBuilder, UEdGraph* InDestGraph) const
{
	if(InGesture.IsValidGesture())
	{
		for(int32 x = 0; x < NodeCommands.Num(); ++x)
		{
			if(NodeCommands[x]->CommandInfo->GetActiveGesture().Get() == InGesture)
			{
				return NodeCommands[x]->GetAction(InPaletteBuilder, InDestGraph);
			}
		}
	}

	return TSharedPtr< FEdGraphSchemaAction >();
}
