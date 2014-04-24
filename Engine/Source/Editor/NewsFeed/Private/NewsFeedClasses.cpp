// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	NewsFeedClasses.cpp: Implements the module's script classes.
=============================================================================*/

#include "NewsFeedPrivatePCH.h"

/* UNewsFeedSettings interface
 *****************************************************************************/

UNewsFeedSettings::UNewsFeedSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
	, MaxItemsToShow(10)
	, ShowOnlyUnreadItems(true)
{ }


void UNewsFeedSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	SettingChangedEvent.Broadcast(Name);
}
