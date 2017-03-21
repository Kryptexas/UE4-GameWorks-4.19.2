// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"

// Custom serialization version for all packages containing Niagara asset types
struct FNiagaraCustomVersion
{
	enum Type
	{
		// Before any version changes were made in niagara
		BeforeCustomVersionWasAdded = 0,

		// Reworked vm external function binding to be more robust.
		VMExternalFunctionBindingRework,
		
		// Making all Niagara files reference the version number, allowing post loading recompilation if necessary.
		PostLoadCompilationEnabled,

		// Moved some runtime cost from external functions into the binding step and used variadic templates to neaten that code greatly.
		VMExternalFunctionBindingReworkPartDeux,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

private:
	FNiagaraCustomVersion() {}
};
