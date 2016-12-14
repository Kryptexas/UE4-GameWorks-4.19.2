// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "UseIfMacro.h"

static int Function_UseIfMacro()
{
#if IF_MACRO
	return 0;
#else
	return 1;
#endif
}
