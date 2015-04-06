/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#ifndef PX_PHYSICS_COMMON_ID_POOL
#define PX_PHYSICS_COMMON_ID_POOL

#include "Px.h"
#include "CmPhysXCommon.h"
#include "PsArray.h"

namespace physx
{
namespace Cm
{
	class IDPool
	{
		PxU32				currentID;
		Ps::Array<PxU32>	freeIDs;
	public:
        IDPool() : currentID(0), freeIDs(PX_DEBUG_EXP("IDPoolFreeIDs"))	{}

		void	freeID(PxU32 id)
		{
			// Allocate on first call
			// Add released ID to the array of free IDs
			freeIDs.pushBack(id);
		}

		void	freeAll()
		{
			currentID = 0;
			freeIDs.clear();
		}

		PxU32	getNewID()
		{
			// If recycled IDs are available, use them
			const PxU32 size = freeIDs.size();
			if(size)
			{
				const PxU32 id = freeIDs[size-1]; // Recycle last ID
				freeIDs.popBack();
				return id;
			}
			// Else create a new ID
			return currentID++;
		}
	};

} // namespace Cm

}

#endif
