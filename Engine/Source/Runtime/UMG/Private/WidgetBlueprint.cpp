// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UWidgetBlueprint

UWidgetBlueprint::UWidgetBlueprint(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

bool UWidgetBlueprint::ValidateGeneratedClass(const UClass* InClass)
{
	bool Result = Super::ValidateGeneratedClass(InClass);

	//for ( auto CompIt = Blueprint->Timelines.CreateConstIterator(); CompIt; ++CompIt )
	//{
	//	const UTimelineTemplate* Template = ( *CompIt );
	//	if ( !ensure(Template && ( Template->GetOuter() == GeneratedClass )) )
	//	{
	//		return false;
	//	}
	//}

	//for ( auto CompIt = GeneratedClass->Timelines.CreateConstIterator(); CompIt; ++CompIt )
	//{
	//	const UTimelineTemplate* Template = ( *CompIt );
	//	if ( !ensure(Template && ( Template->GetOuter() == GeneratedClass )) )
	//	{
	//		return false;
	//	}
	//}

	return Result;
}