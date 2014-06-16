// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemEditorPrivatePCH.h"

#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"

#include "Abilities/GameplayAbility.h"
#include "GameplayAbilityBlueprint.h"

#include "GameplayAbilityBlueprintGeneratedClass.h"

#include "GameplayAbilitiesBlueprintFactory.h"

#define LOCTEXT_NAMESPACE "UGameplayAbilitiesBlueprintFactory"

/*------------------------------------------------------------------------------
	UGameplayAbilitiesBlueprintFactory implementation.
------------------------------------------------------------------------------*/

UGameplayAbilitiesBlueprintFactory::UGameplayAbilitiesBlueprintFactory(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UGameplayAbilityBlueprint::StaticClass();
	ParentClass = UGameplayAbility::StaticClass();
}

bool UGameplayAbilitiesBlueprintFactory::ConfigureProperties()
{
	return true;
};

UObject* UGameplayAbilitiesBlueprintFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	// Make sure we are trying to factory a gameplay ability blueprint, then create and init one
	check(Class->IsChildOf(UGameplayAbilityBlueprint::StaticClass()));

	// If they selected an interface, force the parent class to be UInterface
	if (BlueprintType == BPTYPE_Interface)
	{
		ParentClass = UInterface::StaticClass();
	}

	if ( ( ParentClass == NULL ) || !FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentClass) || !ParentClass->IsChildOf(UGameplayAbility::StaticClass()) )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT("ClassName"), (ParentClass != NULL) ? FText::FromString( ParentClass->GetName() ) : LOCTEXT("Null", "(null)") );
		FMessageDialog::Open( EAppMsgType::Ok, FText::Format( LOCTEXT("CannotCreateGameplayAbilityBlueprint", "Cannot create a Gameplay Ability Blueprint based on the class '{ClassName}'."), Args ) );
		return NULL;
	}
	else
	{
		UGameplayAbilityBlueprint* NewBP = CastChecked<UGameplayAbilityBlueprint>(FKismetEditorUtilities::CreateBlueprint(ParentClass, InParent, Name, BlueprintType, UGameplayAbilityBlueprint::StaticClass(), UGameplayAbilityBlueprintGeneratedClass::StaticClass(), CallingContext));

		return NewBP;
	}
}

UObject* UGameplayAbilitiesBlueprintFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return FactoryCreateNew(Class, InParent, Name, Flags, Context, Warn, NAME_None);
}

#undef LOCTEXT_NAMESPACE
