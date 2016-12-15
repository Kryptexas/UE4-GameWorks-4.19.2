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
#include "PsFPU.h"

#include <cfenv>

physx::shdfnd::FPUGuard::FPUGuard()
{
	PX_COMPILE_TIME_ASSERT(sizeof(fenv_t) <= sizeof(mControlWords));

	fegetenv(reinterpret_cast<fenv_t*>(mControlWords));
	fesetenv(FE_DFL_ENV);

	//!!!NX  doesn't support fedisableexcept
	// need to explicitly disable exceptions because fesetenv does not modify
	// the sse control word on 32bit linux (64bit is fine, but do it here just be sure)
	//fedisableexcept(FE_ALL_EXCEPT);

	fesetround(FE_TONEAREST); //!!!NX  keep until this is the default mode
}

physx::shdfnd::FPUGuard::~FPUGuard()
{
	fesetenv(reinterpret_cast<fenv_t*>(mControlWords));
}

PX_FOUNDATION_API void physx::shdfnd::enableFPExceptions()
{
	feclearexcept(FE_ALL_EXCEPT);
	//!!!NX  doesn't support feenableexcept
	//feenableexcept(FE_INVALID|FE_DIVBYZERO|FE_OVERFLOW);	
}

PX_FOUNDATION_API void physx::shdfnd::disableFPExceptions()
{
	//!!!NX  doesn't support fedisableexcept
	//fedisableexcept(FE_ALL_EXCEPT);
}
