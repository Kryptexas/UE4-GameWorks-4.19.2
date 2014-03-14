// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#define PAPER2D_MODULE_NAME "Paper2D"

//////////////////////////////////////////////////////////////////////////
// IPaper2DModuleInterface

class IPaper2DModuleInterface : public IModuleInterface
{
};

// The 3D vector that corresponds to the sprite X axis
extern PAPER2D_API FVector PaperAxisX;

// The 3D vector that corresponds to the sprite Y axis
extern PAPER2D_API FVector PaperAxisY;

// The 3D vector that corresponds to the sprite Z axis
extern PAPER2D_API FVector PaperAxisZ;
