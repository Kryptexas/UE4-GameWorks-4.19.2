// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SimplygonUtilitiesPrivatePCH.h"

#include "SimplygonUtilitiesModule.h"

#include "RawMesh.h"
#include "MeshUtilities.h"
#include "MaterialExportUtils.h"

#define LOCTEXT_NAMESPACE "SimplygonUtilities"

IMPLEMENT_MODULE( FSimplygonUtilities, SimplygonUtilities )


void FSimplygonUtilities::StartupModule()
{
}

void FSimplygonUtilities::ShutdownModule()
{
	
}

FMeshMaterialReductionData::FMeshMaterialReductionData(FRawMesh* InMesh, bool InReleaseMesh)
	: Mesh(InMesh)
	, bReleaseMesh(InReleaseMesh)
{ }

FMeshMaterialReductionData::~FMeshMaterialReductionData()
{
	if (bReleaseMesh)
	{
		delete Mesh;
	}
}

FMeshMaterialReductionData* FSimplygonUtilities::CreateMeshReductionData(FRawMesh* InMesh, bool InReleaseMesh)
{
	return new FMeshMaterialReductionData(InMesh, InReleaseMesh);
}


#undef LOCTEXT_NAMESPACE
