// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DerivedDataCacheCommandlet.cpp: Commandlet for DDC maintenence
=============================================================================*/

#pragma once

#include "DerivedDataCacheCommandlet.generated.h"


UCLASS()
class UDerivedDataCacheCommandlet : public UCommandlet
{
	GENERATED_UCLASS_BODY()
	// Begin UCommandlet Interface
	virtual int32 Main(const FString& Params) OVERRIDE;
	// End UCommandlet Interface

	// We hook this up to a delegate to avoid reloading textures and whatnot
	TSet<FString> PackagesToNotReload;

	void MaybeMarkPackageAsAlreadyLoaded(UPackage *Package);
};


