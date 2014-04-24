// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UWidgetBlueprint

UWidgetBlueprint::UWidgetBlueprint(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UWidgetBlueprint::PostLoad()
{
	Super::PostLoad();

	for ( USlateWrapperComponent* Widget : WidgetTemplates )
	{
		Widget->ConnectEditorData();
	}
}

bool UWidgetBlueprint::ValidateGeneratedClass(const UClass* InClass)
{
	bool Result = Super::ValidateGeneratedClass(InClass);

	const UWidgetBlueprintGeneratedClass* GeneratedClass = Cast<const UWidgetBlueprintGeneratedClass>(InClass);
	if ( !ensure(GeneratedClass) )
	{
		return false;
	}
	const UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(GetBlueprintFromClass(GeneratedClass));
	if ( !ensure(Blueprint) )
	{
		return false;
	}

	for ( const USlateWrapperComponent* Template : Blueprint->WidgetTemplates )
	{
		if ( !ensure(Template && ( Template->GetOuter() == GeneratedClass )) )
		{
			return false;
		}
	}

	for ( const USlateWrapperComponent* Template : GeneratedClass->WidgetTemplates )
	{
		if ( !ensure(Template && ( Template->GetOuter() == GeneratedClass )) )
		{
			return false;
		}
	}

	return Result;
}

void UWidgetBlueprint::PostLoadSubobjects(FObjectInstancingGraph* OuterInstanceGraph)
{
	Super::PostLoadSubobjects(OuterInstanceGraph);

	UWidgetBlueprintGeneratedClass* BPGClass = Cast<UWidgetBlueprintGeneratedClass>(*GeneratedClass);
	if ( BPGClass )
	{
		BPGClass->WidgetTemplates = WidgetTemplates;
	}
}

#undef LOCTEXT_NAMESPACE 