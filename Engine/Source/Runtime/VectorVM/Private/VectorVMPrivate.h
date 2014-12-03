// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VectorVM.h"


namespace VectorVM
{
	/** Constants. */
	enum
	{
		ChunkSize = 128,
		ElementsPerVector = 4,
		VectorsPerChunk = ChunkSize / ElementsPerVector,
	};
}
