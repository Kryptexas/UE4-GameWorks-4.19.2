// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2017 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#include "foundation/PxPreprocessor.h"
#include "PxPhysicsVersion.h"
#include "NpMiddlewareInfo.h"

#if !PX_NX
	#error "This file should only be included by NX builds!!"
#endif

#include <nn/nn_Middleware.h>

namespace physx
{

#if PX_DEBUG
#define PHYSX_CONFIG "debug"
#elif PX_CHECKED
#define PHYSX_CONFIG "checked"
#elif PX_PROFILE
#define PHYSX_CONFIG "profile"
#else
#define PHYSX_CONFIG "release"
#endif

#define MODULE_STRING "PhysX-"              \
PX_STRINGIZE(PX_PHYSICS_VERSION_MAJOR) "_"  \
PX_STRINGIZE(PX_PHYSICS_VERSION_MINOR) "_"  \
PX_STRINGIZE(PX_PHYSICS_VERSION_BUGFIX) "-" \
PHYSX_CONFIG

NN_DEFINE_MIDDLEWARE(g_MiddlewareInfo, "NVIDIA", MODULE_STRING);

void NpSetMiddlewareInfo()
{
	NN_USING_MIDDLEWARE(g_MiddlewareInfo);
}

}
