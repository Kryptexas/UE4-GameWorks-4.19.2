// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlutilityPrivatePCH.h"
#include "ClassViewerModule.h"
#include "ClassViewerFilter.h"
#include "BlutilityClasses.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/SClassPickerDialog.h"

class FBlutilityBlueprintFactoryFilter : public IClassViewerFilter
{
public:
	TSet< const UClass* > AllowedChildOfClasses;

	bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs )
	{
		return InFilterFuncs->IfInChildOfClassesSet(AllowedChildOfClasses, InClass) != EFilterReturn::Failed;
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) OVERRIDE
	{
		return InFilterFuncs->IfInChildOfClassesSet(AllowedChildOfClasses, InUnloadedClassData) != EFilterReturn::Failed;
	}
};

/////////////////////////////////////////////////////
// UEditorUtilityBlueprintFactory

UEditorUtilityBlueprintFactory::UEditorUtilityBlueprintFactory(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	UClass* DefaultParentClass = APlacedEditorUtilityBase::StaticClass();

	if ( GEditor )
	{
		if (GetDefault<UEditorExperimentalSettings>()->bEnableEditorUtilityBlueprints)
		{
			bCreateNew = true;
		}
	}

	bEditAfterNew = true;
	SupportedClass = UEditorUtilityBlueprint::StaticClass();
	ParentClass = DefaultParentClass;
}

bool UEditorUtilityBlueprintFactory::ConfigureProperties()
{
	// Null the parent class so we can check for selection later
	ParentClass = NULL;

	// Load the class viewer module to display a class picker
	FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

	// Fill in options
	FClassViewerInitializationOptions Options;
	Options.Mode = EClassViewerMode::ClassPicker;
	Options.DisplayMode = EClassViewerDisplayMode::ListView;
	Options.bShowObjectRootClass = true;

	// Only want blueprint actor base classes.
	Options.bIsBlueprintBaseOnly = true;

	// This will allow unloaded blueprints to be shown.
	Options.bShowUnloadedBlueprints = true;

	TSharedPtr< FBlutilityBlueprintFactoryFilter > Filter = MakeShareable(new FBlutilityBlueprintFactoryFilter);
	Options.ClassFilter = Filter;

	//Filter->AllowedChildOfClasses.Add(APlacedEditorUtilityBase::StaticClass());
	Filter->AllowedChildOfClasses.Add(UGlobalEditorUtilityBase::StaticClass());


	const FText TitleText = NSLOCTEXT("EditorFactories", "CreateBlueprintOptions", "Pick Parent Class");
	UClass* ChosenClass = NULL;
	const bool bPressedOk = SClassPickerDialog::PickClass(TitleText, Options, ChosenClass, UEditorUtilityBlueprint::StaticClass());
	if (bPressedOk)
	{
		ParentClass = ChosenClass;
	}

	return bPressedOk;
}

UObject* UEditorUtilityBlueprintFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	// Make sure we are trying to factory a blueprint, then create and init one
	check(Class->IsChildOf(UBlueprint::StaticClass()));

	if ((ParentClass == NULL) || !FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentClass))
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("ClassName"), (ParentClass != NULL) ? FText::FromString( ParentClass->GetName() ) : NSLOCTEXT("UnrealEd", "Null", "(null)") );
		FMessageDialog::Open( EAppMsgType::Ok, FText::Format( NSLOCTEXT("UnrealEd", "CannotCreateBlueprintFromClass", "Cannot create a blueprint based on the class '{0}'."), Args ) );
		return NULL;
	}
	else
	{
		return FKismetEditorUtilities::CreateBlueprint(ParentClass, InParent, Name, BPTYPE_Normal, UEditorUtilityBlueprint::StaticClass());
	}
}

void UEditorUtilityBlueprintFactory::OnClassPicked(UClass* InChosenClass)
{
	ParentClass = InChosenClass;
	PickerWindow.Pin()->RequestDestroyWindow();
}