// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/** 
 * InterpFilter_Custom: Filter class for filtering matinee groups.  
 * Used by the matinee editor to let users organize tracks/groups.
 *
 */

#pragma once
#include "InterpFilter_Custom.generated.h"

UCLASS(HeaderGroup=Interpolation, MinimalAPI)
class UInterpFilter_Custom : public UInterpFilter
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITORONLY_DATA
	/** Which groups are included in this filter. */
	UPROPERTY()
	TArray<class UInterpGroup*> GroupsToInclude;

#endif // WITH_EDITORONLY_DATA

	// Begin UInterpFilter Interface
	virtual void FilterData(class AMatineeActor* InMatineeActor) OVERRIDE;
	// End UInterpFilter Interface
};

