// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "StaticMeshImporter.h"
#include "USDImporter.h"
#include "USDConversionUtils.h"
#include "RawMesh.h"
#include "MeshUtilities.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "PackageName.h"
#include "Package.h"
#include "USDAssetImportData.h"
#include "Factories/Factory.h"

#define LOCTEXT_NAMESPACE "USDImportPlugin"

UStaticMesh* FUSDStaticMeshImporter::ImportStaticMesh(FUsdImportContext& ImportContext, const FUsdAssetPrimToImport& PrimToImport)
{
	IUsdPrim* Prim = PrimToImport.Prim;

	const FTransform& ConversionTransform = ImportContext.ConversionTransform;

	const FMatrix PrimToWorld = ImportContext.bApplyWorldTransformToGeometry ? USDToUnreal::ConvertMatrix(Prim->GetLocalToWorldTransform()) : FMatrix::Identity;

	FTransform FinalTransform = FTransform(PrimToWorld)*ConversionTransform;
	FMatrix FinalTransformIT = FinalTransform.ToInverseMatrixWithScale().GetTransposed();

	const bool bFlip = FinalTransform.GetDeterminant() > 0.0f;

	int32 NumLODs = PrimToImport.NumLODs;

	UUSDImportOptions* ImportOptions = nullptr;
	UStaticMesh* NewMesh = USDUtils::FindOrCreateObject<UStaticMesh>(ImportContext.Parent, ImportContext.ObjectName, ImportContext.ImportObjectFlags);
	check(NewMesh);
	UUSDAssetImportData* ImportData = Cast<UUSDAssetImportData>(NewMesh->AssetImportData);
	if (!ImportData)
	{
		ImportData = NewObject<UUSDAssetImportData>(NewMesh);
		ImportData->ImportOptions = DuplicateObject<UUSDImportOptions>(ImportContext.ImportOptions, ImportData);
		NewMesh->AssetImportData = ImportData;
	}
	ImportOptions = CastChecked<UUSDAssetImportData>(NewMesh->AssetImportData)->ImportOptions;
	check(ImportOptions);

	FString CurrentFilename = UFactory::GetCurrentFilename();
	if (!CurrentFilename.IsEmpty())
	{
		NewMesh->AssetImportData->Update(UFactory::GetCurrentFilename());
	}

	NewMesh->StaticMaterials.Empty();

	TArray<UMaterialInterface*> Materials;

	for (int32 LODIndex = 0; LODIndex < NumLODs; ++LODIndex)
	{
		FRawMesh RawTriangles;

		TArray<IUsdPrim*> PrimsWithGeometry;
		for (IUsdPrim* MeshPrim : PrimToImport.MeshPrims)
		{
			if (MeshPrim->GetNumLODs() > LODIndex)
			{
				// If the mesh has LOD children at this index then use that as the geom prim
				MeshPrim->SetActiveLODIndex(LODIndex);

				ImportContext.PrimResolver->FindMeshChildren(ImportContext, MeshPrim, false, PrimsWithGeometry);
			}
			else if (LODIndex == 0)
			{
				// If a mesh has no lods then it should only contribute to the base LOD
				PrimsWithGeometry.Add(MeshPrim);
			}
		}

		for (IUsdPrim* GeomPrim : PrimsWithGeometry)
		{
			// If we dont have a geom prim this might not be an error so dont message it.  The geom prim may not contribute to the LOD for whatever reason
			if (GeomPrim)
			{
				const FUsdGeomData* GeomDataPtr = GeomPrim->GetGeometryData();

				if (GeomDataPtr && IsTriangleMesh(GeomDataPtr))
				{
					const FUsdGeomData& GeomData = *GeomDataPtr;

					const int32 FaceOffset = RawTriangles.FaceMaterialIndices.Num();
					const int32 VertexOffset = RawTriangles.VertexPositions.Num();
					const int32 WedgeOffset = RawTriangles.WedgeIndices.Num();

					// Fill in the raw data
					{
						{
							// @todo Smoothing.  Not supported by USD
							RawTriangles.FaceSmoothingMasks.AddUninitialized(GeomData.FaceVertexCounts.size());
							for (int32 LocalFaceIndex = 0; LocalFaceIndex < GeomData.FaceVertexCounts.size(); ++LocalFaceIndex)
							{
								RawTriangles.FaceSmoothingMasks[FaceOffset + LocalFaceIndex] = 0xFFFFFFFF;
							}
						}

						// Positions
						{
							RawTriangles.VertexPositions.AddUninitialized(GeomData.Points.size());
							for (int32 LocalPointIndex = 0; LocalPointIndex < GeomData.Points.size(); ++LocalPointIndex)
							{
								const FUsdVectorData& Point = GeomData.Points[LocalPointIndex];
								FVector Pos = FVector(Point.X, Point.Y, Point.Z);
								Pos = FinalTransform.TransformPosition(Pos);
								RawTriangles.VertexPositions[VertexOffset + LocalPointIndex] = Pos;
							}
						}

						// Indices
						{
							RawTriangles.WedgeIndices.AddUninitialized(GeomData.FaceIndices.size());
							for (int32 LocalFaceIndex = 0; LocalFaceIndex < GeomData.FaceIndices.size(); ++LocalFaceIndex)
							{
								RawTriangles.WedgeIndices[WedgeOffset + LocalFaceIndex] = GeomData.FaceIndices[LocalFaceIndex] + VertexOffset;
							}
						}

						const int32 NumFaces = GeomData.FaceIndices.size() / 3;

						// Material names and indices 
						{

							// There must always be one material
							int32 NumMaterials = FMath::Max<int32>(1, GeomData.MaterialNames.size());

							TArray<int32> LocalToGlobalMaterialMap;
							LocalToGlobalMaterialMap.AddUninitialized(NumMaterials);
							FString BasePackageName = FPackageName::GetLongPackagePath(NewMesh->GetOutermost()->GetName());

							// Add a material slot for each material
							for (int32 LocalMaterialIndex = 0; LocalMaterialIndex < NumMaterials; ++LocalMaterialIndex)
							{

								UMaterialInterface* ExistingMaterial = nullptr;

								if (LocalMaterialIndex < GeomData.MaterialNames.size())
								{
									FString MaterialName = USDToUnreal::ConvertString(GeomData.MaterialNames[LocalMaterialIndex]);

									FText Error;
									BasePackageName /= MaterialName;

									FString MaterialPath = MaterialName;

									ExistingMaterial = UMaterialImportHelpers::FindExistingMaterialFromSearchLocation(MaterialPath, BasePackageName, ImportOptions->MaterialSearchLocation, Error);

									if (!Error.IsEmpty())
									{
										ImportContext.AddErrorMessage(EMessageSeverity::Error, Error);
									}
								}


								Materials.Add(ExistingMaterial ? ExistingMaterial : UMaterial::GetDefaultMaterial(MD_Surface));

								int32 GlobalIndex = NewMesh->StaticMaterials.AddUnique(ExistingMaterial ? ExistingMaterial : UMaterial::GetDefaultMaterial(MD_Surface));
								NewMesh->SectionInfoMap.Set(LODIndex, GlobalIndex, FMeshSectionInfo(GlobalIndex));

								LocalToGlobalMaterialMap[LocalMaterialIndex] = GlobalIndex;
							}

							// Material Indices 
							{
								RawTriangles.FaceMaterialIndices.AddZeroed(GeomData.FaceVertexCounts.size());
								for (int32 FaceMaterialIndex = 0; FaceMaterialIndex < GeomData.FaceVertexCounts.size(); ++FaceMaterialIndex)
								{
									RawTriangles.FaceMaterialIndices[FaceOffset + FaceMaterialIndex] = LocalToGlobalMaterialMap[GeomData.FaceMaterialIndices[FaceMaterialIndex]];
								}
							}

						}

						// UVs and normals
						{

							if (GeomData.NumUVs > 0)
							{
								for (int32 UVIndex = 0; UVIndex < GeomData.NumUVs; ++UVIndex)
								{
									RawTriangles.WedgeTexCoords[UVIndex].AddUninitialized(GeomData.FaceIndices.size());
								}
							}
							else
							{
								// Have to at least have one UV set
								ImportContext.AddErrorMessage(EMessageSeverity::Warning,
									FText::Format(LOCTEXT("StaticMeshesHaveNoUVS", "{0} (LOD {1}) has no UVs.  At least one valid UV set should exist on a static mesh. This mesh will likely have rendering issues"),
										FText::FromString(ImportContext.ObjectName),
										FText::AsNumber(LODIndex)));

								RawTriangles.WedgeTexCoords[0].AddZeroed(GeomData.FaceIndices.size());
							}

							// Add one normal per tri vert
							RawTriangles.WedgeTangentZ.AddUninitialized(NumFaces * 3);

							for (int32 FaceIndex = 0; FaceIndex < NumFaces; ++FaceIndex)
							{
								const int32 DestFaceIndex = FaceOffset + FaceIndex;

								// Assume triangles for now
								for (int32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
								{
									const int32 LocalWedgeIndex = FaceIndex * 3 + CornerIndex;

									const int32 WedgeIndex = WedgeOffset + FaceIndex * 3 + CornerIndex;

									// Normals
									if (GeomData.Normals.size() > 0)
									{
										// Note about this mapping:  Normals are not primvars in USD files.  If the normals do not have a 1:1 mapping with indices we assume they are face varying
										// todo: fix this in the usd wrapper not here so it is consistent with UVs 
										const int32 NormalIndex = GeomData.Normals.size() != GeomData.FaceIndices.size() ? GeomData.FaceIndices[LocalWedgeIndex] : LocalWedgeIndex;

										check(NormalIndex < GeomData.Normals.size());

										const FUsdVectorData& Normal = GeomData.Normals[NormalIndex];
										//FVector TransformedNormal = ConversionMatrixIT.TransformVector(PrimToWorldIT.TransformVector(FVector(Normal.X, Normal.Y, Normal.Z)));
										FVector TransformedNormal = FinalTransformIT.TransformVector(FVector(Normal.X, Normal.Y, Normal.Z));

										RawTriangles.WedgeTangentZ[WedgeIndex] = TransformedNormal.GetSafeNormal();
									}


									// UVs
									if (GeomData.NumUVs > 0)
									{
										for (int32 UVIndex = 0; UVIndex < GeomData.NumUVs; ++UVIndex)
										{
											TArray<FVector2D>& TexCoords = RawTriangles.WedgeTexCoords[UVIndex];

											EUsdInterpolationMethod UVInterpMethod = GeomData.UVs[UVIndex].UVInterpMethod;

											// Get the index into the point array for this wedge
											const int32 PointIndex = UVInterpMethod == EUsdInterpolationMethod::FaceVarying ? LocalWedgeIndex : GeomData.FaceIndices[LocalWedgeIndex];

											// In this mode there is a single vertex per vertex so 
											// the point index should match up
											check(PointIndex < GeomData.UVs[UVIndex].Coords.size());
											const FUsdVector2Data& UV = GeomData.UVs[UVIndex].Coords[PointIndex];

											// Flip V for Unreal uv's which match directx
											TexCoords[WedgeIndex] = FVector2D(UV.X, -UV.Y);
										}
									}

								}
							}
						}


						if (bFlip)
						{
							// Flip anything that is indexed
							for (int32 FaceIndex = 0; FaceIndex < NumFaces; ++FaceIndex)
							{
								const int32 I0 = WedgeOffset + FaceIndex * 3 + 0;
								const int32 I2 = WedgeOffset + FaceIndex * 3 + 2;
								Swap(RawTriangles.WedgeIndices[I0], RawTriangles.WedgeIndices[I2]);

								if (RawTriangles.WedgeTangentZ.Num())
								{
									Swap(RawTriangles.WedgeTangentZ[I0], RawTriangles.WedgeTangentZ[I2]);
								}

								for (int32 TexCoordIndex = 0; TexCoordIndex < MAX_MESH_TEXTURE_COORDS; ++TexCoordIndex)
								{
									TArray<FVector2D>& TexCoords = RawTriangles.WedgeTexCoords[TexCoordIndex];
									if (TexCoords.Num() > 0)
									{
										Swap(TexCoords[I0], TexCoords[I2]);
									}
								}
							}
						}
					}
				}
				else
				{
					ImportContext.AddErrorMessage(EMessageSeverity::Error, FText::Format(LOCTEXT("StaticMeshesMustBeTriangulated", "{0} is not a triangle mesh. Static meshes must be triangulated to import"), FText::FromString(ImportContext.ObjectName)));

					if(NewMesh)
					{
						NewMesh->ClearFlags(RF_Standalone);
						NewMesh = nullptr;
					}
					break;
				}
			}
		}

		
		if(NewMesh)
		{
			if (!NewMesh->SourceModels.IsValidIndex(LODIndex))
			{
				// Add one LOD 
				NewMesh->SourceModels.AddDefaulted();
			}

			FStaticMeshSourceModel& SrcModel = NewMesh->SourceModels[LODIndex];

			RawTriangles.CompactMaterialIndices();

			if(RawTriangles.IsValidOrFixable())
			{

				SrcModel.RawMeshBulkData->SaveRawMesh(RawTriangles);

				// Recompute normals if we didnt import any
				SrcModel.BuildSettings.bRecomputeNormals = RawTriangles.WedgeTangentZ.Num() == 0;

				// Always recompute tangents as USD files do not contain tangent information
				SrcModel.BuildSettings.bRecomputeTangents = true;

				// Use mikktSpace if we have normals
				SrcModel.BuildSettings.bUseMikkTSpace = RawTriangles.WedgeTangentZ.Num() != 0;
				SrcModel.BuildSettings.bGenerateLightmapUVs = true;
				SrcModel.BuildSettings.bBuildAdjacencyBuffer = false;
				SrcModel.BuildSettings.bBuildReversedIndexBuffer = false;
			}
			else
			{
				ImportContext.AddErrorMessage(EMessageSeverity::Error, FText::Format(LOCTEXT("StaticMeshesNoValidSourceData","'{0}' does not have valid source data, mesh will not be imported"), FText::FromString(NewMesh->GetName())));
				NewMesh->ClearFlags(RF_Standalone);
				NewMesh = nullptr;
			}
		}

	}

	if(NewMesh)
	{
		NewMesh->ImportVersion = EImportStaticMeshVersion::BeforeImportStaticMeshVersionWasAdded;

		NewMesh->CreateBodySetup();

		NewMesh->SetLightingGuid();

		NewMesh->PostEditChange();
	}

	return NewMesh;
}

bool FUSDStaticMeshImporter::IsTriangleMesh(const FUsdGeomData* GeomData)
{
	bool bIsTriangleMesh = true;
	for (int32 VertexCount : GeomData->FaceVertexCounts)
	{
		if (VertexCount != 3)
		{
			bIsTriangleMesh = false;
			break;
		}
	}

	return bIsTriangleMesh;
}

#undef LOCTEXT_NAMESPACE
