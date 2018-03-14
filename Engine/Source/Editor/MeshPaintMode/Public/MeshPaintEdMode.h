// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMeshPaintMode.h"

class FModeToolkit;

/**
 * Mesh Paint editor mode
 */
class FEdModeMeshPaint : public IMeshPaintEdMode
{
public:
	/** Constructor */
	FEdModeMeshPaint() {}

	/** Destructor */
	virtual ~FEdModeMeshPaint() {}
	virtual void Initialize() override;
	virtual TSharedPtr< FModeToolkit> GetToolkit() override;
};