#include "StaticMeshImporter.h"
#include "USDImporter.h"
#include "USDConversionUtils.h"
#include "RawMesh.h"
#include "MeshUtilities.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "PackageName.h"
#include "Package.h"

#define LOCTEXT_NAMESPACE "USDImportPlugin"

UStaticMesh* FUSDStaticMeshImporter::ImportStaticMesh(FUsdImportContext& ImportContext, const FUsdPrimToImport& PrimToImport)
{
	IUsdPrim* Prim = PrimToImport.Prim;

	const FTransform& ConversionTransform = ImportContext.ConversionTransform;

	const FMatrix PrimToWorld = ImportContext.bApplyWorldTransformToGeometry ? USDToUnreal::ConvertMatrix(Prim->GetLocalToWorldTransform()) : FMatrix::Identity;

	FTransform FinalTransform = FTransform(PrimToWorld)*ConversionTransform;
	FMatrix FinalTransformIT = FinalTransform.ToInverseMatrixWithScale().GetTransposed();

	const bool bFlip = FinalTransform.GetDeterminant() > 0.0f;

	int LODIndex = 0;
	const FUsdGeomData* GeomDataPtr = PrimToImport.GetGeomData(LODIndex, UnrealUSDWrapper::GetDefaultTimeCode());

	UStaticMesh* ImportedMesh = nullptr;
	if (GeomDataPtr)
	{
		if(IsTriangleMesh(GeomDataPtr))
		{
			ImportedMesh = USDUtils::FindOrCreateObject<UStaticMesh>(ImportContext.Parent, ImportContext.ObjectName, ImportContext.ObjectFlags);
			check(ImportedMesh);

			const FUsdGeomData& GeomData = *GeomDataPtr;
			FRawMesh RawTriangles;

			// Fill in the raw data
			{
				{
					// @todo Smoothing.  Not supported by USD
					RawTriangles.FaceSmoothingMasks.AddUninitialized(GeomData.FaceVertexCounts.size());
					for (int32 FaceIndex = 0; FaceIndex < RawTriangles.FaceSmoothingMasks.Num(); ++FaceIndex)
					{
						RawTriangles.FaceSmoothingMasks[FaceIndex] = 0xFFFFFFFF;
					}
				}

			

				// Positions
				{
					RawTriangles.VertexPositions.Reserve(GeomData.Points.size());
					for (const FUsdVectorData& Point : GeomData.Points)
					{
						FVector Pos = FVector(Point.X, Point.Y, Point.Z);
						Pos = FinalTransform.TransformPosition(Pos);
						RawTriangles.VertexPositions.Add(Pos);
					}
				}

				// Indices
				{
					RawTriangles.WedgeIndices.AddUninitialized(GeomData.FaceIndices.size());
					FMemory::Memcpy(&RawTriangles.WedgeIndices[0], &GeomData.FaceIndices[0], sizeof(int32)*RawTriangles.WedgeIndices.Num());
				}

				int32 NumFaces = RawTriangles.WedgeIndices.Num() / 3;


				// Material Indices 
				{
					RawTriangles.FaceMaterialIndices.AddZeroed(GeomData.FaceVertexCounts.size());
					FMemory::Memcpy(&RawTriangles.FaceMaterialIndices[0], &GeomData.FaceMaterialIndices[0], sizeof(int32)*GeomData.FaceMaterialIndices.size());
				}


				// UVs and normals
				{
					if (GeomData.NumUVs > 0)
					{
						for (int32 UVIndex = 0; UVIndex < GeomData.NumUVs; ++UVIndex)
						{
							RawTriangles.WedgeTexCoords[UVIndex].AddUninitialized(RawTriangles.WedgeIndices.Num());
						}
					}
					else
					{
						// Have to at least have one UV set
						ImportContext.AddErrorMessage(EMessageSeverity::Warning,
							FText::Format(LOCTEXT("StaticMeshesHaveNoUVS", "{0} has no UVs.  At least one valid UV set should exist on a static mesh. This mesh will likely have rendering issues"), FText::FromString(ImportContext.ObjectName)));

						RawTriangles.WedgeTexCoords[0].AddZeroed(RawTriangles.WedgeIndices.Num());
					}

					for (int32 FaceIdx = 0; FaceIdx < NumFaces; ++FaceIdx)
					{
						// Assume triangles for now
						for (int32 CornerIndex = 0; CornerIndex < 3; CornerIndex++)
						{
							int32 WedgeIndex = FaceIdx * 3 + CornerIndex;

							// Normals
							if (GeomData.Normals.size() > 0)
							{
								FUsdVectorData Normal = GeomData.Normals[WedgeIndex];
								//FVector TransformedNormal = ConversionMatrixIT.TransformVector(PrimToWorldIT.TransformVector(FVector(Normal.X, Normal.Y, Normal.Z)));
								FVector TransformedNormal = FinalTransformIT.TransformVector(FVector(Normal.X, Normal.Y, Normal.Z));

								RawTriangles.WedgeTangentZ.Add(TransformedNormal.GetSafeNormal());
							}


							// UVs
							if (GeomData.NumUVs > 0)
							{
								for (int32 UVIndex = 0; UVIndex < GeomData.NumUVs; ++UVIndex)
								{
									TArray<FVector2D>& TexCoords = RawTriangles.WedgeTexCoords[UVIndex];

									EUsdInterpolationMethod UVInterpMethod = GeomData.UVs[UVIndex].UVInterpMethod;

									// Get the index into the point array for this wedge
									int32 PointIndex = UVInterpMethod == EUsdInterpolationMethod::FaceVarying ? WedgeIndex : RawTriangles.WedgeIndices[WedgeIndex];

									// In this mode there is a single vertex per vertex so 
									// the point index should match up
									FUsdVector2Data UV = GeomData.UVs[UVIndex].Coords[PointIndex];

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
					for (int32 FaceIdx = 0; FaceIdx < NumFaces; ++FaceIdx)
					{
						int32 I0 = FaceIdx * 3 + 0;
						int32 I2 = FaceIdx * 3 + 2;
						Swap(RawTriangles.WedgeIndices[I0], RawTriangles.WedgeIndices[I2]);

						if (RawTriangles.WedgeTangentZ.Num())
						{
							Swap(RawTriangles.WedgeTangentZ[I0], RawTriangles.WedgeTangentZ[I2]);
						}

						for (int32 TexCoordIdx = 0; TexCoordIdx < MAX_MESH_TEXTURE_COORDS; ++TexCoordIdx)
						{
							TArray<FVector2D>& TexCoords = RawTriangles.WedgeTexCoords[TexCoordIdx];
							if (TexCoords.Num() > 0)
							{
								Swap(TexCoords[I0], TexCoords[I2]);
							}
						}
					}
				}
			}

			// @todo LOD support
			if (!ImportedMesh->SourceModels.IsValidIndex(LODIndex))
			{
				// Add one LOD 
				ImportedMesh->SourceModels.AddDefaulted();
			}

			FStaticMeshSourceModel& SrcModel = ImportedMesh->SourceModels[LODIndex];

			RawTriangles.CompactMaterialIndices();

			SrcModel.RawMeshBulkData->SaveRawMesh(RawTriangles);

			SrcModel.BuildSettings.bRecomputeNormals = true;
			SrcModel.BuildSettings.bRecomputeTangents = true;
			// Use mikktSpace if we have normals
			SrcModel.BuildSettings.bUseMikkTSpace = RawTriangles.WedgeTangentZ.Num() != 0;
			SrcModel.BuildSettings.bGenerateLightmapUVs = true;
			SrcModel.BuildSettings.bBuildAdjacencyBuffer = false;
			SrcModel.BuildSettings.bBuildReversedIndexBuffer = false;

			ImportedMesh->StaticMaterials.Empty();

			// There must always be one material
			int32 NumMaterials = FMath::Max<int32>(1, GeomData.MaterialNames.size());

			FString BasePackageName = FPackageName::GetLongPackagePath(ImportedMesh->GetOutermost()->GetName());

			// Add a material slot for each material
			for (int32 MaterialIdx = 0; MaterialIdx < NumMaterials; ++MaterialIdx)
			{
				UMaterialInterface* ExistingMaterial = nullptr;

				if(GeomData.MaterialNames.size() > MaterialIdx)
				{
					FString MaterialName = USDToUnreal::ConvertString(GeomData.MaterialNames[MaterialIdx]);

					FText Error;
					BasePackageName /= MaterialName;

					FString MaterialPath = MaterialName;

					ExistingMaterial = UMaterialImportHelpers::FindExistingMaterialFromSearchLocation(MaterialPath, BasePackageName, ImportContext.ImportOptions->MaterialSearchLocation, Error);

					if (!Error.IsEmpty())
					{
						ImportContext.AddErrorMessage(EMessageSeverity::Error, Error);
					}
				}

				ImportedMesh->StaticMaterials.Add(ExistingMaterial ? ExistingMaterial : UMaterial::GetDefaultMaterial(MD_Surface));
				ImportedMesh->SectionInfoMap.Set(LODIndex, MaterialIdx, FMeshSectionInfo(MaterialIdx));
			}


			ImportedMesh->ImportVersion = EImportStaticMeshVersion::BeforeImportStaticMeshVersionWasAdded;

			ImportedMesh->CreateBodySetup();

			ImportedMesh->SetLightingGuid();

			ImportedMesh->PostEditChange();
		}
		else
		{
			ImportContext.AddErrorMessage(EMessageSeverity::Error, 
				FText::Format(LOCTEXT("StaticMeshesMustBeTriangulated", "{0} is not a triangle mesh. Static meshes must be triangulated to import"), FText::FromString(ImportContext.ObjectName)));
		}
	}

	return ImportedMesh;
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
