// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#if UE_ENABLE_ICU
#if ICU_OVERRIDE_LINKED_DATA
#include "CorePrivate.h"

namespace ICUDataOverride
{
	struct ICUCommonDataOverride
	{
		double bogus;
		uint8_t bytes[22453792]; 
	};

	const ICUCommonDataOverride* GetCommonDataOverride();
}
#endif //ICU_OVERRIDE_LINKED_DATA
#endif //UE_ENABLE_ICU