// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "BlueprintActionMenuUtils.h"
#include "BlueprintActionMenuBuilder.h"
#include "BlueprintActionFilter.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintEditorUtils.h"	// for DoesSupportComponents()
#include "BlueprintPaletteFavorites.h"
#include "EdGraphSchema_K2.h"		// for bUseLegacyActionMenus 
#include "K2Node_ActorBoundEvent.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_MatineeController.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "KismetEditorUtilities.h"	// for CanPasteNodes()
#include "K2ActionMenuBuilder.h"

#define LOCTEXT_NAMESPACE "BlueprintActionMenuUtils"

/*******************************************************************************
* Static FBlueprintActionMenuUtils Helpers
******************************************************************************/

namespace BlueprintActionMenuUtilsImpl
{
	static int32 const MainMenuSectionGroup   = 000;
	static int32 const ComponentsSectionGroup = 100;
	static int32 const LevelActorSectionGroup = 101;

	/**
	 * Additional filter rejection test, for menu sections that only contain 
	 * bound actions. Rejects any action that is not bound.
	 * 
	 * @param  Filter			The filter querying this rejection test.
	 * @param  BlueprintAction	The action to test.
	 * @return True if the spawner is unbound (and should be rejected), otherwise false.
	 */
	static bool IsUnBoundSpawner(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);

	/**
	 * Filter rejection test, for favorite menus. Rejects any actions that are 
	 * not favorited by the user.
	 * 
	 * @param  Filter			The filter querying this rejection test.
	 * @param  BlueprintAction	The action to test.
	 * @return True if the spawner is not favorited (and should be rejected), otherwise false.
	 */
	static bool IsNonFavoritedAction(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);

	static bool IsPureNonConstAction(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);

	static bool IsUnexposedMemberAction(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);

	/**
	 * Utility function to find a common base class from a set of given classes.
	 * 
	 * @param  ObjectSet	The set of objects that you want a single base class for.
	 * @return A common base class for all the specified classes (falls back to UObject's class).
	 */
	static UClass* FindCommonBaseClass(TArray<UObject*> const& ObjectSet);

	/**
	 * 
	 * 
	 * @param  Pin	
	 * @return 
	 */
	static UClass* GetPinClassType(UEdGraphPin const* Pin);

	/**
	 * 
	 * 
	 * @param  ComponentsFilter	
	 * @param  MenuOut	
	 * @return 
	 */
	static void AddComponentSections(FBlueprintActionFilter const& ComponentsFilter, FBlueprintActionMenuBuilder& MenuOut);

	/**
	 * 
	 * 
	 * @param  LevelActorsFilter	
	 * @param  MenuOut	
	 * @return 
	 */
	static void AddLevelActorSections(FBlueprintActionFilter const& LevelActorsFilter, FBlueprintActionMenuBuilder& MenuOut);
}

//------------------------------------------------------------------------------
static bool BlueprintActionMenuUtilsImpl::IsUnBoundSpawner(FBlueprintActionFilter const& /*Filter*/, FBlueprintActionInfo& BlueprintAction)
{
	return (BlueprintAction.GetBindings().Num() <= 0);
}

//------------------------------------------------------------------------------
static bool BlueprintActionMenuUtilsImpl::IsNonFavoritedAction(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction)
{
	UEditorUserSettings& EditorUserSettings = GEditor->AccessEditorUserSettings();
	// grab the user's favorites
	UBlueprintPaletteFavorites const* BlueprintFavorites = EditorUserSettings.BlueprintFavorites;
	checkSlow(BlueprintFavorites != nullptr);

	return !BlueprintFavorites->IsFavorited(BlueprintAction);
}

//------------------------------------------------------------------------------
static bool BlueprintActionMenuUtilsImpl::IsPureNonConstAction(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction)
{
	bool bIsFliteredOut = false;

	if (UFunction const* Function = BlueprintAction.GetAssociatedFunction())
	{
		bool const bIsImperative = !Function->HasAnyFunctionFlags(FUNC_BlueprintPure);
		bool const bIsConstFunc  =  Function->HasAnyFunctionFlags(FUNC_Const);
		bIsFliteredOut = !bIsImperative && !bIsConstFunc;
	}
	return bIsFliteredOut;
}

//------------------------------------------------------------------------------
static bool BlueprintActionMenuUtilsImpl::IsUnexposedMemberAction(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction)
{
	bool bIsFliteredOut = false;

	if (UFunction const* Function = BlueprintAction.GetAssociatedFunction())
	{
		TArray<FString> AllExposedCategories;
		for (TWeakObjectPtr<UObject> Binding : BlueprintAction.GetBindings())
		{
			if (UObjectProperty* ObjectProperty = Cast<UObjectProperty>(Binding.Get()))
			{
				FString const ExposedCategoryMetadata = ObjectProperty->GetMetaData(FBlueprintMetadata::MD_ExposeFunctionCategories);
				if (ExposedCategoryMetadata.IsEmpty())
				{
					continue;
				}

				TArray<FString> PropertyExposedCategories;
				ExposedCategoryMetadata.ParseIntoArray(&PropertyExposedCategories, TEXT(","), true);
				AllExposedCategories.Append(PropertyExposedCategories);
			}
		}

		FString FunctionCategory = Function->GetMetaData(FBlueprintMetadata::MD_FunctionCategory);
		bIsFliteredOut = !AllExposedCategories.Contains(FunctionCategory);
	}
	return bIsFliteredOut;
}

//------------------------------------------------------------------------------
static UClass* BlueprintActionMenuUtilsImpl::FindCommonBaseClass(TArray<UObject*> const& ObjectSet)
{
	UClass* CommonClass = UObject::StaticClass();
	if (ObjectSet.Num() > 0)
	{
		CommonClass = ObjectSet[0]->GetClass();
		for (UObject const* Object : ObjectSet)
		{
			UClass* Class = Object->GetClass();
			while (!Class->IsChildOf(CommonClass))
			{
				CommonClass = CommonClass->GetSuperClass();
			}
		}
	}	
	return CommonClass;
}

//------------------------------------------------------------------------------
static UClass* BlueprintActionMenuUtilsImpl::GetPinClassType(UEdGraphPin const* Pin)
{
	UClass* PinObjClass = nullptr;

	FEdGraphPinType const& PinType = Pin->PinType;
	if ((PinType.PinCategory == UEdGraphSchema_K2::PC_Object) || 
		(PinType.PinCategory == UEdGraphSchema_K2::PC_Interface))
	{
		bool const bIsSelfPin = !PinType.PinSubCategoryObject.IsValid();
		if (bIsSelfPin)
		{
			PinObjClass = CastChecked<UK2Node>(Pin->GetOwningNode())->GetBlueprint()->SkeletonGeneratedClass;
		}
		else
		{
			PinObjClass = Cast<UClass>(PinType.PinSubCategoryObject.Get());
		}
	}

	return PinObjClass;
}

//------------------------------------------------------------------------------
static void BlueprintActionMenuUtilsImpl::AddComponentSections(FBlueprintActionFilter const& ComponentsFilter, FBlueprintActionMenuBuilder& MenuOut)
{
	FText EventSectionHeading = LOCTEXT("ComponentsEventCategory", "Add Event for Selected Components");
	FText FuncSectionHeading  = LOCTEXT("ComponentsFuncCategory",  "Call Function on Selected Components");

	if (ComponentsFilter.Context.SelectedObjects.Num() == 1)
	{
		FText const ComponentName = FText::FromName(ComponentsFilter.Context.SelectedObjects.Last()->GetFName());
		FuncSectionHeading  = FText::Format(LOCTEXT("SingleComponentFuncCategory", "Call Function on {0}"), ComponentName);
		EventSectionHeading = FText::Format(LOCTEXT("SingleComponentFuncCategory", "Add Event for {0}"), ComponentName);
	}

	FBlueprintActionFilter ComponentFunctionsFilter = ComponentsFilter;
	ComponentFunctionsFilter.PermittedNodeTypes.Add(UK2Node_CallFunction::StaticClass());
	MenuOut.AddMenuSection(ComponentFunctionsFilter, FuncSectionHeading, ComponentsSectionGroup, FBlueprintActionMenuBuilder::ConsolidateBoundActions);

	FBlueprintActionFilter ComponentEventsFilter = ComponentsFilter;
	ComponentEventsFilter.PermittedNodeTypes.Add(UK2Node_ComponentBoundEvent::StaticClass());
	MenuOut.AddMenuSection(ComponentEventsFilter, EventSectionHeading, ComponentsSectionGroup, FBlueprintActionMenuBuilder::ConsolidateBoundActions);
}

//------------------------------------------------------------------------------
static void BlueprintActionMenuUtilsImpl::AddLevelActorSections(FBlueprintActionFilter const& LevelActorsFilter, FBlueprintActionMenuBuilder& MenuOut)
{
	FText EventSectionHeading = LOCTEXT("ActorsEventCategory", "Add Event for Selected Actors");
	FText FuncSectionHeading  = LOCTEXT("ActorsFuncCategory",  "Call Function on Selected Actors");

	if (LevelActorsFilter.Context.SelectedObjects.Num() == 1)
	{
		FText const ActorName = FText::FromName(LevelActorsFilter.Context.SelectedObjects.Last()->GetFName());
		FuncSectionHeading  = FText::Format(LOCTEXT("SingleActorFuncCategory", "Call Function on {0}"), ActorName);
		EventSectionHeading = FText::Format(LOCTEXT("SingleActorFuncCategory", "Add Event for {0}"), ActorName);
	}

	FBlueprintActionFilter ActorFunctionsFilter = LevelActorsFilter;
	ActorFunctionsFilter.PermittedNodeTypes.Add(UK2Node_CallFunction::StaticClass());
	MenuOut.AddMenuSection(ActorFunctionsFilter, FuncSectionHeading, LevelActorSectionGroup, FBlueprintActionMenuBuilder::ConsolidateBoundActions);

	FBlueprintActionFilter ActorEventsFilter = LevelActorsFilter;
	ActorEventsFilter.PermittedNodeTypes.Add(UK2Node_ActorBoundEvent::StaticClass());
	MenuOut.AddMenuSection(ActorEventsFilter, EventSectionHeading, LevelActorSectionGroup, FBlueprintActionMenuBuilder::ConsolidateBoundActions);

	FBlueprintActionFilter ActorReferencesFilter = LevelActorsFilter;
	ActorReferencesFilter.RejectedNodeTypes.Append(ActorFunctionsFilter.PermittedNodeTypes);
	ActorReferencesFilter.RejectedNodeTypes.Append(ActorEventsFilter.PermittedNodeTypes);
	MenuOut.AddMenuSection(ActorReferencesFilter, FText::GetEmpty(), LevelActorSectionGroup, FBlueprintActionMenuBuilder::ConsolidateBoundActions);
}

/*******************************************************************************
 * FBlueprintActionMenuUtils
 ******************************************************************************/

//------------------------------------------------------------------------------
void FBlueprintActionMenuUtils::MakePaletteMenu(FBlueprintActionContext const& Context, UClass* FilterClass, FBlueprintActionMenuBuilder& MenuOut)
{
	MenuOut.Empty();
	
	uint32 FilterFlags = 0x00;
	if (FilterClass != nullptr)
	{
		// make sure we exclude global and static library actions
		FilterFlags |= FBlueprintActionFilter::BPFILTER_RejectGlobalFields;
	}
	
	FBlueprintActionFilter MenuFilter(FilterFlags);
	MenuFilter.Context = Context;
	
	// self member variables can be accessed through the MyBluprint panel (even
	// inherited ones)... external variables can be accessed through the context
	// menu (don't want to clutter the palette, I guess?)
	MenuFilter.RejectedNodeTypes.Add(UK2Node_VariableGet::StaticClass());
	MenuFilter.RejectedNodeTypes.Add(UK2Node_VariableSet::StaticClass());
	
	if (FilterClass != nullptr)
	{
		MenuFilter.TargetClasses.Add(FilterClass);
	}

	MenuOut.AddMenuSection(MenuFilter, LOCTEXT("PaletteRoot", "Library"), /*MenuOrder =*/0, FBlueprintActionMenuBuilder::ConsolidatePropertyActions);
	MenuOut.RebuildActionList();
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuUtils::MakeContextMenu(FBlueprintActionContext const& Context, bool bIsContextSensitive, FBlueprintActionMenuBuilder& MenuOut)
{
	using namespace BlueprintActionMenuUtilsImpl;

	//--------------------------------------
	// Composing Filters
	//--------------------------------------

	FBlueprintActionFilter MainMenuFilter;
	MainMenuFilter.Context = Context;
	MainMenuFilter.Context.SelectedObjects.Empty();

	FBlueprintActionFilter ComponentsFilter;
	ComponentsFilter.Context = Context;
	// only want bound actions for this menu section
	ComponentsFilter.AddRejectionTest(FBlueprintActionFilter::FRejectionTestDelegate::CreateStatic(IsUnBoundSpawner));
	// @TODO: don't know exactly why we can only bind non-pure/const functions;
	//        this is mirrored after FK2ActionMenuBuilder::GetFunctionCallsOnSelectedActors()
	//        and FK2ActionMenuBuilder::GetFunctionCallsOnSelectedComponents(),
	//        where we make the same stipulation
	ComponentsFilter.AddRejectionTest(FBlueprintActionFilter::FRejectionTestDelegate::CreateStatic(IsPureNonConstAction));
	

	FBlueprintActionFilter LevelActorsFilter;
	LevelActorsFilter.Context = Context;
	// only want bound actions for this menu section
	LevelActorsFilter.AddRejectionTest(FBlueprintActionFilter::FRejectionTestDelegate::CreateStatic(IsUnBoundSpawner));

	// make sure the bound menu sections have the proper OwnerClasses specified
	for (UObject* Selection : Context.SelectedObjects)
	{
		if (UObjectProperty* ObjProperty = Cast<UObjectProperty>(Selection))
		{
			LevelActorsFilter.Context.SelectedObjects.Remove(Selection);
		}
		else if (AActor* LevelActor = Cast<AActor>(Selection))
		{
			ComponentsFilter.Context.SelectedObjects.Remove(Selection);
		}
		else
		{
			ComponentsFilter.Context.SelectedObjects.Remove(Selection);
			LevelActorsFilter.Context.SelectedObjects.Remove(Selection);
		}
	}

	// make sure all selected level actors are accounted for (in case the caller
	// did not include them in the context)
	for (FSelectionIterator LvlActorIt(*GEditor->GetSelectedActors()); LvlActorIt; ++LvlActorIt)
	{
		AActor* LevelActor = Cast<AActor>(*LvlActorIt);
		LevelActorsFilter.Context.SelectedObjects.AddUnique(LevelActor);
	}

	bool const bAddTargetContext  = bIsContextSensitive;
	bool bCanOperateOnLevelActors = bIsContextSensitive;
	bool bCanHaveActorComponents  = bIsContextSensitive;
	// determine if we can operate on certain object selections (level actors, 
	// components, etc.)
	for (UBlueprint* Blueprint : Context.Blueprints)
	{
		UClass* BlueprintClass = Blueprint->SkeletonGeneratedClass;
		if (BlueprintClass != nullptr)
		{
			bCanOperateOnLevelActors &= BlueprintClass->IsChildOf<ALevelScriptActor>();
			if (bAddTargetContext)
			{
				MainMenuFilter.TargetClasses.Add(BlueprintClass);
			}
		}
		bCanHaveActorComponents &= FBlueprintEditorUtils::DoesSupportComponents(Blueprint);
	}

	if (bAddTargetContext)
	{
		bool bContextPinIsObj = false;

		// if we're dragging from a pin, we further extend the context to cover
		// that pin and any other pins it sits beside 
		for (UEdGraphPin* ContextPin : Context.Pins)
		{
			if (UClass* PinObjClass = GetPinClassType(ContextPin))
			{
				if (!bContextPinIsObj)
				{
					MainMenuFilter.TargetClasses.Empty();
				}
				MainMenuFilter.TargetClasses.Add(PinObjClass);
				bContextPinIsObj = true;
			}

			for (UEdGraphPin* NodePin : ContextPin->GetOwningNode()->Pins)
			{
				if ((NodePin->Direction == ContextPin->Direction) && !bContextPinIsObj)
				{
					if (UClass* PinClass = GetPinClassType(NodePin))
					{
						MainMenuFilter.TargetClasses.Add(PinClass);
					}
				}
			}
		}
	}

	FBlueprintActionFilter CallOnMemberFilter;
	CallOnMemberFilter.Context = MainMenuFilter.Context;
	CallOnMemberFilter.PermittedNodeTypes.Add(UK2Node_CallFunction::StaticClass());
	CallOnMemberFilter.AddRejectionTest(FBlueprintActionFilter::FRejectionTestDelegate::CreateStatic(IsUnBoundSpawner));
	// instead of looking for "ExposeFunctionCategories" on object properties,
	// we just expose functions for all components, this is problematic if we 
	// were using the "ExposeFunctionCategories" on any non-component object 
	// properties... if we re-enable this, then make sure to remove the 
	// IsChildOf<UActorComponent>() check below
	//CallOnMemberFilter.AddRejectionTest(FBlueprintActionFilter::FRejectionTestDelegate::CreateStatic(IsUnexposedMemberAction));

	for (UClass const* TargetClass : MainMenuFilter.TargetClasses)
	{
		for (TFieldIterator<UObjectProperty> PropertyIt(TargetClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			UObjectProperty* ComponentProperty = *PropertyIt;
			// @TODO: consider a more relaxed scenario where we allow other
			//        (non-component) properties as options here too
			if (!ComponentProperty->PropertyClass->IsChildOf<UActorComponent>() || 
				!ComponentProperty->HasAnyPropertyFlags(CPF_BlueprintVisible))
			{
				continue;
			}
			CallOnMemberFilter.Context.SelectedObjects.Add(ComponentProperty);
		}
	}

	//--------------------------------------
	// Defining Menu Sections
	//--------------------------------------	

	MenuOut.Empty();

	bool const bAddComponentsSection = bIsContextSensitive && bCanHaveActorComponents && (ComponentsFilter.Context.SelectedObjects.Num() > 0);
	// add the components section to the menu (if we don't have any components
	// selected, then inform the user through a dummy menu entry)
	if (bAddComponentsSection)
	{
		AddComponentSections(ComponentsFilter, MenuOut);
	}

	bool const bAddLevelActorsSection = bIsContextSensitive && bCanOperateOnLevelActors && (LevelActorsFilter.Context.SelectedObjects.Num() > 0);
	// add the level actor section to the menu
	if (bAddLevelActorsSection)
	{
		AddLevelActorSections(LevelActorsFilter, MenuOut);
	}

	if (bIsContextSensitive)
	{
		MenuOut.AddMenuSection(CallOnMemberFilter);
	}
	else
	{
		MainMenuFilter.Context.Pins.Empty();
	}
	MenuOut.AddMenuSection(MainMenuFilter);

	//--------------------------------------
	// Building the Menu
	//--------------------------------------

	MenuOut.RebuildActionList();

	UEditorExperimentalSettings const* ExperimentalSettings =  GetDefault<UEditorExperimentalSettings>();
	if (ExperimentalSettings->bUseRefactoredBlueprintMenuingSystem)
	{
		for (UEdGraph const* Graph : Context.Graphs)
		{
			if (FKismetEditorUtilities::CanPasteNodes(Graph))
			{
				// @TODO: Grey out menu option with tooltip if one of the nodes cannot paste into this graph
				TSharedPtr<FEdGraphSchemaAction> PasteHereAction(new FEdGraphSchemaAction_K2PasteHere(TEXT(""), LOCTEXT("PasteHereMenuName", "Paste here"), TEXT(""), MainMenuSectionGroup));
				MenuOut.AddAction(PasteHereAction);
				break;
			}
		}

		if (bIsContextSensitive && bCanHaveActorComponents && !bAddComponentsSection)
		{
			FText SelectComponentMsg = LOCTEXT("SelectComponentForEvents", "Select a Component to see available Events & Functions");
			FText SelectComponentToolTip = LOCTEXT("SelectComponentForEventsTooltip", "Select a Component in the MyBlueprint tab to see available Events and Functions in this menu.");
			TSharedPtr<FEdGraphSchemaAction> MsgAction = TSharedPtr<FEdGraphSchemaAction>(new FEdGraphSchemaAction_Dummy(TEXT(""), SelectComponentMsg, SelectComponentToolTip.ToString(), ComponentsSectionGroup));
			MenuOut.AddAction(MsgAction);
		}

		if (bIsContextSensitive && bCanOperateOnLevelActors && !bAddLevelActorsSection)
		{
			FText SelectActorsMsg = LOCTEXT("SelectActorForEvents", "Select Actor(s) to see available Events & Functions");
			FText SelectActorsToolTip = LOCTEXT("SelectActorForEventsTooltip", "Select Actor(s) in the level to see available Events and Functions in this menu.");
			TSharedPtr<FEdGraphSchemaAction> MsgAction = TSharedPtr<FEdGraphSchemaAction>(new FEdGraphSchemaAction_Dummy(TEXT(""), SelectActorsMsg, SelectActorsToolTip.ToString(), LevelActorSectionGroup));
			MenuOut.AddAction(MsgAction);
		}
	}
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuUtils::MakeFavoritesMenu(FBlueprintActionContext const& Context, FBlueprintActionMenuBuilder& MenuOut)
{
	MenuOut.Empty();

	UEditorExperimentalSettings const* ExperimentalSettings = GetDefault<UEditorExperimentalSettings>();
	if (ExperimentalSettings->bUseRefactoredBlueprintMenuingSystem)
	{
		FBlueprintActionFilter MenuFilter;
		MenuFilter.Context = Context;
		MenuFilter.AddRejectionTest(FBlueprintActionFilter::FRejectionTestDelegate::CreateStatic(BlueprintActionMenuUtilsImpl::IsNonFavoritedAction));

		MenuOut.AddMenuSection(MenuFilter);
		MenuOut.RebuildActionList();
	}
	else
	{
		check(Context.Blueprints.Num() > 0);
		FBlueprintPaletteListBuilder LegacyMenuBuilder(Context.Blueprints[0]);
		UEdGraphSchema_K2 const* K2Schema = GetDefault<UEdGraphSchema_K2>();
		FK2ActionMenuBuilder(K2Schema).GetPaletteActions(LegacyMenuBuilder, /*ClassFilter =*/nullptr);

		UEditorUserSettings& EditorUserSettings = GEditor->AccessEditorUserSettings();
		// grab the user's favorites
		UBlueprintPaletteFavorites const* BlueprintFavorites = EditorUserSettings.BlueprintFavorites;
		check(BlueprintFavorites != nullptr);

		for (int32 ActionIndex = 0; ActionIndex < LegacyMenuBuilder.GetNumActions(); ++ActionIndex)
		{
			FGraphActionListBuilderBase::ActionGroup& ActionGroup = LegacyMenuBuilder.GetAction(ActionIndex);
			if (!ensure(ActionGroup.Actions.Num() == 1))
			{
				continue;
			}

			TSharedPtr<FEdGraphSchemaAction> Action = ActionGroup.Actions[0];
			if (!Action.IsValid())
			{
				continue;
			}

			if (BlueprintFavorites->IsFavorited(Action))
			{
				MenuOut.AddAction(Action);
			}
		}
	} 	
}

#undef LOCTEXT_NAMESPACE
