// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SuperSearchPrivatePCH.h"

#if WITH_EDITOR

#include "SuperSearchSettings.h"

void USuperSearchSettings::PostInitProperties()
{
	Super::PostInitProperties();

	FSuperSearchModule::Get().SetSearchEngine(SearchEngine);
}

void USuperSearchSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if ( Name == GET_MEMBER_NAME_CHECKED(USuperSearchSettings, SearchEngine) )
	{
		FSuperSearchModule::Get().SetSearchEngine(SearchEngine);
	}
}

#endif