// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	VectorVMPrivate.h: Private definitions for the vector virtual machine.
==============================================================================*/

#pragma once
#include "VectorVM.h"

namespace VectorVM
{
	/** Constants. */
	enum
	{
		ChunkSize = 64,
		ElementsPerVector = 4,
		VectorsPerChunk = ChunkSize / ElementsPerVector,
	};
}
