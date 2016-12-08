// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "UseClassWithNonStaticField.h"

static void Function_UseClassWithNonStaticField()
{
#ifdef UNREAL_CODE_ANALYZER
	FTestClassWithNonStaticField TestClassWithNonStaticField;
#endif
}
