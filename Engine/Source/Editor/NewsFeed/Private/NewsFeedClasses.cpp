// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "NewsFeedPrivatePCH.h"


/* UNewsFeedSettings structors
 *****************************************************************************/

UNewsFeedSettings::UNewsFeedSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
	, MaxItemsToShow(10)
	, ShowOnlyUnreadItems(true)
{ }


/* UObject overrides
 *****************************************************************************/

void UNewsFeedSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	SettingChangedEvent.Broadcast(Name);
}
