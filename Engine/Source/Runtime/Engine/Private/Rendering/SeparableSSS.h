// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
SeparableSSS.h: Computing the kernel for the Separable Screen Space Subsurface Scattering
=============================================================================*/

#pragma once

// @param Out, needs to be preallocated with the expected size (RGB is the sample weight, A is the offset), [0] is the center samples, following elements need to be mirrored with A, -A
// @parma SubsurfaceColor see SubsurfaceProfile.h
// @parma FalloffColor see SubsurfaceProfile.h
void ComputeMirroredSSSKernel(TArray<FLinearColor>& Out, FLinearColor SubsurfaceColor, FLinearColor FalloffColor);


