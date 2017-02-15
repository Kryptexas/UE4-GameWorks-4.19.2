// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FTestClassWithStaticField
{
public:
	static int StaticField;
};

int FTestClassWithStaticField::StaticField = 5;
