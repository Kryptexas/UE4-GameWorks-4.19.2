// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PyWrapperStruct.h"
#include "PyUtil.h"

#if WITH_PYTHON

/** Python type for FVector */
extern PyTypeObject PyWrapperVectorType;

/** Python type for FVector2D */
extern PyTypeObject PyWrapperVector2DType;

/** Python type for FVector4 */
extern PyTypeObject PyWrapperVector4Type;

/** Python type for FQuat */
extern PyTypeObject PyWrapperQuatType;

/** Python type for FLinearColor */
extern PyTypeObject PyWrapperLinearColorType;

/** Initialize the PyWrapperMath types and add them to the given Python module */
void InitializePyWrapperMath(PyObject* PyModule);

/** Type for all UE4 exposed FVector instances */
typedef TPyWrapperInlineStruct<FVector> FPyWrapperVector;

/** Type for all UE4 exposed FVector2D instances */
typedef TPyWrapperInlineStruct<FVector2D> FPyWrapperVector2D;

/** Type for all UE4 exposed FVector4 instances */
typedef TPyWrapperInlineStruct<FVector4> FPyWrapperVector4;

/** Type for all UE4 exposed FQuat instances */
typedef TPyWrapperInlineStruct<FQuat> FPyWrapperQuat;

/** Type for all UE4 exposed FLinearColor instances */
typedef TPyWrapperInlineStruct<FLinearColor> FPyWrapperLinearColor;

Expose_TNameOf(FPyWrapperVector)
Expose_TNameOf(FPyWrapperVector2D)
Expose_TNameOf(FPyWrapperVector4)
Expose_TNameOf(FPyWrapperQuat)
Expose_TNameOf(FPyWrapperLinearColor)

typedef TPyPtr<FPyWrapperVector> FPyWrapperVectorPtr;
typedef TPyPtr<FPyWrapperVector2D> FPyWrapperVector2DPtr;
typedef TPyPtr<FPyWrapperVector4> FPyWrapperVector4Ptr;
typedef TPyPtr<FPyWrapperQuat> FPyWrapperQuatPtr;
typedef TPyPtr<FPyWrapperLinearColor> FPyWrapperLinearColorPtr;

#endif	// WITH_PYTHON
