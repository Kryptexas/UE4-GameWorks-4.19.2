// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "UseClassWithNonStaticPtrField.h"

static void Function_UseClassWithNonStaticPtrField()
{
#ifdef UNREAL_CODE_ANALYZER
	FTestClassWithNonStaticPtrField TestClassWithNonStaticPtrField;
#endif
}
