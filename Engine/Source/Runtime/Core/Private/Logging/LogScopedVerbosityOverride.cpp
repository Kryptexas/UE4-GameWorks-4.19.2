// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "CorePrivatePCH.h"
#include "Logging/LogScopedVerbosityOverride.h"

/** Back up the existing verbosity for the category then sets new verbosity.*/
FLogScopedVerbosityOverride::FLogScopedVerbosityOverride(FLogCategoryBase * Category, ELogVerbosity::Type Verbosity)
	:	SavedCategory(Category)
{
	check (SavedCategory);
	SavedVerbosity = SavedCategory->GetVerbosity();
	SavedCategory->SetVerbosity(Verbosity);
}

/** Restore the verbosity overrides for the category to the previous value.*/
FLogScopedVerbosityOverride::~FLogScopedVerbosityOverride()
{
	SavedCategory->SetVerbosity(SavedVerbosity);
}

