// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Inheritance.h"

class FTestDerivedClass : public FTestBaseClass
{ };

static void Function_Inheritance()
{
#ifdef UNREAL_CODE_ANALYZER
	FTestDerivedClass TestDerivedClass;
#endif
}
