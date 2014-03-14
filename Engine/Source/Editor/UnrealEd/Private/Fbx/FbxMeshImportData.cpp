// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

UFbxMeshImportData::UFbxMeshImportData(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	NormalImportMethod = FBXNIM_ComputeNormals;
}