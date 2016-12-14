// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "TemplatedClassInstance.h"
#include "UseClass.h"

static void Function_TemplatedClassInstance()
{
#ifdef UNREAL_CODE_ANALYZER
	FTemplateClass<FTestClass> TemplateClass;
#endif
}
