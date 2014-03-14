// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __ActorIconFinder_h__
#define __ActorIconFinder_h__

#pragma once

class FClassIconFinder
{
public:
	/** Find the best fitting small icon to use for the supplied actor array */
	UNREALED_API static const FSlateBrush* FindIconForActors(const TArray< TWeakObjectPtr<AActor> >& InActors, UClass*& CommonBaseClass);

	/** Find the small icon to use for the supplied actor */
	UNREALED_API static const FSlateBrush* FindIconForActor( const TWeakObjectPtr<AActor>& InActor );
	
	/** Find the small icon name to use for the supplied actor */
	UNREALED_API static FName FindIconNameForActor( const TWeakObjectPtr<AActor>& InActor );

	/** Find the small icon to use for the supplied class */
	UNREALED_API static const FSlateBrush* FindIconForClass(UClass* InClass, const FName& InDefaultName = FName() );

	/** Find the small icon name to use for the supplied class */
	UNREALED_API static FName FindIconNameForClass(UClass* InClass, const FName& InDefaultName = FName() );
};

#endif // __ActorIconFinder_h__