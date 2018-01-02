// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "GLTFStaticMesh.h"
#include "GLTFPackage.h"

#include "Engine/StaticMesh.h"
#include "RawMesh.h"
#include "Materials/Material.h"
#include "AssetRegistryModule.h"

using namespace GLTF;

template <typename T>
TArray<T> ReIndex(const TArray<T>& Source, const TArray<uint32>& Indices)
{
	TArray<T> Result;
	Result.Reserve(Indices.Num());
	for (uint32 Index : Indices)
	{
		Result.Add(Source[Index]);
	}
	return Result;
}

static FVector ConvertVec3(const FVector& In)
{
	// glTF uses a right-handed coordinate system, with Y up.
	// UE4 uses a left-handed coordinate system, with Z up.
	return { In.X, In.Z, In.Y };
}

static FVector ConvertPosition(const FVector& In)
{
	constexpr float Scale = 100.0f; // glTF (m) to UE4 (cm)
	return Scale * ConvertVec3(In);
}

// Convert tangent from Vec4 (glTF) to Vec3 (Unreal)
static FVector Tanslate(const FVector4& In)
{
	// Ignore In.W for now. (TODO: the right thing)
	// Its sign indicates handedness of the tangent basis.
	return ConvertVec3({ In.X, In.Y, In.Z });
}

// Convert tangents from Vec4 (glTF) to Vec3 (Unreal)
static TArray<FVector> Tanslate(const TArray<FVector4>& In)
{
	TArray<FVector> Result;
	Result.Reserve(In.Num());

	for (const FVector& Vec : In)
	{
		Result.Add(Tanslate(Vec));
	}

	return Result;
}

static void AppendWedgeIndices(FRawMesh& RawMesh, const TArray<uint32>& PrimIndices)
{
	// Indices come in Prim-relative, we need them Mesh-relative.
	// Simply offset each index by the number of vertices already in the mesh.
	// Call this *before* adding this Prim's vertex data.
	const uint32 Offset = RawMesh.VertexPositions.Num();
	RawMesh.WedgeIndices.Reserve(RawMesh.WedgeIndices.Num() + PrimIndices.Num());
	for (uint32 Index : PrimIndices)
	{
		RawMesh.WedgeIndices.Add(Index + Offset);
	}
}

static TArray<FVector> GenerateFlatNormals(const TArray<FVector>& Positions, const TArray<uint32>& Indices)
{
	TArray<FVector> Normals;
	const uint32 N = Indices.Num();
	check(N % 3 == 0);
	Normals.AddUninitialized(N);

	for (uint32 i = 0; i < N; i += 3)
	{
		const FVector& A = Positions[Indices[i]];
		const FVector& B = Positions[Indices[i + 1]];
		const FVector& C = Positions[Indices[i + 2]];

		const FVector Normal = FVector::CrossProduct(A - B, A - C).GetSafeNormal();

		// Same for each corner of the triangle.
		Normals[i] = Normal;
		Normals[i + 1] = Normal;
		Normals[i + 2] = Normal;
	}

	return Normals;
}

// Add N copies of Value to end of Array
template <typename T>
static void AddN(TArray<T>& Array, T Value, uint32 N)
{
	Array.Reserve(Array.Num() + N);
	while (N--)
	{
		Array.Add(Value);
	}
}

static void AssignMaterials(UStaticMesh* StaticMesh, FRawMesh& RawMesh, const TArray<UMaterial*>& Materials, const TSet<int32>& MaterialIndices)
{
	// Create material slots for this mesh, only for the materials it uses.

	// Sort material indices so slots will be in same order as glTF file. Likely the same order as content creation app!
	// (first entry will be INDEX_NONE if present)
	TArray<int32> SortedMaterialIndices = MaterialIndices.Array();
	SortedMaterialIndices.Sort();

	const int32 N = MaterialIndices.Num();

	TMap<int32, int32> MaterialIndexToSlot;
	MaterialIndexToSlot.Reserve(N);
	StaticMesh->StaticMaterials.Reserve(N);

	for (int32 MaterialIndex : SortedMaterialIndices)
	{
		UMaterial* Mat;
		int32 MeshSlot;

		if (MaterialIndex == INDEX_NONE)
		{
			// Add a slot for the default material.
			static UMaterial* DefaultMat = UMaterial::GetDefaultMaterial(MD_Surface);
			Mat = DefaultMat;
			MeshSlot = StaticMesh->StaticMaterials.Add(DefaultMat);
		}
		else
		{
			// Add a slot for a real material.
			Mat = Materials[MaterialIndex];
			FName MatName(*(Mat->GetName()));
			MeshSlot = StaticMesh->StaticMaterials.Emplace(Mat, MatName, MatName);
		}

		MaterialIndexToSlot.Add(MaterialIndex, MeshSlot);

		StaticMesh->SectionInfoMap.Set(0, MeshSlot, FMeshSectionInfo(MeshSlot));
	}

	// Replace material index references with this mesh's slots.
	for (int32& Index : RawMesh.FaceMaterialIndices)
	{
		Index = MaterialIndexToSlot[Index];
	}
}

UStaticMesh* ImportStaticMesh(const FAsset& Asset, const TArray<UMaterial*>& Materials, UObject* InParent, FName InName, EObjectFlags Flags, uint32 Index)
{
	// We should warn if certain things are "fixed up" during import.
	bool bDidGenerateTexCoords = false;
	bool bDidGenerateTangents = false;
	bool bMeshUsesEmptyMaterial = false;

	const FMesh& Mesh = Asset.Meshes[Index];

	FString AssetName;
	UPackage* AssetPackage = GetAssetPackageAndName<UStaticMesh>(InParent, Mesh.Name, TEXT("SM"), InName, Index, AssetName);

	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(AssetPackage, FName(*AssetName), Flags);

	StaticMesh->SourceModels.AddDefaulted();
	FStaticMeshSourceModel& SourceModel = StaticMesh->SourceModels.Last();
	FMeshBuildSettings& Settings = SourceModel.BuildSettings;

	Settings.bRecomputeNormals = false;
	Settings.bRecomputeTangents = !Mesh.HasTangents();
	Settings.bUseMikkTSpace = true;

	Settings.bRemoveDegenerates = false;
	Settings.bBuildAdjacencyBuffer = false;
	Settings.bBuildReversedIndexBuffer = false;

	Settings.bUseHighPrecisionTangentBasis = false;
	Settings.bUseFullPrecisionUVs = false;

	Settings.bGenerateLightmapUVs = false; // set to true if asset has no UV1 ?

	FRawMesh RawMesh;
	TSet<int32> MaterialIndicesUsed;

	for (const FPrimitive& Prim : Mesh.Primitives)
	{
		const uint32 TriCount = Prim.TriangleCount();
		const TArray<uint32> Indices = Prim.GetTriangleIndices();

		// Remember which primitives use which materials.
		MaterialIndicesUsed.Add(Prim.MaterialIndex);
		AddN(RawMesh.FaceMaterialIndices, Prim.MaterialIndex, TriCount);

		// Make all faces part of the same smoothing group, so Unreal will combine identical adjacent verts.
		constexpr uint32 SmoothMask = 1;
		AddN(RawMesh.FaceSmoothingMasks, SmoothMask, TriCount);
		// (Is there a way to set auto-gen smoothing threshold? glTF spec says to generate flat normals if they're not specified.
		//   We want to combine identical verts whether they're smooth neighbors or triangles belonging to the same flat polygon.)

		AppendWedgeIndices(RawMesh, Indices);

		const TArray<FVector>& Positions = Prim.GetPositions();
		RawMesh.VertexPositions.Append(Positions);

		// glTF does not guarantee each primitive within a mesh has the same attributes.
		// Fill in gaps as needed:
		// - missing normals will be flat, based on triangle orientation
		// - missing UVs will be (0,0)
		// - missing tangents will be (0,0,1)

		if (Prim.HasNormals())
		{
			RawMesh.WedgeTangentZ.Append(ReIndex(Prim.GetNormals(), Indices));
		}
		else
		{
			RawMesh.WedgeTangentZ.Append(GenerateFlatNormals(Positions, Indices));
		}

		if (Prim.HasTangents())
		{
			// glTF stores tangent as Vec4, with W component indicating handedness of tangent basis.
			const TArray<FVector> Tangents = Tanslate(Prim.GetTangents());
			RawMesh.WedgeTangentX.Append(ReIndex(Tangents, Indices));
		}
		else if (Mesh.HasTangents())
		{
			// If other primitives in this mesh have tangents, generate filler ones for this primitive, to avoid gaps.
			AddN(RawMesh.WedgeTangentX, FVector(0.0f, 0.0f, 1.0f), Prim.VertexCount());
			bDidGenerateTangents = true;
		}

		for (uint32 UV : { 0, 1 })
		{
			if (Prim.HasTexCoords(UV))
			{
				RawMesh.WedgeTexCoords[UV].Append(ReIndex(Prim.GetTexCoords(UV), Indices));
			}
			else if (UV == 0 || Mesh.HasTexCoords(UV))
			{
				// Unreal StaticMesh must have UV channel 0.
				// glTF doesn't require this since not all materials need texture coordinates.
				// We also fill UV channel >= 1 for this primitive if other primitives have it, to avoid gaps.
				RawMesh.WedgeTexCoords[UV].AddZeroed(Prim.VertexCount());
				bDidGenerateTexCoords = true;
			}
		}
	}

	// Transform from glTF -> UE4 coordinate system.
	// Tangents already transformed during conversion.

	for (FVector& Pos : RawMesh.VertexPositions)
	{
		Pos = ConvertPosition(Pos);
	}

	for (FVector& Normal : RawMesh.WedgeTangentZ)
	{
		Normal = ConvertVec3(Normal);
	}

	// Finish processing materials for this mesh.
	AssignMaterials(StaticMesh, RawMesh, Materials, MaterialIndicesUsed);
	// RawMesh.CompactMaterialIndices(); // needed?
	bMeshUsesEmptyMaterial = MaterialIndicesUsed.Contains(INDEX_NONE);

	SourceModel.RawMeshBulkData->SaveRawMesh(RawMesh);
	StaticMesh->ImportVersion = EImportStaticMeshVersion::BeforeImportStaticMeshVersionWasAdded;
	StaticMesh->PostEditChange();

	// Set the dirty flag so this package will get saved later
	AssetPackage->SetDirtyFlag(true);
	FAssetRegistryModule::AssetCreated(StaticMesh);

	// TODO: warn if certain things were "fixed up" during import.

	return StaticMesh;
}

TArray<UStaticMesh*> ImportStaticMeshes(const FAsset& Asset, const TArray<UMaterial*>& Materials, UObject* InParent, FName InName, EObjectFlags Flags)
{
	const uint32 MeshCount = Asset.Meshes.Num();
	TArray<UStaticMesh*> Result;
	Result.Reserve(MeshCount);

	for (uint32 Index = 0; Index < MeshCount; ++Index)
	{
		UStaticMesh* StaticMesh = ImportStaticMesh(Asset, Materials, InParent, InName, Flags, Index);
		if (StaticMesh)
		{
			Result.Add(StaticMesh);
		}
	}

	return Result;
}
