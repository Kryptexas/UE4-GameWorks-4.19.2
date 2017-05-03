// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	ApplePlatformAtomics.h: Apple platform Atomics functions
==============================================================================================*/

#pragma once
#include "GenericPlatform/GenericPlatformAtomics.h"
#include "CoreTypes.h"
#include "Clang/ClangPlatformAtomics.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

/**
 * Apple implementation of the Atomics OS functions
 **/
struct CORE_API FApplePlatformAtomics : public FClangPlatformAtomics
{
};

PRAGMA_ENABLE_DEPRECATION_WARNINGS

typedef FApplePlatformAtomics FPlatformAtomics;

