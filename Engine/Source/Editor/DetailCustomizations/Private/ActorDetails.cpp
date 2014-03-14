// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "ActorDetails.h"
#include "AssetSelection.h"
#include "Editor/Layers/Public/LayersModule.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "Editor/LevelEditor/Public/LevelEditorActions.h"
#include "Editor/ClassViewer/Public/ClassViewerModule.h"
#include "Editor/ClassViewer/Public/ClassViewerFilter.h"
#include "Editor/UnrealEd/Public/Kismet2/KismetEditorUtilities.h"
#include "Editor/UnrealEd/Public/Kismet2/KismetDebugUtilities.h"
#include "ClassIconFinder.h"
#include "Runtime/AssetRegistry/Public/AssetRegistryModule.h"
#include "SCodeView.h"
#include "BlueprintGraphDefinitions.h"
#include "ActorMaterialCategory.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "ScopedTransaction.h"
#include "SCreateBlueprintFromActorDialog.h"
#include "ActorEditorUtils.h"
#include "BlueprintEditorUtils.h"
#include "ComponentTransformDetails.h"
#include "ComponentsTree.h"
#include "IPropertyUtilities.h"
#include "IDocumentation.h"

#define LOCTEXT_NAMESPACE "ActorDetails"

TSharedRef<IDetailCustomization> FActorDetails::MakeInstance()
{
	return MakeShareable( new FActorDetails );
}

FActorDetails::~FActorDetails()
{
}

void FActorDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	// These details only apply when adding an instance of the actor in a level
	if( !DetailLayout.GetDetailsView().HasClassDefaultObject() && DetailLayout.GetDetailsView().GetSelectedActorInfo().NumSelected > 0 )
	{
		// Build up a list of unique blueprints in the selection set (recording the first actor in the set for each one)
		TMap<UBlueprint*, UObject*> UniqueBlueprints;

		// Per level Actor Counts
		TMap<ULevel*, int32> ActorsPerLevelCount;

		bool bHasBillboardComponent = false;
		const TArray< TWeakObjectPtr<UObject> >& SelectedObjects = DetailLayout.GetDetailsView().GetSelectedObjects();
		for (int32 ObjectIndex = 0; ObjectIndex < SelectedObjects.Num(); ++ObjectIndex)
		{
			AActor* Actor = Cast<AActor>( SelectedObjects[ObjectIndex].Get() );

			if (Actor != NULL)
			{
				// Store the selected actors for use later. Its fine to do this when CustomizeDetails is called because if the selected actors changes, CustomizeDetails will be called again on a new instance
				// and our current resource would be destroyed.
				SelectedActors.Add( Actor );

				// Record the level that contains this actor and increment it's actor count
				ULevel* Level = Actor->GetTypedOuter<ULevel>();
				if (Level != NULL)
				{
					int32& ActorCountForThisLevel = ActorsPerLevelCount.FindOrAdd(Level);
					++ActorCountForThisLevel;
				}

				// Add to the unique blueprint map if the actor is generated from a blueprint
				if (UBlueprint* Blueprint = Cast<UBlueprint>(Actor->GetClass()->ClassGeneratedBy))
				{
					if (!UniqueBlueprints.Find(Blueprint))
					{
						UniqueBlueprints.Add(Blueprint, Actor);
					}
				}

				if (!bHasBillboardComponent)
				{
					bHasBillboardComponent = Actor->FindComponentByClass<UBillboardComponent>() != NULL;
				}
			}
		}

		if (!bHasBillboardComponent)
		{
			// Actor billboard scale is not relevant if the actor doesn't have a billboard component
			DetailLayout.HideProperty( GET_MEMBER_NAME_CHECKED(AActor, SpriteScale) );
		}

		AddTransformCategory( DetailLayout );

		AddMaterialCategory( DetailLayout );

		AddActorCategory( DetailLayout, ActorsPerLevelCount );

		// Get the list of hidden categories
		TArray<FString> HideCategories;
		DetailLayout.GetDetailsView().GetBaseClass()->GetHideCategories(HideCategories);

		// Add Blueprint category, if not being hidden
		if (!HideCategories.Contains(TEXT("Blueprint")))
		{
			AddBlueprintCategory(DetailLayout, UniqueBlueprints);
		}

		if( GetDefault<UEditorExperimentalSettings>()->bCodeView )
		{
			AddCodeViewCategory( DetailLayout );
		}

		if (!HideCategories.Contains(TEXT("Layers")))
		{
			AddLayersCategory(DetailLayout);
		}

		//AddComponentsCategory( DetailLayout );
	}
}

void FActorDetails::OnConvertActor(UClass* ChosenClass)
{
	if (ChosenClass)
	{
		// Check each selected actor's pointer.
		TArray<AActor*> SelectedActorsRaw;
		for (int32 i=0; i<SelectedActors.Num(); i++)
		{
			if (SelectedActors[i].IsValid())
			{
				SelectedActorsRaw.Add(SelectedActors[i].Get());
			}
		}

		// If there are valid pointers, convert the actors.
		if (SelectedActorsRaw.Num())
		{
			// Dismiss the menu BEFORE converting actors as it can refresh the details panel and if the menu is still open
			// it will be parented to an invalid actor details widget
			FSlateApplication::Get().DismissAllMenus();

			GEditor->ConvertActors(SelectedActorsRaw, ChosenClass, TSet<FString>(), true);
		}
	}
}

class FConvertToClassFilter : public IClassViewerFilter
{
public:
	/** All classes in this set will be allowed. */
	TSet< const UClass* > AllowedClasses;

	/** All classes in this set will be disallowed. */
	TSet< const UClass* > DisallowedClasses;

	/** Allowed ChildOf relationship. */
	TSet< const UClass* > AllowedChildOfRelationship;

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs ) OVERRIDE
	{
		if(InClass->GetName() == TEXT("MyLightBlueprint_C"))
		{
			InClass = InClass;
		}
		EFilterReturn::Type eState = InFilterFuncs->IfInClassesSet(AllowedClasses, InClass);
		if(eState == EFilterReturn::NoItems)
		{
			eState = InFilterFuncs->IfInChildOfClassesSet(AllowedChildOfRelationship, InClass);
		}

		// As long as it has not failed to be on an allowed list, check if it is on a disallowed list.
		if(eState == EFilterReturn::Passed)
		{
			eState = InFilterFuncs->IfInClassesSet(DisallowedClasses, InClass);

			// If it passes, it's on the disallowed list, so we do not want it.
			if(eState == EFilterReturn::Passed)
			{
				return false;
			}
			else
			{
				return true;
			}
		}

		return false;
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) OVERRIDE
	{
		EFilterReturn::Type eState = InFilterFuncs->IfInClassesSet(AllowedClasses, InUnloadedClassData);
		if(eState == EFilterReturn::NoItems)
		{
			eState = InFilterFuncs->IfInChildOfClassesSet(AllowedChildOfRelationship, InUnloadedClassData);
		}

		// As long as it has not failed to be on an allowed list, check if it is on a disallowed list.
		if(eState == EFilterReturn::Passed)
		{
			eState = InFilterFuncs->IfInClassesSet(DisallowedClasses, InUnloadedClassData);

			// If it passes, it's on the disallowed list, so we do not want it.
			if(eState == EFilterReturn::Passed)
			{
				return false;
			}
			else
			{
				return true;
			}
		}

		return false;
	}
};

UClass* FActorDetails::GetConversionRoot( UClass* InCurrentClass ) const
{
	UClass* ParentClass = InCurrentClass;

	while(ParentClass)
	{
		if( ParentClass->GetBoolMetaData(FName(TEXT("IsConversionRoot"))) )
		{
			break;
		}
		ParentClass = ParentClass->GetSuperClass();
	}

	return ParentClass;
}

void FActorDetails::CreateClassPickerConvertActorFilter(const TWeakObjectPtr<AActor> ConvertActor, class FClassViewerInitializationOptions* ClassPickerOptions)
{
	// Shouldn't ever be overwriting an already established filter
	check( ConvertActor.IsValid() )
	check( ClassPickerOptions != NULL && !ClassPickerOptions->ClassFilter.IsValid() )
	TSharedPtr<FConvertToClassFilter> Filter = MakeShareable(new FConvertToClassFilter);
	ClassPickerOptions->ClassFilter = Filter;

	UClass* ConvertClass = ConvertActor->GetClass();
	UClass* RootConversionClass = GetConversionRoot(ConvertClass);

	if(RootConversionClass)
	{
		Filter->AllowedChildOfRelationship.Add(RootConversionClass);
	}

	// Never convert to the same class
	Filter->DisallowedClasses.Add(ConvertClass);
}

TSharedRef<SWidget> FActorDetails::OnGetConvertContent()
{
	// Build a class picker widget

	// Fill in options
	FClassViewerInitializationOptions Options;

	Options.bShowUnloadedBlueprints = true;
	Options.bIsActorsOnly = true;
	Options.bIsPlaceableOnly = true;

	// All selected actors are of the same class, so just need to use one to generate the filter
	if ( SelectedActors.Num() > 0 )
	{
		CreateClassPickerConvertActorFilter(SelectedActors.Top(), &Options);
	}

	Options.Mode = EClassViewerMode::ClassPicker;
	Options.DisplayMode = EClassViewerDisplayMode::ListView;

	TSharedRef<SWidget> ClassPicker = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateClassViewer(Options, FOnClassPicked::CreateSP(this, &FActorDetails::OnConvertActor));

	return
		SNew(SBox)
			.WidthOverride(280)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
				.AutoHeight()
				.MaxHeight(500)
			[
				ClassPicker
			]			
		];
}

EVisibility FActorDetails::GetConvertMenuVisibility() const
{
	return GEditorModeTools().EnsureNotInMode(FBuiltinEditorModes::EM_InterpEdit) ?
		EVisibility::Visible :
		EVisibility::Collapsed;
}

TSharedRef<SWidget> FActorDetails::MakeConvertMenu( const FSelectedActorInfo& SelectedActorInfo )
{
	if( SelectedActorInfo.HasConvertableAsset() )
	{
		UClass* RootConversionClass = GetConversionRoot(SelectedActorInfo.SelectionClass);
		const FSlateFontInfo& FontInfo = FEditorStyle::GetFontStyle( TEXT("ExpandableArea.NormalFont") );
		return
			SNew( SBox )
			.HAlign( HAlign_Left )
			[
				SNew( SComboButton )
					.IsEnabled(RootConversionClass != NULL)
					.Visibility( this, &FActorDetails::GetConvertMenuVisibility )
					.OnGetMenuContent( this, &FActorDetails::OnGetConvertContent )
					.ButtonContent()
				[
					SNew( STextBlock )
					.Text( LOCTEXT("ConvertButton", "Convert") ) 
					.ToolTipText( LOCTEXT("ConvertButton_ToolTip", "Convert actors to different types") )
					.Font( FontInfo )
				]
			];

	}

	// Nothing to convert
	return SNullWidget::NullWidget;

}

void FActorDetails::OnNarrowSelectionSetToSpecificLevel( TWeakObjectPtr<ULevel> LevelToNarrowInto )
{
	if (ULevel* RequiredLevel = LevelToNarrowInto.Get())
	{
		// Remove any selected objects that aren't in the specified level
		TArray<AActor*> ActorsToDeselect;
		for ( TArray< TWeakObjectPtr<AActor> >::TConstIterator Iter(SelectedActors); Iter; ++Iter)
		{ 
			if( (*Iter).IsValid() )
			{
				AActor* Actor = (*Iter).Get();
				if (!Actor->IsIn(RequiredLevel))
				{
					ActorsToDeselect.Add(Actor);
				}
			}
		}

		for (TArray<AActor*>::TIterator DeselectIt(ActorsToDeselect); DeselectIt; ++DeselectIt)
		{
			AActor* Actor = *DeselectIt;
			GEditor->SelectActor(Actor, /*bSelected=*/ false, /*bNotify=*/ false);
		}

		// Tell the editor selection status was changed.
		GEditor->NoteSelectionChange();
	}
}

bool FActorDetails::IsActorValidForLevelScript() const
{
	AActor* Actor = GEditor->GetSelectedActors()->GetTop<AActor>();
	return FKismetEditorUtilities::IsActorValidForLevelScript(Actor);
}

FReply FActorDetails::FindSelectedActorsInLevelScript()
{
	GUnrealEd->FindSelectedActorsInLevelScript();
	return FReply::Handled();
};

bool FActorDetails::AreAnySelectedActorsInLevelScript() const
{
	return GUnrealEd->AreAnySelectedActorsInLevelScript();
};

/** Util to create a menu for events we can add for the selected actor */
TSharedRef<SWidget> FActorDetails::MakeEventOptionsWidgetFromSelection()
{
	FMenuBuilder EventMenuBuilder( true, NULL );

	AActor* Actor = SelectedActors[0].Get();
	FKismetEditorUtilities::AddLevelScriptEventOptionsForActor(EventMenuBuilder, SelectedActors[0], true, true, false);
	return EventMenuBuilder.MakeWidget();
}


void FActorDetails::AddLayersCategory( IDetailLayoutBuilder& DetailBuilder )
{
	if( !FModuleManager::Get().IsModuleLoaded( TEXT("Layers") ) )
	{
		return;
	}

	FLayersModule& LayersModule = FModuleManager::LoadModuleChecked< FLayersModule >( TEXT("Layers") );

	const FString LayerCategory = LOCTEXT("LayersCategory", "Layers").ToString();

	DetailBuilder.EditCategory( "Layers", LayerCategory, ECategoryPriority::Uncommon )
	.AddCustomRow( TEXT("") )
	[
		LayersModule.CreateLayerCloud( SelectedActors )
	];
}

void FActorDetails::AddTransformCategory( IDetailLayoutBuilder& DetailBuilder )
{
	const FSelectedActorInfo& SelectedActorInfo = DetailBuilder.GetDetailsView().GetSelectedActorInfo();

	bool bAreBrushesSelected = SelectedActorInfo.bHaveBrush;
	bool bIsOnlyWorldPropsSelected =  SelectedActors.Num() == 1 && SelectedActors[0].IsValid() && SelectedActors[0]->IsA<AWorldSettings>();
	bool bLacksRootComponent = SelectedActors[0].IsValid() && (SelectedActors[0]->GetRootComponent()==NULL);

	// Don't show the Transform details when brushes are selected, if the only actor selected is world properties, or if they have no RootComponent
	if ( bAreBrushesSelected || bIsOnlyWorldPropsSelected || bLacksRootComponent )
	{
		return;
	}
	
	TSharedRef<FComponentTransformDetails> TransformDetails = MakeShareable( new FComponentTransformDetails( DetailBuilder.GetDetailsView().GetSelectedObjects(), SelectedActorInfo, DetailBuilder.GetPropertyUtilities()->GetNotifyHook() ) );

	IDetailCategoryBuilder& TransformCategory = DetailBuilder.EditCategory( "TransformCommon", LOCTEXT("TransformCommonCategory", "Transform").ToString(), ECategoryPriority::Transform );

	TransformCategory.AddCustomBuilder( TransformDetails );

}

void FActorDetails::AddMaterialCategory( IDetailLayoutBuilder& DetailBuilder )
{
	MaterialCategory = MakeShareable( new FActorMaterialCategory( SelectedActors ) );
	MaterialCategory->Create( DetailBuilder );
}


void FActorDetails::AddActorCategory( IDetailLayoutBuilder& DetailBuilder, const TMap<ULevel*, int32>& ActorsPerLevelCount )
{		
	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>( TEXT("LevelEditor") );

	const FLevelEditorCommands& Commands = LevelEditor.GetLevelEditorCommands();
	TSharedRef<const FUICommandList> CommandBindings = LevelEditor.GetGlobalLevelEditorActions();

	const FSelectedActorInfo& SelectedActorInfo = DetailBuilder.GetDetailsView().GetSelectedActorInfo();
	TSharedPtr<SVerticalBox> LevelBox;

	IDetailCategoryBuilder& ActorCategory = DetailBuilder.EditCategory("Actor", TEXT(""), ECategoryPriority::Uncommon );


	// Create the info buttons per level
	for ( auto LevelIt( ActorsPerLevelCount.CreateConstIterator() ); LevelIt; ++LevelIt)
	{
		ULevel* Level = LevelIt.Key();
		int32 SelectedActorCountInLevel = LevelIt.Value();
		
		// Get a description of the level
		FText LevelDescription = FText::FromString( FPackageName::GetShortName( Level->GetOutermost()->GetFName() ) );
		if (Level == Level->OwningWorld->PersistentLevel)
		{
			LevelDescription = NSLOCTEXT("UnrealEd", "PersistentLevel", "Persistent Level");
		}

		// Create a description and tooltip for the actor count/selection hyperlink
		const FText ActorCountDescription = FText::Format( LOCTEXT("SelectedActorsInOneLevel", "{0} selected in"), FText::AsNumber( SelectedActorCountInLevel ) );

		const FText Tooltip = FText::Format( LOCTEXT("SelectedActorsHyperlinkTooltip", "Narrow the selection set to just the actors in {0}"), LevelDescription);

		// Create the row for this level
		TWeakObjectPtr<ULevel> WeakLevelPtr = Level;

		ActorCategory.AddCustomRow( LOCTEXT("SelectionFilter", "Selected").ToString() )
		.NameContent()
		[
			SNew(SHyperlink)
				.Style(FEditorStyle::Get(), "HoverOnlyHyperlink")
				.OnNavigate( this, &FActorDetails::OnNarrowSelectionSetToSpecificLevel, WeakLevelPtr )
				.Text(ActorCountDescription)
				.TextStyle(FEditorStyle::Get(), "DetailsView.HyperlinkStyle")
				.ToolTipText(Tooltip)
		]
		.ValueContent()
		[
			SNew(STextBlock)
				.Text(LevelDescription)
				.Font( IDetailLayoutBuilder::GetDetailFont() )
		];

	}

	// WorldSettings should never convert to another class type.
	if(SelectedActorInfo.SelectionClass != AWorldSettings::StaticClass())
	{
		ActorCategory.AddCustomRow(  LOCTEXT("SelectionFilter", "Selected").ToString() )
		.NameContent()
		[
			MakeConvertMenu( SelectedActorInfo )
		];
	}
}

void FActorDetails::AddBlueprintCategory( IDetailLayoutBuilder& DetailBuilder, const TMap<UBlueprint*, UObject*>& UniqueBlueprints )
{
	// Create the overarching structure and expose the vertical box to add entries per-blueprint
	TSharedPtr<SVerticalBox> Box;

	IDetailCategoryBuilder& BlueprintCategory = DetailBuilder.EditCategory("Blueprint", NSLOCTEXT("BlueprintDetails", "BlueprintTitle", "Blueprint").ToString(), ECategoryPriority::Uncommon );

	// Only show the blueprints section if a single actor is selected
	if ( SelectedActors.Num() > 0 )
	{
		// We don't need to replace the blueprints with a combined one unless the number of selected actors have 
		AddUtilityBlueprintRows( BlueprintCategory );

		if ( UniqueBlueprints.Num() > 0 )
		{
			// Add an edit row for each blueprint
			for ( auto BlueprintIt( UniqueBlueprints.CreateConstIterator() ); BlueprintIt; ++BlueprintIt )
			{
				UBlueprint* Blueprint = BlueprintIt.Key();
				UObject* Actor = BlueprintIt.Value();

				AddSingleBlueprintRow( BlueprintCategory, Blueprint, Actor );
			}
		}
		else
		{
			AddSingleBlueprintRow( BlueprintCategory, NULL, SelectedActors[0].Get() );
		}
	}
}

void FActorDetails::AddCodeViewCategory( IDetailLayoutBuilder& DetailBuilder )
{
	if (FModuleManager::Get().IsSolutionFilePresent())
	{
		TSharedRef< CodeView::SCodeView > CodeViewWidget =
			SNew( CodeView::SCodeView )
			.GetSelectedActors( this, &FActorDetails::GetSelectedActors );

		// Only start out expanded if we're already in "ready to populate" mode.  This is because we don't want
		// to immediately start digesting symbols as soon as the widget is visible.  Instead, when the user
		// expands the section, we'll start loading symbols.  However, this state is remembered even after
		// the widget is destroyed.
		const bool bShouldInitiallyExpand = CodeViewWidget->IsReadyToPopulate();

		DetailBuilder.EditCategory( "CodeView", NSLOCTEXT("ActorDetails", "CodeViewSection", "Code View").ToString(), ECategoryPriority::Uncommon )
		.InitiallyCollapsed( !bShouldInitiallyExpand )
		// The expansion state should not be restored
		.RestoreExpansionState( false )
		.OnExpansionChanged( FOnBooleanValueChanged::CreateSP( CodeViewWidget, &CodeView::SCodeView::OnDetailSectionExpansionChanged ) )
		.AddCustomRow( NSLOCTEXT("ActorDetails", "CodeViewSection", "Code View").ToString() )
		[
			// @todo editcode1: Width of item is too big for detail view?!
			CodeViewWidget
		];
	}
}

void FActorDetails::AddComponentsCategory( IDetailLayoutBuilder& DetailBuilder )
{
	AActor* SelectedActor = NULL;
	const TArray< TWeakObjectPtr<AActor> >& Actors = GetSelectedActors();
	if(Actors.Num() == 1 && Actors[0].IsValid())
	{
		SelectedActor = Actors[0].Get();
	}

	DetailBuilder.EditCategory( "Components", NSLOCTEXT("ActorDetails", "ComponentsSection", "Components").ToString(), ECategoryPriority::Uncommon )
		.InitiallyCollapsed( true )
		.AddCustomRow( NSLOCTEXT("ActorDetails", "ComponentsSection", "Components").ToString() )
		[
			SNew( SComponentsTree, SelectedActor )
		];
}

const TArray< TWeakObjectPtr<AActor> >& FActorDetails::GetSelectedActors() const
{
	return SelectedActors;
}

void FActorDetails::AddUtilityBlueprintRows( IDetailCategoryBuilder& BlueprintCategory )
{
	if ( SelectedActors.Num() == 1 )
	{
		// Find In Level Blueprint
		TWeakObjectPtr<AActor> ActorPtr = SelectedActors[0];
		FText ActorLabel = NSLOCTEXT( "UnrealEd", "None", "None" );

		if ( ActorPtr.IsValid() )
		{
			ActorLabel = FText::FromString( ActorPtr.Get()->GetActorLabel() );
		}

		FFormatNamedArguments Args;
		Args.Add( TEXT( "ActorLabel" ), ActorLabel );

		const FText FindButtonLabel = FText::Format( LOCTEXT( "FindInLevelScript", "Find {ActorLabel} in Level Blueprint" ), Args );
		const FText FindButtonToolTip = FText::Format( LOCTEXT( "FindInLevelScript_ToolTip", "Search for uses of {ActorLabel} in the Level Blueprint" ), Args );

		BlueprintCategory.AddCustomRow( LOCTEXT( "FindInLevelScript_Filter", "Find in Level Script" ).ToString() )
		.WholeRowContent()
		[
			SNew( SHorizontalBox )

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding( 0, 0, 10, 0 )
			[
				SNew( SBox )
				.WidthOverride( 250 )
				[
					SNew( SButton )
					.ToolTipText( FindButtonToolTip )
					.OnClicked( this, &FActorDetails::FindSelectedActorsInLevelScript )
					.IsEnabled( this, &FActorDetails::AreAnySelectedActorsInLevelScript )
					.HAlign( HAlign_Fill )
					.ContentPadding( 2 )
					[
						SNew( STextBlock )
						.Text( FindButtonLabel )
						.Font( IDetailLayoutBuilder::GetDetailFont() )
					]
				]
			]
		];

		// Add Level Events
		const FText AddEventLabel = FText::Format( LOCTEXT( "ScriptingEvents", "Add Level Events for {ActorLabel}" ), Args );
		const FText AddEventToolTip = FText::Format( LOCTEXT( "ScripingEvents_Tooltip", "Adds or views events for {ActorLabel} in the Level Blueprint" ), Args );

		BlueprintCategory.AddCustomRow( LOCTEXT( "ScriptingEvents_Filter", "Level Blueprint Events" ).ToString() )
		.WholeRowContent()
		[
			SNew( SHorizontalBox )

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding( 0, 0, 10, 0 )
			[
				SNew( SBox )
				.WidthOverride( 250 )
				[
					SNew( SComboButton )
					.IsEnabled( this, &FActorDetails::IsActorValidForLevelScript )
					.OnGetMenuContent( this, &FActorDetails::MakeEventOptionsWidgetFromSelection )
					.ContentPadding( 2 )
					.HAlign( HAlign_Fill )
					.ButtonContent()
					[
						SNew( STextBlock )
						.Text( AddEventLabel )
						.ToolTipText( AddEventToolTip )
						.Font( IDetailLayoutBuilder::GetDetailFont() )
					]
				]
			]
		];
	}

	// Create the row for this blueprint
	BlueprintCategory.AddCustomRow( NSLOCTEXT( "BlueprintDetails", "BlueprintFilter", "Blueprints" ).ToString() )
	.WholeRowContent()
	[
		SNew( SHorizontalBox )

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding( 0, 0, 10, 0 )
		[
			SNew( SBox )
			.WidthOverride( 250 )
			[
				SNew( SButton )
				.ToolTipText( LOCTEXT( "CreateHarvestBlueprint_ToolTip", "Harvest Components from Selected Actors and create Blueprint" ) )
				.HAlign( HAlign_Fill )
				//.ButtonColorAndOpacity( FLinearColor( 0.2f, 0.4f, 0.6f, 1.0f ) )
				.OnClicked( FOnClicked::CreateRaw( this, &FActorDetails::OnPickBlueprintPathClicked, true ) )
				.ContentPadding( 2 )
				[
					SNew( STextBlock )
					.Text( LOCTEXT( "ReplaceWithAmalgamBlueprint", "Replace With Composited Blueprint" ) )
				]
			]
		]
	];
}

// Creates a row for a single blueprint in the selection details view   
BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FActorDetails::AddSingleBlueprintRow( IDetailCategoryBuilder& BlueprintCategory, UBlueprint* InBlueprint, UObject* InExampleActor)
{
	TWeakObjectPtr<UBlueprint> WeakBP = InBlueprint;
	TWeakObjectPtr<UObject> WeakFirstAsset = InExampleActor;

	TAttribute<const FSlateBrush*> BlueprintStatusIcon;
	BlueprintStatusIcon.Bind(FGetBrushImage::CreateSP(this, &FActorDetails::GetBlueprintStatusIcon, WeakBP));

	TAttribute<FText> BlueprintStatusTooltip;
	BlueprintStatusTooltip.Bind(FGetText::CreateSP(this, &FActorDetails::GetBlueprintStatusTooltip, WeakBP));

	TAttribute<const FSlateBrush*> BreakpointStatusIcon;
	BreakpointStatusIcon.Bind(FGetBrushImage::CreateSP(this, &FActorDetails::GetBlueprintBreakpointStatusIcon, WeakBP));

	const FSlateFontInfo& InfoFont = FEditorStyle::GetFontStyle( TEXT("ExpandableArea.NormalFont") );

	FText Name, Description;
	if (InBlueprint)
	{
		Name = FText::FromString( InBlueprint->GetName() );
		Description = FText::FromString( InBlueprint->GetDesc() );
	}

	int NumBlueprintableActors = 0;
	bool IsBlueprintBased = InBlueprint ? true : false;

	if(!InBlueprint)
	{
		for ( TWeakObjectPtr<AActor>& Actor : SelectedActors )
		{
			if( FKismetEditorUtilities::CanCreateBlueprintOfClass(Actor->GetClass()))
			{
				NumBlueprintableActors++;
			}
		}
	}

	const bool bCanCreateActorBlueprint = ( !IsBlueprintBased && ( NumBlueprintableActors == 1 ) );

	// Set up PathPickerConfig.
	PathForActorBlueprint = FString("/Game/");
	FPathPickerConfig PathPickerConfig;
	PathPickerConfig.DefaultPath = PathForActorBlueprint;
	PathPickerConfig.OnPathSelected = FOnPathSelected::CreateRaw(this, &FActorDetails::OnSelectBlueprintPath);
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	FText CreateBlueprintToolTip = LOCTEXT("CreateBlueprintSuccess_ToolTip", "Create a Blueprint based on this Actor");
	if(IsBlueprintBased)
	{
		CreateBlueprintToolTip = LOCTEXT("CreateBlueprintFail_ToolTip", "Cannot create Blueprint using this Actor class");
	}
	else if(!IsBlueprintBased && NumBlueprintableActors > 1)
	{
		CreateBlueprintToolTip = LOCTEXT("CreateBlueprintFailTooMany_ToolTip", "Cannot create Blueprint, please select only one Actor");
	}

	FString SelectedActorClassName;
	if(NumBlueprintableActors == 1)
	{
		SelectedActorClassName = SelectedActors[0]->GetClass()->GetName();
	}
	
	FText CurrentBlueprintLong = FText::Format( FText::FromString( TEXT( "{0} {1}" ) ), Name, Description );

	// Blueprint Details
	BlueprintCategory.AddCustomRow( NSLOCTEXT("BlueprintDetails", "BlueprintFilter", "Blueprints").ToString() )
	.Visibility( InBlueprint ? EVisibility::Visible : EVisibility::Collapsed )
	.NameContent()
	[
		SNew( SHorizontalBox )

		// Blueprint status icon
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding( 0.0f, 0.0f, 4.0f, 0.0f )
		[
			SNew(SButton)
			.OnClicked( this, &FActorDetails::OnCompileBlueprintClicked, WeakBP )
			.ToolTipText( BlueprintStatusTooltip )
			.ButtonStyle( FEditorStyle::Get(), "NoBorder" )
			.ContentPadding(0.0f)
			[
				SNew(SImage)
				.Image( BlueprintStatusIcon )
			]
		]

		// Blueprint name
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign( VAlign_Center )
		.HAlign( HAlign_Fill )
		.Padding( 0.0f, 0.0f, 4.0f, 0.0f )
		[
			SNew( STextBlock )
			.Font( InfoFont )
			.Text( Name )
			.ToolTipText( CurrentBlueprintLong )
		]

		// Blueprint Description
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign( VAlign_Center )
		.HAlign( HAlign_Fill )
		.Padding( 0.0f, 0.0f, 4.0f, 0.0f )
		[
			SNew( STextBlock )
			.Font( InfoFont )
			.ColorAndOpacity(FSlateColor::UseSubduedForeground() )
			.Text( Description )
			.ToolTipText( CurrentBlueprintLong )
		]
	]
	.ValueContent()
	.MinDesiredWidth(200.0f)
	.MaxDesiredWidth(200.0f)
	[
		SNew( SHorizontalBox )

		// Breakpoint enable/disable toggle and status button
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding( 0.0f, 0.0f, 4.0f, 0.0f )
		[
			SNew(SButton)
			.OnClicked( this, &FActorDetails::OnToggleBreakpointsClicked, WeakBP )
			.ToolTipText( LOCTEXT("BlueprintBreakpointToggleAll_Tooltip", "Toggle the enabled state of all breakpoints in this blueprint") )
			.ButtonStyle( FEditorStyle::Get(), "NoBorder" )
			.ContentPadding(0.0f)
			[
				SNew(SImage)
				.Image( BreakpointStatusIcon )
			]
		]

		// Button for editing the blueprint
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew( SButton )
			.VAlign( VAlign_Center )
			.HAlign( HAlign_Center )
			.IsEnabled(InBlueprint != NULL)
			.OnClicked( this, &FActorDetails::OnEditBlueprintClicked, WeakBP, WeakFirstAsset )
			[
				SNew( STextBlock )
				.Font( InfoFont )
				.Text( LOCTEXT("EditAsset", "Edit") )
			]
		]
		
		// Button for pushing to Blueprint defaults
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew( SButton )
			.VAlign( VAlign_Center )
			.HAlign( HAlign_Center )
			.IsEnabled( this, &FActorDetails::PushToBlueprintDefaults_IsEnabled, WeakBP )
			.OnClicked( this, &FActorDetails::PushToBlueprintDefaults_OnClicked, WeakBP )
			.ToolTip(IDocumentation::Get()->CreateToolTip(
				TAttribute<FText>::Create( TAttribute<FText>::FGetter::CreateRaw(this, &FActorDetails::PushToBlueprintDefaults_ToolTipText, WeakBP) ),
				NULL,
				TEXT("Shared/LevelEditor"),
				TEXT("PushToBlueprintDefaults")))
			[
				SNew( STextBlock )
				.Font( InfoFont )
				.Text( LOCTEXT("PushToBlueprintDefaultsButtonLabel", "Apply") )
			]
		]

		// Button for resetting to Blueprint defaults
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew( SButton )
			.VAlign( VAlign_Center )
			.HAlign( HAlign_Center )
			.IsEnabled( this, &FActorDetails::ResetToBlueprintDefaults_IsEnabled, WeakBP )
			.OnClicked( this, &FActorDetails::ResetToBlueprintDefaults_OnClicked, WeakBP )
			.ToolTip(IDocumentation::Get()->CreateToolTip(
				TAttribute<FText>::Create( TAttribute<FText>::FGetter::CreateRaw(this, &FActorDetails::ResetToBlueprintDefaults_ToolTipText, WeakBP) ),
				NULL,
				TEXT("Shared/LevelEditor"),
				TEXT("ResetToBlueprintDefaults")))
			[
				SNew( STextBlock )
				.Font( InfoFont )
				.Text( LOCTEXT("ResetToBlueprintDefaultsButtonLabel", "Reset") )
			]
		]
	];

	if(!InBlueprint)
	{
		BlueprintCategory.AddCustomRow( LOCTEXT("CreateBlueprintFilterString", "Create Blueprint").ToString(), true )
		[
			SNew( SHorizontalBox )
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("NoBorder"))
					.Padding(0)
					.ToolTipText(CreateBlueprintToolTip)
				[
					SNew( SButton )
						.IsEnabled(bCanCreateActorBlueprint)
						.ButtonColorAndOpacity(FLinearColor(0.2f, 0.4f, 0.6f, 1.0f))
						.OnClicked(FOnClicked::CreateRaw(this, 	&FActorDetails::OnPickBlueprintPathClicked, false ))
					[
						SNew( SHorizontalBox )
						+SHorizontalBox::Slot()
							.VAlign(VAlign_Center)
							.AutoWidth()
						[
							SNew(STextBlock)
								.Text( FText::Format( LOCTEXT("CreateBlueprintFromActor", "Create {0} Blueprint..."), FText::FromString( SelectedActorClassName ) ) )
						]

						+SHorizontalBox::Slot()
							.VAlign(VAlign_Center)
							.AutoWidth()
							.Padding( 4.0f, 0.0f, 0.0f, 0.0f )
						[
							SNew( SImage )
								.Image(FEditorStyle::GetBrush(TEXT("Kismet.CreateBlueprint")))
						]
					]
				]
			]
		];
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FReply FActorDetails::OnPickBlueprintPathClicked(bool bHarvest )
{
	SCreateBlueprintFromActorDialog::OpenDialog(bHarvest);

	return FReply::Handled();
}

void FActorDetails::OnSelectBlueprintPath( const FString& Path )
{
	PathForActorBlueprint = Path;
}

FText FActorDetails::GetBlueprintStatusTooltip( TWeakObjectPtr<class UBlueprint> Asset ) const
{
	if (UBlueprint* Blueprint = Asset.Get())
	{
		switch (Blueprint->Status)
		{
		default:
		case BS_Unknown:
		case BS_Dirty:
			return LOCTEXT("BlueprintStatus_Tooltip_Unknown", "Blueprint may have been modified. Click to recompile.");
		case BS_Error:
			return LOCTEXT("BlueprintStatus_Tooltip_Error", "Blueprint has one or more errors!");
		case BS_UpToDate:
			return LOCTEXT("BlueprintStatus_Tooltip_UpToDate", "Blueprint is up to date.");
		case BS_UpToDateWithWarnings:
			return LOCTEXT("BlueprintStatus_Tooltip_Warning", "Blueprint has one or more warnings!");
		}
	}

	return FText::GetEmpty();
}

const FSlateBrush* FActorDetails::GetBlueprintStatusIcon(TWeakObjectPtr<class UBlueprint> Asset) const
{
	if (UBlueprint* Blueprint = Asset.Get())
	{
		switch (Blueprint->Status)
		{
		default:
		case BS_Unknown:
		case BS_Dirty:
			return FEditorStyle::GetBrush("Kismet.Status.Unknown.Small");
		case BS_Error:
			return FEditorStyle::GetBrush("Kismet.Status.Error.Small");
		case BS_UpToDate:
			return FEditorStyle::GetBrush("Kismet.Status.Good.Small");
		case BS_UpToDateWithWarnings:
			return FEditorStyle::GetBrush("Kismet.Status.Warning.Small");
		}
	}

	return NULL;
}

const FSlateBrush* FActorDetails::GetBlueprintBreakpointStatusIcon(TWeakObjectPtr<class UBlueprint> Asset) const
{
	if (UBlueprint* Blueprint = Asset.Get())
	{
		const int32 TotalBreakpointCount = Blueprint->Breakpoints.Num();

		// Count the number of enabled breakpoints
		int32 EnabledCount = 0;
		for (int32 BreakpointIndex = 0; BreakpointIndex < TotalBreakpointCount; ++BreakpointIndex)
		{
			UBreakpoint*& Breakpoint = Blueprint->Breakpoints[BreakpointIndex];
			if (Breakpoint->IsEnabledByUser())
			{
				++EnabledCount;
			}
		}

		if (TotalBreakpointCount > 0)
		{
			if (EnabledCount == TotalBreakpointCount)
			{
				// all are enabled
				return FEditorStyle::GetBrush("Kismet.Breakpoint.EnabledAndValid"); //@TODO: Show enabled and invalid if not compiled?
			}
			else if (EnabledCount == 0)
			{
				// none are enabled
				return FEditorStyle::GetBrush("Kismet.Breakpoint.Disabled");
			}
			else
			{
				// some are enabled but not all
				return FEditorStyle::GetBrush("Kismet.Breakpoint.MixedStatus"); //@TODO: Show enabled and invalid if not compiled?
			}
		}
	}

	// show nothing, no breakpoints to care about
	return FEditorStyle::GetBrush("Kismet.Breakpoint.NoneSpacer");
}

FReply FActorDetails::OnCompileBlueprintClicked(TWeakObjectPtr<class UBlueprint> Asset)
{
	if (UBlueprint* Blueprint = Asset.Get())
	{
		if (!Blueprint->IsUpToDate())
		{
			FKismetEditorUtilities::CompileBlueprint(Blueprint);
		}
	}
	return FReply::Handled();
}

FReply FActorDetails::OnToggleBreakpointsClicked(TWeakObjectPtr<class UBlueprint> Asset)
{
	if (UBlueprint* Blueprint = Asset.Get())
	{
		// Recompile the blueprint before trying to set a breakpoint if it's dirty or unknown and we're not in PIE/SIE
		if (((Blueprint->Status == BS_Unknown) || (Blueprint->Status == BS_Dirty)) && (GEditor->PlayWorld == NULL))
		{
			FKismetEditorUtilities::CompileBlueprint(Blueprint);
		}

		// If the blueprint is still not up to date, don't modify the breakpoints
		if (!Blueprint->IsUpToDate())
		{
			return FReply::Handled();
		}

		const int32 TotalBreakpointCount = Blueprint->Breakpoints.Num();

		// Count the number of enabled breakpoints
		int32 EnabledCount = 0;
		for (int32 BreakpointIndex = 0; BreakpointIndex < TotalBreakpointCount; ++BreakpointIndex)
		{
			UBreakpoint*& Breakpoint = Blueprint->Breakpoints[BreakpointIndex];
			if (Breakpoint->IsEnabledByUser())
			{
				++EnabledCount;
			}
		}

		if (TotalBreakpointCount > 0)
		{
			// Determine what the new state should be
			bool bNewEnabledState = false;
			if (EnabledCount == TotalBreakpointCount)
			{
				// all are enabled; make them all disabled
				bNewEnabledState = false;
			}
			else if (EnabledCount == 0)
			{
				// none are enabled; make them all enabled
				bNewEnabledState = true;
			}
			else
			{
				// some are enabled but not all; make them all enabled
				bNewEnabledState = true;
			}

			// Now make all the breakpoints enabled/disabled
			for (int32 BreakpointIndex = 0; BreakpointIndex < TotalBreakpointCount; ++BreakpointIndex)
			{
				UBreakpoint*& Breakpoint = Blueprint->Breakpoints[BreakpointIndex];
				FKismetDebugUtilities::SetBreakpointEnabled(Breakpoint, bNewEnabledState);
			}
		}
	}
	return FReply::Handled();
}

FReply FActorDetails::OnEditBlueprintClicked( TWeakObjectPtr<UBlueprint> InBlueprint, TWeakObjectPtr<UObject> InAsset )
{
	if (UBlueprint* Blueprint = InBlueprint.Get())
	{
		// Set the object being debugged if given an actor reference (if we don't do this before we edit the object the editor wont know we are debebugging something)
		if (UObject* Asset = InAsset.Get())
		{
			check(Asset->GetClass()->ClassGeneratedBy == Blueprint);
			Blueprint->SetObjectBeingDebugged(Asset);
		}
		// Open the blueprint
		GEditor->EditObject( Blueprint );
	}
	return FReply::Handled();
}

bool FActorDetails::PushToBlueprintDefaults_IsEnabled( TWeakObjectPtr<UBlueprint> InBlueprint ) const
{
	bool bIsEnabled = false;
	if(SelectedActors.Num() == 1)
	{
		AActor* Actor = SelectedActors[0].Get();
		UBlueprint* Blueprint = InBlueprint.Get();
		if(Actor != NULL
			&& Blueprint != NULL
			&& Actor->GetClass()->ClassGeneratedBy == Blueprint)
		{
			AActor* BlueprintCDO = Actor->GetClass()->GetDefaultObject<AActor>();
			if(BlueprintCDO != NULL)
			{
				const auto CopyOptions = (EditorUtilities::ECopyOptions::Type)(EditorUtilities::ECopyOptions::PreviewOnly|EditorUtilities::ECopyOptions::OnlyCopyEditOrInterpProperties);
				bIsEnabled = EditorUtilities::CopyActorProperties(Actor, BlueprintCDO, CopyOptions) > 0;
			}
		}
	}

	return bIsEnabled;
}

FText FActorDetails::PushToBlueprintDefaults_ToolTipText( TWeakObjectPtr<UBlueprint> InBlueprint ) const
{
	int32 NumChangedProperties = 0;

	if(SelectedActors.Num() == 1)
	{
		AActor* Actor = SelectedActors[0].Get();
		UBlueprint* Blueprint = InBlueprint.Get();
		if(Actor != NULL
			&& Blueprint != NULL
			&& Actor->GetClass()->ClassGeneratedBy == Blueprint)
		{
			AActor* BlueprintCDO = Actor->GetClass()->GetDefaultObject<AActor>();
			if(BlueprintCDO != NULL)
			{
				const auto CopyOptions = (EditorUtilities::ECopyOptions::Type)(EditorUtilities::ECopyOptions::PreviewOnly|EditorUtilities::ECopyOptions::OnlyCopyEditOrInterpProperties);
				NumChangedProperties += EditorUtilities::CopyActorProperties(Actor, BlueprintCDO, CopyOptions);
			}
		}
	}

	if (NumChangedProperties == 0)
	{
		return LOCTEXT("DisabledPushToBlueprintDefaults_ToolTip", "Replaces the Blueprint's defaults with any altered property values.");
	}
	else if (NumChangedProperties > 1)
	{
		return FText::Format( LOCTEXT("PushToBlueprintDefaults_ToolTip", "Click to apply {0} changed properties to the Blueprint."), FText::AsNumber( NumChangedProperties ) );
	}
	else
	{
		return LOCTEXT("PushOneToBlueprintDefaults_ToolTip", "Click to apply 1 changed property to the Blueprint.");
	}
}

FReply FActorDetails::PushToBlueprintDefaults_OnClicked( TWeakObjectPtr<UBlueprint> InBlueprint )
{
	int32 NumChangedProperties = 0;

	if(SelectedActors.Num() == 1)
	{
		const FScopedTransaction Transaction(LOCTEXT("PushToBlueprintDefaults_Transaction", "Apply Changes to Blueprint"));

		// Perform the actual copy
		AActor* Actor = SelectedActors[0].Get();
		UBlueprint* Blueprint = InBlueprint.Get();
		if(Actor != NULL
			&& Blueprint != NULL
			&& Actor->GetClass()->ClassGeneratedBy == Blueprint)
		{
			AActor* BlueprintCDO = Actor->GetClass()->GetDefaultObject<AActor>();
			if(BlueprintCDO != NULL)
			{
				const auto CopyOptions = (EditorUtilities::ECopyOptions::Type)(EditorUtilities::ECopyOptions::OnlyCopyEditOrInterpProperties|EditorUtilities::ECopyOptions::PropagateChangesToArcheypeInstances);
				NumChangedProperties = EditorUtilities::CopyActorProperties(Actor, BlueprintCDO, CopyOptions);
				if(NumChangedProperties > 0)
				{
					FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
				}
			}
		}

		// Set up a notification record to indicate success/failure
		FNotificationInfo NotificationInfo( FText::GetEmpty() );
		NotificationInfo.FadeInDuration = 1.0f;
		NotificationInfo.FadeOutDuration = 2.0f;
		NotificationInfo.bUseLargeFont = false;
		SNotificationItem::ECompletionState CompletionState;
		if( NumChangedProperties > 0 )
		{
			if( NumChangedProperties > 1 )
			{
				FFormatNamedArguments Args;
				Args.Add( TEXT("BlueprintName"), FText::FromName(Blueprint->GetFName() ) );
				Args.Add( TEXT("NumChangedProperties"), NumChangedProperties );
				Args.Add( TEXT("ActorName"), FText::FromString( Actor->GetActorLabel() ) );
				NotificationInfo.Text = FText::Format( LOCTEXT("PushToBlueprintDefaults_ApplySuccess", "Updated Blueprint {BlueprintName} ({NumChangedProperties} property changes applied from actor {ActorName})."), Args );
			}
			else
			{
				FFormatNamedArguments Args;
				Args.Add( TEXT("BlueprintName"), FText::FromName(Blueprint->GetFName() ) );
				Args.Add( TEXT("ActorName"), FText::FromString( Actor->GetActorLabel() ) );
				NotificationInfo.Text = FText::Format( LOCTEXT("PushOneToBlueprintDefaults_ApplySuccess", "Updated Blueprint {BlueprintName} (1 property change applied from actor {ActorName})."), Args );
			}
			CompletionState = SNotificationItem::CS_Success;
		}
		else
		{
			NotificationInfo.Text = LOCTEXT("PushToBlueprintDefaults_ApplyFailed", "No properties were copied");
			CompletionState = SNotificationItem::CS_Fail;
		}

		// Add the notification to the queue
		const auto Notification = FSlateNotificationManager::Get().AddNotification(NotificationInfo);
		Notification->SetCompletionState(CompletionState);
	}

	return FReply::Handled();
}

bool FActorDetails::ResetToBlueprintDefaults_IsEnabled( TWeakObjectPtr<UBlueprint> InBlueprint ) const
{
	bool bIsEnabled = false;
	if(SelectedActors.Num() == 1)
	{
		AActor* Actor = SelectedActors[0].Get();
		UBlueprint* Blueprint = InBlueprint.Get();
		if(Actor != NULL
			&& Blueprint != NULL
			&& Actor->GetClass()->ClassGeneratedBy == Blueprint)
		{
			AActor* BlueprintCDO = Actor->GetClass()->GetDefaultObject<AActor>();
			if(BlueprintCDO != NULL)
			{
				const auto CopyOptions = (EditorUtilities::ECopyOptions::Type)(EditorUtilities::ECopyOptions::PreviewOnly|EditorUtilities::ECopyOptions::OnlyCopyEditOrInterpProperties);
				bIsEnabled = EditorUtilities::CopyActorProperties(BlueprintCDO, Actor, CopyOptions) > 0;
			}
		}
	}

	return bIsEnabled;
}

FText FActorDetails::ResetToBlueprintDefaults_ToolTipText( TWeakObjectPtr<UBlueprint> InBlueprint ) const
{
	int32 NumChangedProperties = 0;

	if(SelectedActors.Num() == 1)
	{
		AActor* Actor = SelectedActors[0].Get();
		UBlueprint* Blueprint = InBlueprint.Get();
		if(Actor != NULL
			&& Blueprint != NULL
			&& Actor->GetClass()->ClassGeneratedBy == Blueprint)
		{
			AActor* BlueprintCDO = Actor->GetClass()->GetDefaultObject<AActor>();
			if(BlueprintCDO != NULL)
			{
				const auto CopyOptions = (EditorUtilities::ECopyOptions::Type)(EditorUtilities::ECopyOptions::PreviewOnly|EditorUtilities::ECopyOptions::OnlyCopyEditOrInterpProperties);
				NumChangedProperties += EditorUtilities::CopyActorProperties(BlueprintCDO, Actor, CopyOptions);
			}
		}
	}

	if (NumChangedProperties == 0)
	{
		return LOCTEXT("DisabledResetBlueprintDefaults_ToolTip", "Resets altered properties back to their Blueprint default values.");
	}
	else if (NumChangedProperties > 1)
	{
		return FText::Format( LOCTEXT("ResetToBlueprintDefaults_ToolTip", "Click to reset {0} changed properties to their Blueprint default values."), FText::AsNumber( NumChangedProperties ) );
	}
	else
	{
		return LOCTEXT("ResetOneToBlueprintDefaults_ToolTip", "Click to reset 1 changed property to its Blueprint default value.");
	}
}

FReply FActorDetails::ResetToBlueprintDefaults_OnClicked( TWeakObjectPtr<UBlueprint> InBlueprint )
{
	int32 NumChangedProperties = 0;

	if(SelectedActors.Num() == 1)
	{
		const FScopedTransaction Transaction( LOCTEXT("ResetToBlueprintDefaults_Transaction", "Reset to Blueprint Defaults") );

		// Perform the actual copy
		AActor* Actor = SelectedActors[0].Get();
		UBlueprint* Blueprint = InBlueprint.Get();
		if(Actor != NULL
			&& Blueprint != NULL
			&& Actor->GetClass()->ClassGeneratedBy == Blueprint)
		{
			AActor* BlueprintCDO = Actor->GetClass()->GetDefaultObject<AActor>();
			if(BlueprintCDO != NULL)
			{
				const auto CopyOptions = (EditorUtilities::ECopyOptions::Type)(EditorUtilities::ECopyOptions::OnlyCopyEditOrInterpProperties|EditorUtilities::ECopyOptions::CallPostEditChangeProperty);
				NumChangedProperties = EditorUtilities::CopyActorProperties(BlueprintCDO, Actor, CopyOptions);
			}
		}

		// Set up a notification record to indicate success/failure
		FNotificationInfo NotificationInfo( FText::GetEmpty() );
		NotificationInfo.FadeInDuration = 1.0f;
		NotificationInfo.FadeOutDuration = 2.0f;
		NotificationInfo.bUseLargeFont = false;
		SNotificationItem::ECompletionState CompletionState;
		if( NumChangedProperties > 0 )
		{
			if( NumChangedProperties > 1 )
			{
				FFormatNamedArguments Args;
				Args.Add( TEXT("BlueprintName"), FText::FromName(Blueprint->GetFName() ) );
				Args.Add( TEXT("NumChangedProperties"), NumChangedProperties );
				Args.Add( TEXT("ActorName"), FText::FromString( Actor->GetActorLabel() ) );
				NotificationInfo.Text = FText::Format( LOCTEXT("ResetToBlueprintDefaults_ApplySuccess", "Reset {ActorName} ({NumChangedProperties} property changes applied from Blueprint {BlueprintName})."), Args );
			}
			else
			{
				FFormatNamedArguments Args;
				Args.Add( TEXT("BlueprintName"), FText::FromName(Blueprint->GetFName() ) );
				Args.Add( TEXT("ActorName"), FText::FromString( Actor->GetActorLabel() ) );
				NotificationInfo.Text = FText::Format( LOCTEXT("ResetOneToBlueprintDefaults_ApplySuccess", "Reset {ActorName} (1 property change applied from Blueprint {BlueprintName})."), Args );
			}
			CompletionState = SNotificationItem::CS_Success;
		}
		else
		{
			NotificationInfo.Text = LOCTEXT("ResetToBlueprintDefaults_Failed", "No properties were reset");
			CompletionState = SNotificationItem::CS_Fail;
		}

		// Add the notification to the queue
		const auto Notification = FSlateNotificationManager::Get().AddNotification(NotificationInfo);
		Notification->SetCompletionState(CompletionState);
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
