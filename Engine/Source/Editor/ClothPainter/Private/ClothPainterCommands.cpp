// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ClothPainterCommands.h"

void FClothPainterCommands::RegisterCommands()
{

}

const FClothPainterCommands& FClothPainterCommands::Get()
{
	return TCommands<FClothPainterCommands>::Get();
}

