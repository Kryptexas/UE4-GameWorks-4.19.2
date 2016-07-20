// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AlembicLibraryPublicPCH.h"

#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcCoreFactory/All.h>
#include <Alembic/AbcCoreAbstract/TimeSampling.h>
#include <Alembic/AbcCoreHDF5/All.h>

#include "AbcImporter.h"

#include "AbcImportData.h"

#include "PackageTools.h"
#include "RawMesh.h"
#include "ObjectTools.h"

#include "Engine/SkeletalMesh.h"
#include "SkelImport.h"
#include "Animation/AnimSequence.h"

#include "AbcImportUtilities.h"
#include "Runnables/AbcMeshDataImportRunnable.h"
#include "../Utils.h"

#include "MeshUtilities.h"
#include "MaterialUtilities.h"

#include "Runtime/Engine/Classes/Materials/MaterialInterface.h"
#include "Runtime/Engine/Public/MaterialCompiler.h"

#include "ParallelFor.h"

#include "EigenHelper.h"

#define LOCTEXT_NAMESPACE "AbcImporter"

DEFINE_LOG_CATEGORY_STATIC(LogAbcImporter, Verbose, All);

#define OBJECT_TYPE_SWITCH(a, b, c) if (AbcImporterUtilities::IsType<a>(ObjectMetaData)) { \
	a TypedObject = a(b, Alembic::Abc::kWrapExisting); \
	ParseAbcObject<a>(TypedObject, c); }

FAbcImporter::FAbcImporter()
	: ImportData(nullptr)
{

}

FAbcImporter::~FAbcImporter()
{
	if (ImportData)
	{
		delete ImportData;
		ImportData = nullptr;
	}
}

const EAbcImportError FAbcImporter::OpenAbcFileForImport(const FString InFilePath)
{
	// Init factory
	Alembic::AbcCoreFactory::IFactory Factory;
	Factory.setPolicy(Alembic::Abc::ErrorHandler::kThrowPolicy);
	Factory.setOgawaNumStreams(12);
	
	// Convert FString to const char*
	const char* CharFilePath = TCHAR_TO_ANSI(*InFilePath);

	// Extract Archive and compression type from file
	Alembic::AbcCoreFactory::IFactory::CoreType CompressionType;
	Alembic::Abc::IArchive Archive = Factory.getArchive(CharFilePath, CompressionType);
	if (!Archive.valid())
	{
		return AbcImportError_InvalidArchive;
	}

	// Get Top/root object
	Alembic::Abc::IObject TopObject(Archive, Alembic::Abc::kTop);
	if (!TopObject.valid())
	{
		return AbcImportError_NoValidTopObject;
	}

	if (ImportData)
	{
		delete ImportData;
		ImportData = nullptr;
	}

	ImportData = new FAbcImportData();

	TArray<FAbcTransformObject*> AbcHierarchy;
	FGuid ZeroGuid;
	TraverseAbcHierarchy(TopObject, AbcHierarchy, ZeroGuid);

	if (ImportData->PolyMeshObjects.Num() == 0)
	{
		return AbcImportError_NoMeshes;
	}

	ImportData->FilePath = InFilePath;
	ImportData->NumTotalMaterials = 0;


	return AbcImportError_NoError;
}

const EAbcImportError FAbcImporter::ImportTrackData(const int32 InNumThreads, UAbcImportSettings* ImportSettings)
{
	SCOPE_LOG_TIME(TEXT("Alembic_ReadTrackData"), nullptr);
	TArray<FAbcMeshDataImportRunnable*> MeshImportRunnables;

	ImportData->ImportSettings = ImportSettings;

	const int32 NumMeshTracks = ImportData->PolyMeshObjects.Num();	
	FAbcSamplingSettings& SamplingSettings = ImportSettings->SamplingSettings;
	// When importing static meshes optimize frame import span
	if (ImportSettings->ImportType == EAlembicImportType::StaticMesh)
	{
		SamplingSettings.FrameEnd = SamplingSettings.FrameStart + 1;
	}
	
	// This will remove all poly meshes that are set not to be imported in the settings UI
	ImportData->PolyMeshObjects.RemoveAll([=](const TSharedPtr<FAbcPolyMeshObject>& Object) {return !Object->bShouldImport; });

	// Reset objects being constant if importing as skeletal mesh and baking matrix animaiton
	if (ImportSettings->ImportType == EAlembicImportType::Skeletal && ImportSettings->CompressionSettings.bBakeMatrixAnimation)
	{
		for (TSharedPtr<FAbcPolyMeshObject>& MeshObject : ImportData->PolyMeshObjects)
		{
			MeshObject->bConstant = false;
		}
	}

	// Creates materials according to the face set names that were found in the Alembic file
	if (ImportSettings->MaterialSettings.bCreateMaterials)
	{
		for (TSharedPtr<FAbcPolyMeshObject>& MeshObject : ImportData->PolyMeshObjects)
		{
			for (const FString& FaceSetName : MeshObject->FaceSetNames)
			{
				// Preventing duplicate material creation
				UMaterial** ExistingMaterial = ImportData->MaterialMap.Find(*FaceSetName);
				if (!ExistingMaterial)
				{ 
					UMaterial* Material = NewObject<UMaterial>((UObject*)GetTransientPackage(), *FaceSetName);
					Material->bUsedWithMorphTargets = true;
					ImportData->MaterialMap.Add(FaceSetName, Material);
				}
			}
		}
	}	

	// Determining sampling time/types and when to start and stop sampling
	uint32 StartFrameIndex = SamplingSettings.FrameStart;
	uint32 EndFrameIndex = SamplingSettings.FrameEnd;
	int32 FrameSpan = EndFrameIndex - StartFrameIndex;
	float CacheLength = ImportData->MaxTime - ImportData->MinTime;

	float TimeStep = 0.0f;
	EAlembicSamplingType SamplingType = SamplingSettings.SamplingType;
	switch (SamplingType)
	{
	case EAlembicSamplingType::PerFrame:
		{
			// Calculates the time step required to get the number of frames
			TimeStep = CacheLength / (float)ImportData->NumFrames;
			break;
		}
	
	case EAlembicSamplingType::PerTimeStep:
		{
			// Calculates the original time step and the ratio between it and the user specified time step
			const float OriginalTimeStep = CacheLength / (float)ImportData->NumFrames;
			const float FrameStepRatio = OriginalTimeStep / SamplingSettings.TimeSteps;
			TimeStep = SamplingSettings.TimeSteps;

			AbcImporterUtilities::CalculateNewStartAndEndFrameIndices(FrameStepRatio, StartFrameIndex, EndFrameIndex);
			FrameSpan = EndFrameIndex - StartFrameIndex;		
			break;
		}
	case EAlembicSamplingType::PerXFrames:
		{
			// Calculates the original time step and the ratio between it and the user specified time step
			const float OriginalTimeStep = CacheLength / (float)ImportData->NumFrames;
			const float FrameStepRatio = OriginalTimeStep / ((float)SamplingSettings.FrameSteps * OriginalTimeStep);
			TimeStep = ((float)SamplingSettings.FrameSteps * OriginalTimeStep);			

			AbcImporterUtilities::CalculateNewStartAndEndFrameIndices(FrameStepRatio, StartFrameIndex, EndFrameIndex);
			FrameSpan = EndFrameIndex - StartFrameIndex;
			break;
		}

	default:
		checkf(false, TEXT("Incorrect sampling type found in import settings (%i)"), (uint8)SamplingType);
	}

	// Allocating the number of meshsamples we will import for each object
	for (TSharedPtr<FAbcPolyMeshObject>& MeshObject : ImportData->PolyMeshObjects)
	{
		MeshObject->MeshSamples.AddZeroed(FrameSpan);
	}

	// Initializing and running the importing threads 
	const uint32 NumThreads = InNumThreads;

	// At least 4 frames are required in order for use to spin off multiple threads to import the data
	const uint32 MinimumNumberOfSamplesForSpinningOfThreads = 4;
	const uint32 Steps = (FrameSpan <= MinimumNumberOfSamplesForSpinningOfThreads) ? FrameSpan : FMath::CeilToInt((float)FrameSpan / (float)NumThreads);

	uint32 StartingFrameIndex = StartFrameIndex;
	while (StartingFrameIndex < EndFrameIndex)
	{
		FAbcMeshDataImportRunnable* TestRunable = new FAbcMeshDataImportRunnable(ImportData, StartingFrameIndex, FMath::Min(StartingFrameIndex + Steps, EndFrameIndex), TimeStep);
		StartingFrameIndex += Steps;
		MeshImportRunnables.Push(TestRunable);
	}
	
	bool bImportSuccesful = true;

	// All Mesh data is imported from the Alembic format after the runnables have finished
	for (FAbcMeshDataImportRunnable* Runnable : MeshImportRunnables)
	{
		Runnable->Wait();
		bImportSuccesful &= Runnable->WasSuccesful();
	}

	if (!bImportSuccesful)
	{
		return AbcImportError_FailedToImportData;
	}
		
	// Processing the mesh objects in order to calculate their normals/tangents
	for (TSharedPtr<FAbcPolyMeshObject>& MeshObject : ImportData->PolyMeshObjects)
	{
		const bool bFramesAvailable = MeshObject->MeshSamples.Num() > 0;
		check(bFramesAvailable);

		// We determine whether or not the mesh contains constant topology to know if it can be PCA compressed
		int32 VertexCount = bFramesAvailable ? MeshObject->MeshSamples[0]->Vertices.Num() : 0;
		int32 IndexCount = bFramesAvailable ? MeshObject->MeshSamples[0]->Indices.Num() : 0;
		MeshObject->bConstantTopology = true;
		for (FAbcMeshSample* Sample : MeshObject->MeshSamples)
		{
			if (Sample && (VertexCount != Sample->Vertices.Num() || IndexCount != Sample->Indices.Num()))
			{
				MeshObject->bConstantTopology = false;
				break;
			}
		}
		
		// Normal availability determination and calculating what's needed/missing
		const bool bNormalsAvailable = MeshObject->MeshSamples[0]->Normals.Num() > 0 && !ImportData->ImportSettings->NormalGenerationSettings.bRecomputeNormals;
		const bool bFullFrameNormalsAvailable = (!MeshObject->bConstant && MeshObject->MeshSamples.Num() > 1) && (MeshObject->MeshSamples[1]->Normals.Num() > 0);
		const bool bCalculateSmoothingGroups = !ImportData->ImportSettings->NormalGenerationSettings.bForceOneSmoothingGroupPerObject;		
		if (!bNormalsAvailable || !bFullFrameNormalsAvailable)
		{
			// Require calculating Normals, no normals available whatsoever
			if (!bNormalsAvailable)
			{
				// Function bodies for regular and smooth normals to prevent branch within loop
				const TFunctionRef<void(int32)> RegularNormalsFunction
					= [&](int32 Index)
				{
					FAbcMeshSample* MeshSample = MeshObject->MeshSamples[Index];
					if (MeshSample)
					{
						AbcImporterUtilities::CalculateNormals(MeshSample);

						// Setup smoothing masks to 0
						MeshSample->SmoothingGroupIndices.Empty(MeshSample->Indices.Num() / 3);
						MeshSample->SmoothingGroupIndices.AddZeroed(MeshSample->Indices.Num() / 3);
						MeshSample->NumSmoothingGroups = 1;
					}
				};

				const TFunctionRef<void(int32)> SmoothNormalsFunction
					= [&](int32 Index)
				{
					FAbcMeshSample* MeshSample = MeshObject->MeshSamples[Index];

					if (MeshSample)
					{
						AbcImporterUtilities::CalculateSmoothNormals(MeshSample);

						// Setup smoothing masks to 0
						MeshSample->SmoothingGroupIndices.Empty(MeshSample->Indices.Num() / 3);
						MeshSample->SmoothingGroupIndices.AddZeroed(MeshSample->Indices.Num() / 3);
						MeshSample->NumSmoothingGroups = 1;
					}
				};

				ParallelFor(MeshObject->MeshSamples.Num(), bCalculateSmoothingGroups ? RegularNormalsFunction : SmoothNormalsFunction);
			}
			else
			{
				// Just normals for first frame, and we have the extracted smoothing groups
				ParallelFor(MeshObject->MeshSamples.Num() - 1,
					[&](int32 Index)
				{
					FAbcMeshSample* MeshSample = MeshObject->MeshSamples[Index + 1];
					if (MeshSample)
					{
						AbcImporterUtilities::CalculateNormalsWithSmoothingGroups(MeshSample, MeshObject->MeshSamples[0]->SmoothingGroupIndices, MeshObject->MeshSamples[0]->NumSmoothingGroups);

						// Copy smoothing masks from frame 0
						MeshSample->SmoothingGroupIndices = MeshObject->MeshSamples[0]->SmoothingGroupIndices;
					}
				});
			}
		}	

		// Module manager is not thread safe, so need to prefetch before parallelfor
		IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");

		// Since we have normals and UVs now calculate tangents
		ParallelFor(MeshObject->MeshSamples.Num(),
			[&](int32 Index)
		{
			FAbcMeshSample* MeshSample = MeshObject->MeshSamples[Index];
			
			if (MeshSample)
			{
				AbcImporterUtilities::ComputeTangents(MeshSample, ImportSettings, MeshUtilities);
			}
		});

		if (bFramesAvailable)
		{
			ImportData->NumTotalMaterials += MeshObject->MeshSamples[0]->NumMaterials;
		}
	}

	// Reading the required transform tracks
	for (FAbcTransformObject& TransformObject : ImportData->TransformObjects)
	{
		Alembic::AbcGeom::IXform Transform = TransformObject.Transform;
		TransformObject.MatrixSamples.SetNumZeroed(TransformObject.NumSamples);
		TransformObject.TimeSamples.SetNumZeroed(TransformObject.NumSamples);

		ParallelFor(TransformObject.NumSamples,
			[&](int32 Index)
		{
			// Get schema from parent object
			Alembic::AbcGeom::XformSample sample;
			Alembic::AbcGeom::IXformSchema Schema = Transform.getSchema();
			Schema.get(sample, Index);

			// Get matrix and concatenate
			Alembic::Abc::M44d Matrix = sample.getMatrix();
			TransformObject.MatrixSamples[Index] = AbcImporterUtilities::ConvertAlembicMatrix(Matrix);
			
			// Get TimeSampler for this sample's time
			Alembic::Abc::TimeSamplingPtr TimeSampler = Schema.getTimeSampling();
			TransformObject.TimeSamples[Index] = (float)TimeSampler->getSampleTime(Index);
		});
	}

	// Simple duplicate frame removal (only needs to be done if we're importing the data as a geometry cache asset
	if (ImportData->ImportSettings->ImportType == EAlembicImportType::GeometryCache)
	{
		ParallelFor(ImportData->PolyMeshObjects.Num(), [&](int32 MeshObjectIndex)
		{
			TSharedPtr<FAbcPolyMeshObject>& MeshObject = ImportData->PolyMeshObjects[MeshObjectIndex];
			if (!MeshObject->bConstant)
			{
				TMap<uint32, uint32> IdenticalPositions;

				for (int32 SampleIndex = 0; SampleIndex < MeshObject->MeshSamples.Num() - 1; ++SampleIndex)
				{
					FAbcMeshSample* Sample = MeshObject->MeshSamples[SampleIndex];
					FAbcMeshSample* NextSample = MeshObject->MeshSamples[SampleIndex + 1];
					int32 Result = FMemory::Memcmp(&Sample->Vertices[0], &NextSample->Vertices[0], Sample->Vertices.GetTypeSize() * Sample->Vertices.Num());
					if (Result == 0)
					{
						IdenticalPositions.Add(SampleIndex, SampleIndex + 1);
					}
				}

				for (TPair<uint32, uint32> Pair : IdenticalPositions)
				{
					uint32 FrameIndex = Pair.Value;
					delete MeshObject->MeshSamples[FrameIndex];
					MeshObject->MeshSamples[FrameIndex] = nullptr;
				}

				MeshObject->MeshSamples.RemoveAll([&](FAbcMeshSample* In) { return In == nullptr; });
				MeshObject->NumSamples = MeshObject->MeshSamples.Num();
			}
		});
	}
	
	return AbcImportError_NoError;
}

void FAbcImporter::TraverseAbcHierarchy(const Alembic::Abc::IObject& InObject, TArray<FAbcTransformObject*>& InObjectHierarchy, FGuid InGuid)
{
	// Get Header and MetaData info from current Alembic Object
	Alembic::AbcCoreAbstract::ObjectHeader Header = InObject.getHeader();	
	const Alembic::Abc::MetaData ObjectMetaData = InObject.getMetaData();
	const uint32 NumChildren = InObject.getNumChildren();

	if (InObjectHierarchy.Num())
	{
		ImportData->Hierarchies.Add(InGuid, InObjectHierarchy);
	}
	
	OBJECT_TYPE_SWITCH(Alembic::AbcGeom::IPolyMesh, InObject, InGuid);
	OBJECT_TYPE_SWITCH(Alembic::AbcGeom::IXform, InObject, InGuid);

	// Recursive traversal of child objects
	if (NumChildren > 0)
	{
		// Push back this object for the Hierarchy
		TArray<FAbcTransformObject*> NewObjectHierarchy = InObjectHierarchy;
		NewObjectHierarchy = InObjectHierarchy;

		if (AbcImporterUtilities::IsType<Alembic::AbcGeom::IXform>(ObjectMetaData))
		{
			NewObjectHierarchy.Add(&ImportData->TransformObjects.Last());
		}

		FGuid ChildGuid = (NewObjectHierarchy.Num() != InObjectHierarchy.Num() ) ? FGuid::NewGuid() : InGuid;

		for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
		{
			const Alembic::Abc::IObject& AbcChildObject = InObject.getChild(ChildIndex);
			TraverseAbcHierarchy(AbcChildObject, NewObjectHierarchy, ChildGuid);
		}
	}
}

template<>
void FAbcImporter::ParseAbcObject<Alembic::AbcGeom::IXform>(Alembic::AbcGeom::IXform& InXform, FGuid InHierarchyGuid)
{
	// Do something	
	FAbcTransformObject TransformObject;
	TransformObject.HierarchyGuid = InHierarchyGuid;	
	TransformObject.Transform = InXform;
	TransformObject.Name = FString(InXform.getName().c_str());

	// Retrieve schema and frame information
	Alembic::AbcGeom::IXformSchema Schema = InXform.getSchema();
	TransformObject.NumSamples = Schema.getNumSamples();

	float MinTime, MaxTime;
	AbcImporterUtilities::GetMinAndMaxTime(InXform.getSchema(), MinTime, MaxTime);
	ImportData->MinTime = FMath::Min(ImportData->MinTime, MinTime);
	ImportData->MaxTime = FMath::Max(ImportData->MaxTime, MaxTime);
	ImportData->NumFrames = FMath::Max(ImportData->NumFrames, (uint32)InXform.getSchema().getNumSamples());

	ImportData->TransformObjects.Push(TransformObject);
}

template<>
void FAbcImporter::ParseAbcObject<Alembic::AbcGeom::IPolyMesh>(Alembic::AbcGeom::IPolyMesh& InPolyMesh, FGuid InHierarchyGuid)
{
	// Do something
	TSharedPtr<FAbcPolyMeshObject> PolyMeshObject = TSharedPtr<FAbcPolyMeshObject>( new FAbcPolyMeshObject());
	PolyMeshObject->Mesh = InPolyMesh;
	PolyMeshObject->Name = FString(InPolyMesh.getName().c_str());
	PolyMeshObject->bShouldImport = true;

	// Retrieve schema and frame information
	Alembic::AbcGeom::IPolyMeshSchema Schema = InPolyMesh.getSchema();
	PolyMeshObject->NumSamples = Schema.getNumSamples();
	PolyMeshObject->bConstant = Schema.isConstant();
	
	PolyMeshObject->HierarchyGuid = InHierarchyGuid;

	AbcImporterUtilities::RetrieveFaceSetNames(Schema, PolyMeshObject->FaceSetNames);

	float MinTime, MaxTime;
	AbcImporterUtilities::GetMinAndMaxTime(InPolyMesh.getSchema(), MinTime, MaxTime);
	ImportData->MinTime = FMath::Min(ImportData->MinTime, MinTime);
	ImportData->MaxTime = FMath::Max(ImportData->MaxTime, MaxTime);
	ImportData->NumFrames = FMath::Max(ImportData->NumFrames, PolyMeshObject->NumSamples);
	
	ImportData->PolyMeshObjects.Push(PolyMeshObject);
}

template<typename T>
T* FAbcImporter::CreateObjectInstance(UObject* InParent, const FString& ObjectName, const EObjectFlags Flags)
{
	// Parent package to place new mesh
	UPackage* Package = nullptr;
	FString NewPackageName;

	// Setup package name and create one accordingly
	NewPackageName = FPackageName::GetLongPackagePath(InParent->GetOutermost()->GetName() + TEXT("/") + ObjectName);
	NewPackageName = PackageTools::SanitizePackageName(NewPackageName);
	Package = CreatePackage(nullptr, *NewPackageName);

	T* ExistingTypedObject = FindObject<T>(Package, *ObjectName);
	UObject* ExistingObject = FindObject<UObject>(Package, *ObjectName);

	if (ExistingTypedObject != nullptr)
	{
		ExistingTypedObject->PreEditChange(nullptr);
	}
	else if (ExistingObject != nullptr)
	{
		// Replacing an object.  Here we go!
		// Delete the existing object
		bool bDeleteSucceeded = ObjectTools::DeleteSingleObject(ExistingObject);

		if (bDeleteSucceeded)
		{
			// Force GC so we can cleanly create a new asset (and not do an 'in place' replacement)
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

			// Create a package for each mesh
			Package = CreatePackage(nullptr, *NewPackageName);
		}
		else
		{
			// failed to delete
			return nullptr;
		}
	}

	if (ExistingTypedObject != nullptr)
	{
		return ExistingTypedObject;
	}
	else
	{
		return NewObject<T>(Package, FName(*ObjectName), Flags | RF_Public);
	}
}

UStaticMesh* FAbcImporter::ImportSingleAsStaticMesh(const int32 MeshTrackIndex, UObject* InParent, EObjectFlags Flags)
{
	// Get Mesh object from array
	check(MeshTrackIndex >= 0 && MeshTrackIndex < ImportData->PolyMeshObjects.Num() && "Incorrect Mesh index");

	// Populate raw mesh from sample
	const uint32 FrameIndex = 0;
	FRawMesh RawMesh;
	GenerateRawMeshFromSample(MeshTrackIndex, FrameIndex, RawMesh);
	
	// Setup static mesh instance
	UStaticMesh* StaticMesh = CreateStaticMeshFromRawMesh(InParent, ImportData->PolyMeshObjects[MeshTrackIndex]->Name, Flags,ImportData->PolyMeshObjects[MeshTrackIndex]->MeshSamples[0]->NumMaterials, ImportData->PolyMeshObjects[MeshTrackIndex]->FaceSetNames, RawMesh);
	
	// Return the freshly created static mesh
	return StaticMesh;
}

UStaticMesh* FAbcImporter::CreateStaticMeshFromRawMesh(UObject* InParent, const FString& Name, EObjectFlags Flags, const uint32 NumMaterials, const TArray<FString>& FaceSetNames, FRawMesh& RawMesh)
{
	UStaticMesh* StaticMesh = CreateObjectInstance<UStaticMesh>(InParent, Name, Flags);

	// Add the first LOD, we only support one
	new(StaticMesh->SourceModels) FStaticMeshSourceModel();

	// Generate a new lighting GUID (so its unique)
	StaticMesh->LightingGuid = FGuid::NewGuid();

	// Set it to use textured lightmaps. Note that Build Lighting will do the error-checking (texcoord index exists for all LODs, etc).
	StaticMesh->LightMapResolution = 64;
	StaticMesh->LightMapCoordinateIndex = 1;

	// Material setup, since there isn't much material information in the Alembic file, 
	UMaterial* DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
	check(DefaultMaterial);

	// Material list
	StaticMesh->Materials.Empty();
	// If there were FaceSets available in the Alembic file use the number of unique face sets as num material entries, otherwise default to one material for the whole mesh
	const uint32 FrameIndex = 0;
	uint32 NumFaceSets = FaceSetNames.Num();

	const bool bCreateMaterial = ImportData->ImportSettings->MaterialSettings.bCreateMaterials;
	for (uint32 MaterialIndex = 0; MaterialIndex < ((NumMaterials != 0 ) ? NumMaterials : 1); ++MaterialIndex)
	{
		UMaterial* Material = nullptr;
		if (FaceSetNames.IsValidIndex(MaterialIndex))
		{			
			Material = RetrieveMaterial(FaceSetNames[MaterialIndex], InParent, Flags);
		}		
				
		StaticMesh->Materials.Add(( Material != nullptr) ? Material : DefaultMaterial);
	}

	// Get the first LOD for filling it up with geometry, only support one LOD
	FStaticMeshSourceModel& SrcModel = StaticMesh->SourceModels[0];
	// Set build settings for the static mesh
	SrcModel.BuildSettings.bRecomputeNormals = false;
	SrcModel.BuildSettings.bRecomputeTangents = false;
	SrcModel.BuildSettings.bUseMikkTSpace = false;
	// Generate Lightmaps uvs (no support for importing right now)
	SrcModel.BuildSettings.bGenerateLightmapUVs = true;
	// Set lightmap UV index to 1 since we currently only import one set of UVs from the Alembic Data file
	SrcModel.BuildSettings.DstLightmapIndex = 1;

	// Build the static mesh (using the build setting etc.) this generates correct tangents using the extracting smoothing group along with the imported Normals data
	StaticMesh->Build(false);

	// Store the raw mesh within the RawMeshBulkData
	SrcModel.RawMeshBulkData->SaveRawMesh(RawMesh);

	// No collision generation for now
	StaticMesh->CreateBodySetup();

	return StaticMesh;
}

const TArray<UStaticMesh*> FAbcImporter::ImportAsStaticMesh(UObject* InParent, EObjectFlags Flags)
{
	checkf(ImportData->PolyMeshObjects.Num() > 0, TEXT("No poly meshes found"));

	TArray<UStaticMesh*> StaticMeshes;
	
	const FAbcStaticMeshSettings& Settings = ImportData->ImportSettings->StaticMeshSettings;

	// Check if the user want the meshes separate or merged
	if (Settings.bMergeMeshes)
	{
		// If merging we merge all the raw mesh structures together and generate a static mesh asset from this
		TArray<FString> MergedFaceSetNames;
		TArray<FAbcMeshSample*> Samples;
		uint32 TotalNumMaterials = 0;
		for (TSharedPtr<FAbcPolyMeshObject> Mesh : ImportData->PolyMeshObjects)
		{
			if (Mesh->bShouldImport)
			{
				checkf(Mesh->MeshSamples.Num(), TEXT("No mesh samples found"));

				FAbcMeshSample* Sample = Mesh->MeshSamples[0];				
				TotalNumMaterials += (Sample->NumMaterials != 0) ? Sample->NumMaterials : 1;

				Samples.Add(Sample);
				if (Mesh->FaceSetNames.Num() > 0)
				{
					MergedFaceSetNames.Append(Mesh->FaceSetNames);
				}
				else
				{
					// Default name
					static const FString DefaultName("NoFaceSetName");
					MergedFaceSetNames.Add(DefaultName);
				}				
			}
		}
		
		FAbcMeshSample* MergedSample = nullptr;
		MergedSample = AbcImporterUtilities::MergeMeshSamples(Samples);
		FRawMesh RawMesh;
		GenerateRawMeshFromSample(MergedSample, RawMesh);
		
		StaticMeshes.Add(CreateStaticMeshFromRawMesh(InParent, FPaths::GetBaseFilename(ImportData->FilePath), Flags,
			TotalNumMaterials, MergedFaceSetNames, RawMesh));
	}
	else
	{
		uint32 MeshIndex = 0;
		for (TSharedPtr<FAbcPolyMeshObject> Mesh : ImportData->PolyMeshObjects)
		{
			if (Mesh->bShouldImport)
			{
				StaticMeshes.Add(ImportSingleAsStaticMesh(MeshIndex, InParent, Flags));
			}
			++MeshIndex;
		}
	}


	return StaticMeshes;
}

UGeometryCache* FAbcImporter::ImportAsGeometryCache(UObject* InParent, EObjectFlags Flags)
{
	// Create a GeometryCache instance 
	UGeometryCache* GeometryCache = CreateObjectInstance<UGeometryCache>(InParent, FPaths::GetBaseFilename(ImportData->FilePath), Flags);

	// In case this is a reimport operation
	GeometryCache->ClearForReimporting();

	// Load the default material for later usage
	UMaterial* DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
	check(DefaultMaterial);

	uint32 MaterialOffset = 0;

	for (TSharedPtr<FAbcPolyMeshObject>& MeshObject : ImportData->PolyMeshObjects)
	{
		UGeometryCacheTrack* Track = nullptr;
		// Determine what kind of GeometryCacheTrack we must create
		if (MeshObject->bConstant)
		{
			// TransformAnimation
			Track = CreateTransformAnimationTrack(MeshObject->Name, MeshObject, GeometryCache, MaterialOffset);
		}
		else
		{
			// FlibookAnimation
			Track = CreateFlipbookAnimationTrack(MeshObject->Name, MeshObject, GeometryCache, MaterialOffset);
		}

		if (Track == nullptr)
		{
			// Import was cancelled
			delete GeometryCache;
			return nullptr;
		}

		// Add materials for this Mesh Object
		const uint32 NumMaterials = (MeshObject->FaceSetNames.Num() > 0) ? MeshObject->FaceSetNames.Num() : 1;
		for (uint32 MaterialIndex = 0; MaterialIndex < NumMaterials; ++MaterialIndex)
		{
			UMaterial* Material = nullptr;
			if (MeshObject->FaceSetNames.IsValidIndex(MaterialIndex))
			{
				Material = RetrieveMaterial(MeshObject->FaceSetNames[MaterialIndex], InParent, Flags);
			}

			GeometryCache->Materials.Add((Material != nullptr) ? Material : DefaultMaterial);
		}
		MaterialOffset += NumMaterials;

		// Get Matrix samples 
		TArray<FMatrix> Matrices;
		TArray<float> SampleTimes;
		GetMatrixSamplesForObject(MeshObject->Mesh, Matrices, SampleTimes);
		// Store samples inside the track
		Track->SetMatrixSamples(Matrices, SampleTimes);

		// Update Total material count
		ImportData->NumTotalMaterials += Track->GetNumMaterials();

		check(Track != nullptr && "Invalid track data");
		GeometryCache->AddTrack(Track);
	}

	return GeometryCache;
}

USkeletalMesh* FAbcImporter::ImportAsSkeletalMesh(UObject* InParent, EObjectFlags Flags)
{
	// First compress the animation data
	CompressAnimationDataUsingPCA(ImportData->ImportSettings->CompressionSettings, true);

	// Enforce to compute normals and tangents for the average sample which forms the base of the skeletal mesh
	IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
	for (const FCompressedAbcData& CompressedData : ImportData->CompressedMeshData)
	{
		FAbcMeshSample* AverageSample = CompressedData.AverageSample;
		AbcImporterUtilities::CalculateNormalsWithSmoothingGroups(AverageSample, AverageSample->SmoothingGroupIndices, AverageSample->NumSmoothingGroups);
		AbcImporterUtilities::ComputeTangents(AverageSample, ImportData->ImportSettings, MeshUtilities);
	}

	// Create a Skeletal mesh instance 
	USkeletalMesh* SkeletalMesh = CreateObjectInstance<USkeletalMesh>(InParent, FPaths::GetBaseFilename(ImportData->FilePath), Flags);

	// Touch pre edit change
	SkeletalMesh->PreEditChange(NULL);

	// Retrieve the imported resource structure and allocate a new LOD model
	FSkeletalMeshResource* ImportedResource = SkeletalMesh->GetImportedResource();
	check(ImportedResource->LODModels.Num() == 0);
	ImportedResource->LODModels.Empty();
	new(ImportedResource->LODModels)FStaticLODModel();
	SkeletalMesh->LODInfo.Empty();
	SkeletalMesh->LODInfo.AddZeroed();

	
	FStaticLODModel& LODModel = ImportedResource->LODModels[0];	

	// Generate skeletal mesh data from 
	FSkeletalMeshImportData SkeletalMeshImportData;
	// TODO not use CopyLODImportData but populate arrays ourselves directly
	GenerateSkeletalMeshDataFromCompressedData(SkeletalMeshImportData, ImportData->CompressedMeshData);

	TArray<FVector> LODPoints;
	TArray<FMeshWedge> LODWedges;
	TArray<FMeshFace> LODFaces;
	TArray<FVertInfluence> LODInfluences;
	TArray<int32> LODPointToRawMap;
	SkeletalMeshImportData.CopyLODImportData(LODPoints, LODWedges, LODFaces, LODInfluences, LODPointToRawMap);

	const FMeshBoneInfo BoneInfo(FName(TEXT("RootBone"), FNAME_Add), TEXT("RootBone_Export"), INDEX_NONE);
	const FTransform BoneTransform;
	SkeletalMesh->RefSkeleton.Add(BoneInfo, BoneTransform);

	IMeshUtilities::MeshBuildOptions BuildOptions;
	BuildOptions.bKeepOverlappingVertices = true;
	BuildOptions.bComputeNormals = ImportData->ImportSettings->NormalGenerationSettings.bRecomputeNormals || !SkeletalMeshImportData.bHasNormals;
	BuildOptions.bComputeTangents = ImportData->ImportSettings->NormalGenerationSettings.bRecomputeNormals || !SkeletalMeshImportData.bHasTangents; 
	BuildOptions.bUseMikkTSpace = true;
	BuildOptions.bRemoveDegenerateTriangles = false;	

	// Forced to 1
	ImportedResource->LODModels[0].NumTexCoords = 1;
	SkeletalMesh->bHasVertexColors = false;
	FBox BoundingBox(LODPoints.GetData(), LODPoints.Num());
	/* TODO calculate bounding box according to animation */
	SkeletalMesh->SetImportedBounds(BoundingBox);

	TArray<FText> WarningMessages;
	TArray<FName> WarningNames;
	// Create actual rendering data.
	bool bBuildSuccess = MeshUtilities.BuildSkeletalMesh(ImportedResource->LODModels[0], SkeletalMesh->RefSkeleton, LODInfluences, LODWedges, LODFaces, LODPoints, LODPointToRawMap, BuildOptions, &WarningMessages, &WarningNames);

	if (!bBuildSuccess)
	{
		SkeletalMesh->MarkPendingKill();
		return NULL;
	}

	// Presize the per-section shadow casting array with the number of sections in the imported LOD.
	SkeletalMesh->LODInfo[0].TriangleSortSettings.AddZeroed(LODModel.Sections.Num());

	// Create the skeleton object
	FString SkeletonName = FString::Printf(TEXT("%s_Skeleton"), *SkeletalMesh->GetName());
	USkeleton* Skeleton = CreateObjectInstance<USkeleton>(InParent, SkeletonName, Flags);
	
	// Merge bones to the selected skeleton
	check(Skeleton->MergeAllBonesToBoneTree(SkeletalMesh));
	Skeleton->MarkPackageDirty();
	if (SkeletalMesh->Skeleton != Skeleton)
	{
		SkeletalMesh->Skeleton = Skeleton;
		SkeletalMesh->MarkPackageDirty();
	}

	// Create animation sequence for the skeleton
	UAnimSequence* Sequence = CreateObjectInstance<UAnimSequence>(InParent, FString::Printf(TEXT("%s_Animation"), *SkeletalMesh->GetName()), Flags);
	Sequence->SetSkeleton(Skeleton);
	Sequence->SequenceLength = ImportData->MaxTime - ImportData->MinTime;

	int32 ObjectIndex = 0;
	uint32 TriangleOffset = 0;
	uint32 WedgeOffset = 0;
	uint32 VertexOffset = 0;

	for (FCompressedAbcData& CompressedData : ImportData->CompressedMeshData)
	{
		FAbcMeshSample* AverageSample = CompressedData.AverageSample;

		if ( CompressedData.BaseSamples.Num() > 0 )
		{
			int32 NumBases = CompressedData.BaseSamples.Num();
			int32 NumUsedBases = 0;

			const int32 NumIndices = CompressedData.AverageSample->Indices.Num();
			
			for (int32 BaseIndex = 0; BaseIndex < NumBases; ++BaseIndex)
			{
				FAbcMeshSample* BaseSample = CompressedData.BaseSamples[BaseIndex];

				AbcImporterUtilities::CalculateNormalsWithSmoothingGroups(BaseSample, BaseSample->SmoothingGroupIndices, BaseSample->NumSmoothingGroups);				
				AbcImporterUtilities::ComputeTangents(BaseSample, ImportData->ImportSettings, MeshUtilities);
				
				// Create new morph target with name based on object and base index
				UMorphTarget* MorphTarget = NewObject<UMorphTarget>(SkeletalMesh, FName(*FString::Printf(TEXT("Base_%i_%i"), BaseIndex, ObjectIndex)));

				// Setup morph target vertices directly
				TArray<FMorphTargetDelta> MorphDeltas;
				GenerateMorphTargetVertices(BaseSample, MorphDeltas, AverageSample, WedgeOffset);								
				MorphTarget->PopulateDeltas(MorphDeltas, 0);

				const float PercentageOfVerticesInfluences = ((float)MorphTarget->MorphLODModels[0].Vertices.Num() / (float)NumIndices) * 100.0f;
				if (PercentageOfVerticesInfluences > ImportData->ImportSettings->CompressionSettings.MinimumNumberOfVertexInfluencePercentage)
				{
					SkeletalMesh->RegisterMorphTarget(MorphTarget);
					MorphTarget->MarkPackageDirty();
					
					// Set up curves
					const TArray<float>& CurveValues = CompressedData.CurveValues[BaseIndex];
					const TArray<float>& TimeValues = CompressedData.TimeValues[BaseIndex];
					// Morph target stuffies
					FString CurveName = FString::Printf(TEXT("Base_%i_%i"), BaseIndex, ObjectIndex);
					FName ConstCurveName = *CurveName;

					// Sets up the morph target curves with the sample values and time keys
					SetupMorphTargetCurves(Skeleton, ConstCurveName, Sequence, CurveValues, TimeValues);
				}
				else
				{
					MorphTarget->MarkPendingKill();
				}
			}
		}

		Sequence->RawCurveData.RemoveRedundantKeys();

		WedgeOffset += CompressedData.AverageSample->Indices.Num();
		VertexOffset += CompressedData.AverageSample->Vertices.Num();

		const uint32 NumMaterials = CompressedData.MaterialNames.Num();
		for (uint32 MaterialIndex = 0; MaterialIndex < NumMaterials; ++MaterialIndex)
		{			
			const FString& MaterialName = CompressedData.MaterialNames[MaterialIndex];
			UMaterial* Material = RetrieveMaterial(MaterialName, InParent, Flags);
			SkeletalMesh->Materials.Add(FSkeletalMaterial(Material, true));
		}

		++ObjectIndex;
	}

	SkeletalMesh->CalculateInvRefMatrices();
	SkeletalMesh->PostEditChange();
	SkeletalMesh->MarkPackageDirty();
	
	// Retrieve the name mapping container
	const FSmartNameMapping* NameMapping = Skeleton->GetSmartNameContainer(USkeleton::AnimCurveMappingName);
	Sequence->RawCurveData.RefreshName(NameMapping);
	Sequence->MarkRawDataAsModified();
	Sequence->MarkPackageDirty();
	
	return SkeletalMesh;
}

void FAbcImporter::SetupMorphTargetCurves(USkeleton* Skeleton, FName ConstCurveName, UAnimSequence* Sequence, const TArray<float> &CurveValues, const TArray<float>& TimeValues)
{
	FSmartName NewName;
	Skeleton->AddSmartNameAndModify(USkeleton::AnimCurveMappingName, ConstCurveName, NewName);

	check(Sequence->RawCurveData.AddCurveData(NewName, ACF_MorphTargetCurve));
	FFloatCurve * NewCurve = static_cast<FFloatCurve *> (Sequence->RawCurveData.GetCurveData(NewName.UID, FRawCurveTracks::FloatType));

	for (int32 KeyIndex = 0; KeyIndex < CurveValues.Num(); ++KeyIndex)
	{
		const float& CurveValue = CurveValues[KeyIndex];
		const float& TimeValue = TimeValues[KeyIndex];

		FKeyHandle NewKeyHandle = NewCurve->FloatCurve.AddKey(TimeValue, CurveValue, false);

		ERichCurveInterpMode NewInterpMode = RCIM_Linear;
		ERichCurveTangentMode NewTangentMode = RCTM_Auto;
		ERichCurveTangentWeightMode NewTangentWeightMode = RCTWM_WeightedNone;

		float LeaveTangent = 0.f;
		float ArriveTangent = 0.f;
		float LeaveTangentWeight = 0.f;
		float ArriveTangentWeight = 0.f;

		NewCurve->FloatCurve.SetKeyInterpMode(NewKeyHandle, NewInterpMode);
		NewCurve->FloatCurve.SetKeyTangentMode(NewKeyHandle, NewTangentMode);
		NewCurve->FloatCurve.SetKeyTangentWeightMode(NewKeyHandle, NewTangentWeightMode);
	}
}

void FAbcImporter::CompressAnimationDataUsingPCA(const FAbcCompressionSettings& InCompressionSettings, const bool bRunComparison /*= false*/)
{
	// Copy poly mesh objects array
	TArray<TSharedPtr<FAbcPolyMeshObject>> PolyMeshObjectsToProcess = ImportData->PolyMeshObjects;
	if (!InCompressionSettings.bBakeMatrixAnimation)
	{
		PolyMeshObjectsToProcess.RemoveAll(
			[=](const TSharedPtr<FAbcPolyMeshObject>& Object)
		{
			return !(Object->bConstantTopology && !Object->bConstant);
		});
	}

	// Non merged path
	const uint32 FrameZeroIndex = 0;

	if (InCompressionSettings.bMergeMeshes)
	{
		TArray<FVector> AverageVertexData;
		TArray<FVector> AverageNormalData;
		float MinTime = FLT_MAX;
		float MaxTime = -FLT_MAX;
		
		FAbcMeshSample MergedZeroFrameSample;
		// Allocate compressed mesh data object
		ImportData->CompressedMeshData.AddDefaulted();
		FCompressedAbcData& CompressedData = ImportData->CompressedMeshData.Last();
				
		TArray<uint32> ObjectVertexOffsets;
		int32 NumSamples = 0;
		// Populate average frame data, frame zero sample and material names from all objects
		for (const TSharedPtr<FAbcPolyMeshObject>& MeshObject : PolyMeshObjectsToProcess)
		{
			ObjectVertexOffsets.Add(AverageVertexData.Num());

			NumSamples = FMath::Max(NumSamples, MeshObject->MeshSamples.Num());
			AbcImporterUtilities::CalculateAverageFrameData(MeshObject, AverageVertexData, AverageNormalData, MinTime, MaxTime);			
			AbcImporterUtilities::AppendMeshSample(&MergedZeroFrameSample, MeshObject->MeshSamples[FrameZeroIndex]);
			AbcImporterUtilities::AppendMaterialNames(MeshObject, CompressedData);
		}
		
		const uint32 NumVertices = AverageVertexData.Num();
		const uint32 NumMatrixRows = NumVertices * 3;
		const uint32 NumIndices = AverageNormalData.Num();
		
		TArray<float> OriginalMatrix;
		OriginalMatrix.AddZeroed(NumMatrixRows * NumSamples);

		uint32 ObjectIndex = 0;
		// For each object generate the delta frame data for the PCA compression
		for (const TSharedPtr<FAbcPolyMeshObject>& MeshObject : PolyMeshObjectsToProcess)
		{
			TArray<float> ObjectMatrix;
			uint32 SampleIndex = 0;
			for (const FAbcMeshSample* MeshSample : MeshObject->MeshSamples)
			{
				AbcImporterUtilities::GenerateDeltaFrameDataMatrix(MeshSample->Vertices, AverageVertexData, SampleIndex * NumMatrixRows, ObjectVertexOffsets[ObjectIndex], OriginalMatrix);
				++SampleIndex;
			}	
			ObjectIndex++;
		}

		// Perform compression
		TArray<float> OutU, OutV, OutMatrix;
		const uint32 NumUsedSingularValues = PerformSVDCompression(OriginalMatrix, NumMatrixRows, NumSamples, OutU, OutV, InCompressionSettings.PercentageOfTotalBases / 100.0f, InCompressionSettings.MaxNumberOfBases);

		// Set up average frame 
		CompressedData.AverageSample = new FAbcMeshSample(MergedZeroFrameSample);
		FMemory::Memcpy(CompressedData.AverageSample->Vertices.GetData(), AverageVertexData.GetData(), sizeof(FVector) * NumVertices);
					
		const float FrameStep = (MaxTime - MinTime) / (float)(NumSamples);
		AbcImporterUtilities::GenerateCompressedMeshData(CompressedData, NumUsedSingularValues, NumSamples, OutU, OutV, FrameStep);
		
		if (bRunComparison)
		{
			CompareCompressionResult(OriginalMatrix, NumSamples, NumMatrixRows, NumUsedSingularValues, NumVertices, OutU, OutV, AverageVertexData);
		}
	}
	else
	{
		// Each individual object creates a compressed data object
		for (const TSharedPtr<FAbcPolyMeshObject>& MeshObject : PolyMeshObjectsToProcess)
		{
			const uint32 NumSamples = MeshObject->MeshSamples.Num();
			const uint32 NumVertices = MeshObject->MeshSamples[FrameZeroIndex]->Vertices.Num();
			const uint32 NumMatrixRows = NumVertices * 3;
			const uint32 NumIndices = MeshObject->MeshSamples[FrameZeroIndex]->Indices.Num();

			TArray<FVector> AverageVertexData;
			TArray<FVector> AverageNormalData;
			float MinTime = FLT_MAX;
			float MaxTime = -FLT_MAX;
			AbcImporterUtilities::CalculateAverageFrameData(MeshObject, AverageVertexData, AverageNormalData, MinTime, MaxTime);

			// Setup original matrix from data
			TArray<float> OriginalMatrix;
			AbcImporterUtilities::GenerateDeltaFrameDataMatrix(MeshObject, AverageVertexData, OriginalMatrix);

			// Perform compression
			TArray<float> OutU, OutV, OutMatrix;
			const uint32 NumUsedSingularValues = PerformSVDCompression(OriginalMatrix, NumMatrixRows, NumSamples, OutU, OutV, InCompressionSettings.PercentageOfTotalBases / 100.0f, InCompressionSettings.MaxNumberOfBases);

			// Allocate compressed mesh data object
			ImportData->CompressedMeshData.AddDefaulted();
			FCompressedAbcData& CompressedData = ImportData->CompressedMeshData.Last();
			CompressedData.Guid = MeshObject->HierarchyGuid;
			CompressedData.AverageSample = new FAbcMeshSample(*MeshObject->MeshSamples[0]);
			FMemory::Memcpy(CompressedData.AverageSample->Vertices.GetData(), AverageVertexData.GetData(), sizeof(FVector) * NumVertices);

			const float FrameStep = (MaxTime - MinTime) / (float)( NumSamples);
			AbcImporterUtilities::GenerateCompressedMeshData(CompressedData, NumUsedSingularValues, NumSamples, OutU, OutV, FrameStep);

			AbcImporterUtilities::AppendMaterialNames(MeshObject, CompressedData);

			if (bRunComparison)
			{
				CompareCompressionResult(OriginalMatrix, NumSamples, NumMatrixRows, NumUsedSingularValues, NumVertices, OutU, OutV, AverageVertexData);
			}
		}
	}
}

void FAbcImporter::CompareCompressionResult(const TArray<float>& OriginalMatrix, const uint32 NumSamples, const uint32 NumRows, const uint32 NumUsedSingularValues, const uint32 NumVertices, const TArray<float>& OutU, const TArray<float>& OutV, const TArray<FVector>& AverageFrame)
{
	// TODO NEED FEEDBACK FOR USER ON COMPRESSION RESULTS
#if 0
	TArray<float> ComparisonMatrix;
	ComparisonMatrix.AddZeroed(OriginalMatrix.Num());
	for (uint32 SampleIndex = 0; SampleIndex < NumSamples; ++SampleIndex)
	{
		const int32 SampleOffset = (SampleIndex * NumRows);
		const int32 CurveOffset = (SampleIndex * NumUsedSingularValues);
		for (uint32 BaseIndex = 0; BaseIndex < NumUsedSingularValues; ++BaseIndex)
		{
			const int32 BaseOffset = (BaseIndex * NumVertices * 3);
			for (uint32 VertexIndex = 0; VertexIndex < NumVertices; VertexIndex++)
			{
				const int32 VertexOffset = (VertexIndex * 3);
				ComparisonMatrix[VertexOffset + SampleOffset + 0] += OutU[BaseOffset + VertexOffset + 0] * OutV[BaseIndex + CurveOffset];
				ComparisonMatrix[VertexOffset + SampleOffset + 1] += OutU[BaseOffset + VertexOffset + 1] * OutV[BaseIndex + CurveOffset];
				ComparisonMatrix[VertexOffset + SampleOffset + 2] += OutU[BaseOffset + VertexOffset + 2] * OutV[BaseIndex + CurveOffset];
			}
		}
	}

	Eigen::MatrixXf EigenOriginalMatrix;
	EigenHelpers::ConvertArrayToEigenMatrix(OriginalMatrix, NumRows, NumSamples, EigenOriginalMatrix);

	Eigen::MatrixXf EigenComparisonMatrix;
	EigenHelpers::ConvertArrayToEigenMatrix(ComparisonMatrix, NumRows, NumSamples, EigenComparisonMatrix);

	TArray<float> AverageMatrix;
	AverageMatrix.AddZeroed(NumRows * NumSamples);
	
	
	for (int32 Index = 0; Index < AverageFrame.Num(); Index++ )
	{
		const FVector& Vector = AverageFrame[Index];
		for (uint32 i = 0; i < NumSamples; ++i)
		{
			const uint32 IndexOffset = (Index * 3) + (i * NumRows);
			AverageMatrix[IndexOffset + 0] = Vector.X;
			AverageMatrix[IndexOffset + 1] = Vector.Y;
			AverageMatrix[IndexOffset + 2] = Vector.Z;
		}		
	}

	Eigen::MatrixXf EigenAverageMatrix;
	EigenHelpers::ConvertArrayToEigenMatrix(AverageMatrix, NumRows, NumSamples, EigenAverageMatrix);

	Eigen::MatrixXf One = (EigenOriginalMatrix - EigenComparisonMatrix);
	Eigen::MatrixXf Two = (EigenOriginalMatrix - EigenAverageMatrix);

	const float LengthOne = One.squaredNorm();
	const float LengthTwo = Two.squaredNorm();
	const float Distortion = (LengthOne / LengthTwo) * 100.0f;

	// Compare arrays
	for (int32 i = 0; i < ComparisonMatrix.Num(); ++i)
	{
		ensureMsgf(FMath::IsNearlyEqual(OriginalMatrix[i], ComparisonMatrix[i]), TEXT("Difference of %2.10f found"), FMath::Abs(OriginalMatrix[i] - ComparisonMatrix[i]));
	}
#endif 
}

const int32 FAbcImporter::PerformSVDCompression(TArray<float>& OriginalMatrix, const uint32 NumRows, const uint32 NumSamples, TArray<float>& OutU, TArray<float>& OutV, const float InPercentage, const int32 InFixedNumValue)
{
	TArray<float> OutS;
	EigenHelpers::PerformSVD(OriginalMatrix, NumRows, NumSamples, OutU, OutV, OutS);

	// Now we have the new basis data we have to construct the correct morph target data and curves
	const float PercentageBasesUsed = InPercentage;
	const int32 NumNonZeroSingularValues = OutS.Num();
	const int32 NumUsedSingularValues = (InFixedNumValue != 0) ? FMath::Min(InFixedNumValue, (int32)OutS.Num()) : (int32)((float)NumNonZeroSingularValues * PercentageBasesUsed);

	// Pre-multiply the bases with it's singular values
	ParallelFor(NumUsedSingularValues, [&](int32 ValueIndex)
	{
		const float Multiplier = OutS[ValueIndex];
		const int32 ValueOffset = ValueIndex * NumRows;

		for (uint32 RowIndex = 0; RowIndex < NumRows; ++RowIndex)
		{
			OutU[ValueOffset + RowIndex] *= Multiplier;
		}
	});

	UE_LOG(LogAbcImporter, Log, TEXT("Decomposed animation and reconstructed with %i number of bases (full %i, percentage %f, calculated %i)"), NumUsedSingularValues, OutS.Num(), PercentageBasesUsed * 100.0f, NumUsedSingularValues);	
	
	return NumUsedSingularValues;
}

UStaticMesh* FAbcImporter::ReimportSingleAsStaticMesh(UStaticMesh* Mesh)
{
	const FString StaticMeshName = Mesh->GetName();
	uint32 MeshObjectIndex = 0;
	// If there is an object in the Alembic file that corresponds to the Mesh's current one use it for importing
	for (TSharedPtr<FAbcPolyMeshObject>& MeshObject : ImportData->PolyMeshObjects)
	{
		if (StaticMeshName.Equals(MeshObject->Name))
		{
			return ImportSingleAsStaticMesh(MeshObjectIndex, Mesh->GetOuter(), RF_Public | RF_Standalone);
		}
		MeshObjectIndex++;
	}

	// Otherwise reimporting failed so return null
	return nullptr;
}

UGeometryCache* FAbcImporter::ReimportAsGeometryCache(UGeometryCache* GeometryCache)
{
	UGeometryCache* ReimportedCache = ImportAsGeometryCache(GeometryCache->GetOuter(), RF_Public | RF_Standalone);

	for (TObjectIterator<UGeometryCacheComponent> CacheIt; CacheIt; ++CacheIt)
	{
		CacheIt->OnObjectReimported(ReimportedCache);
	}

	return ReimportedCache;
}

USkeletalMesh* FAbcImporter::ReimportAsSkeletalMesh(USkeletalMesh* SkeletalMesh)
{
	USkeletalMesh* ReimportedSkeletalMesh = ImportAsSkeletalMesh(SkeletalMesh->GetOuter(), RF_Public | RF_Standalone);
	return ReimportedSkeletalMesh;
}

const TArray<TSharedPtr<FAbcPolyMeshObject>>& FAbcImporter::GetPolyMeshes() const
{
	return ImportData->PolyMeshObjects;
}

const uint32 FAbcImporter::GetNumFrames() const
{
	return (ImportData != nullptr) ? ImportData->NumFrames : 0;
}

const uint32 FAbcImporter::GetNumMeshTracks() const
{
	return (ImportData != nullptr) ? ImportData->PolyMeshObjects.Num() : 0;
}

void FAbcImporter::GenerateRawMeshFromSample(const uint32 MeshTrackIndex, const uint32 SampleIndex, FRawMesh& RawMesh)
{
	FAbcMeshSample* Sample = ImportData->PolyMeshObjects[MeshTrackIndex]->MeshSamples[SampleIndex];
	GenerateRawMeshFromSample(Sample, RawMesh);
}

void FAbcImporter::GenerateRawMeshFromSample(FAbcMeshSample* Sample, FRawMesh& RawMesh)
{
	// Set vertex data for mesh
	RawMesh.VertexPositions = Sample->Vertices;

	// Copy over per-index based data
	RawMesh.WedgeIndices = Sample->Indices;
	RawMesh.WedgeTangentX = Sample->TangentX;
	RawMesh.WedgeTangentY = Sample->TangentY;
	RawMesh.WedgeTangentZ = Sample->Normals;
	RawMesh.WedgeTexCoords[0] = Sample->UVs;
	RawMesh.WedgeColors.AddDefaulted(RawMesh.WedgeIndices.Num());

	// Copy over per-face data
	RawMesh.FaceMaterialIndices = Sample->MaterialIndices;
	RawMesh.FaceSmoothingMasks = Sample->SmoothingGroupIndices;
}

UGeometryCacheTrack_FlipbookAnimation* FAbcImporter::CreateFlipbookAnimationTrack(const FString& TrackName, TSharedPtr<FAbcPolyMeshObject>& InMeshObject, UGeometryCache* GeometryCacheParent, const uint32 MaterialOffset)
{
	UGeometryCacheTrack_FlipbookAnimation* Track = NewObject<UGeometryCacheTrack_FlipbookAnimation>(GeometryCacheParent, FName(*TrackName), RF_Public);

	Alembic::Abc::TimeSamplingPtr TimeSampler = InMeshObject->Mesh.getSchema().getTimeSampling();

	FGeometryCacheMeshData PreviousMeshData;
	bool first = true;

	FScopedSlowTask SlowTask(150, FText::FromString(FString(TEXT("Loading Tracks"))));
	SlowTask.MakeDialog(true);

	// We need all mesh data per sample for vertex animation
	const uint32 NumSamples = InMeshObject->NumSamples;
	for (uint32 SampleIndex = 0; SampleIndex < NumSamples; ++SampleIndex)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("TrackName"), FText::FromString(TrackName));
		Arguments.Add(TEXT("SampleIndex"), FText::AsNumber(SampleIndex + 1));
		Arguments.Add(TEXT("NumSamples"), FText::AsNumber(NumSamples));

		SlowTask.EnterProgressFrame(100.0f / (float)NumSamples, FText::Format(LOCTEXT("AbcImporter_CreateFlipbookAnimationTrack", "Loading Track: {TrackName} [Sample {SampleIndex} of {NumSamples}]"), Arguments));

		// Generate the mesh data for this sample
		FGeometryCacheMeshData MeshData;
		GenerateGeometryCacheMeshDataForSample(MeshData, InMeshObject, SampleIndex, MaterialOffset);
				
		// Get the SampleTime		
		const float SampleTime = InMeshObject->MeshSamples[SampleIndex]->SampleTime; 

		// Store sample in track
		Track->AddMeshSample(MeshData, SampleTime);

		PreviousMeshData = MeshData;

		if (GWarn->ReceivedUserCancel())
		{
			delete Track;
			return nullptr;
		}
	}

	return Track;
}

UGeometryCacheTrack_TransformAnimation* FAbcImporter::CreateTransformAnimationTrack(const FString& TrackName, TSharedPtr<FAbcPolyMeshObject>& InMeshObject, UGeometryCache* GeometryCacheParent, const uint32 MaterialOffset)
{
	// Create the TransformAnimationTrack 
	UGeometryCacheTrack_TransformAnimation* Track = NewObject<UGeometryCacheTrack_TransformAnimation>(GeometryCacheParent, FName(*TrackName), RF_Public);

	// Only need to generate GeometryCacheMeshData for the from the first sample
	const int32 MeshTrackIndex = 0;
	FGeometryCacheMeshData MeshData;
	GenerateGeometryCacheMeshDataForSample(MeshData, InMeshObject, MeshTrackIndex, MaterialOffset);

	Track->SetMesh(MeshData);
	return Track;
}

void FAbcImporter::GenerateGeometryCacheMeshDataForSample(FGeometryCacheMeshData& OutMeshData, TSharedPtr<FAbcPolyMeshObject>& InMeshObject, const uint32 SampleIndex, const uint32 MaterialOffset)
{
	check(SampleIndex < (uint32)InMeshObject->NumSamples);
	check(SampleIndex < (uint32)InMeshObject->MeshSamples.Num());
	
	FAbcMeshSample* MeshSample = InMeshObject->MeshSamples[SampleIndex];

	// Bounding box
	OutMeshData.BoundingBox = FBox(MeshSample->Vertices);

	uint32 NumMaterials = MaterialOffset;

	const int32 NumTriangles = MeshSample->Indices.Num() / 3;	
	const uint32 NumSections = MeshSample->NumMaterials ? MeshSample->NumMaterials : 1;

	TArray<TArray<uint32>> SectionIndices;
	SectionIndices.AddDefaulted(NumSections);

	OutMeshData.Vertices.AddZeroed(MeshSample->Normals.Num());

	for (int32 TriangleIndex = 0; TriangleIndex < NumTriangles; ++TriangleIndex)
	{
		const int32 SectionIndex = MeshSample->MaterialIndices[TriangleIndex];
		TArray<uint32>& Section = SectionIndices[SectionIndex];

		for (int32 VertexIndex = 0; VertexIndex < 3; ++VertexIndex)
		{
			const int32 CornerIndex = (TriangleIndex * 3) + VertexIndex;
			const int32 Index = MeshSample->Indices[CornerIndex];
			FDynamicMeshVertex& Vertex = OutMeshData.Vertices[CornerIndex];

			Vertex.Position = MeshSample->Vertices[Index];
			Vertex.SetTangents(MeshSample->TangentX[CornerIndex], MeshSample->TangentY[CornerIndex], MeshSample->Normals[CornerIndex]);
			Vertex.TextureCoordinate = MeshSample->UVs[CornerIndex];
			Vertex.Color = FColor::White;

			Section.Add(CornerIndex);
		}
	}

	OutMeshData.BatchesInfo.AddDefaulted(SectionIndices.Num());

	TArray<uint32>& Indices = OutMeshData.Indices;
	for (uint32 BatchIndex = 0; BatchIndex < NumSections; ++BatchIndex)
	{
		FGeometryCacheMeshBatchInfo& BatchInfo = OutMeshData.BatchesInfo[BatchIndex];
		BatchInfo.StartIndex = Indices.Num();
		BatchInfo.MaterialIndex = NumMaterials;
		NumMaterials++;

		BatchInfo.NumTriangles = SectionIndices[BatchIndex].Num() / 3;
		Indices.Append(SectionIndices[BatchIndex]);
	}

	bool check = true;
}


void FAbcImporter::GenerateSkeletalMeshDataFromCompressedData(FSkeletalMeshImportData& SkeletalMeshData, const TArray<FCompressedAbcData>& CompressedMeshData)
{
	uint32 VertexOffset = 0;
	uint32 IndexOffset = 0;
	uint32 WedgeOffset = 0;
	uint32 TriangleOffset = 0;
	for (const FCompressedAbcData& CompressedData : CompressedMeshData)
	{
		FAbcMeshSample* Sample = CompressedData.AverageSample;
		const uint32 NumVertices = Sample->Vertices.Num();
		const uint32 NumIndices = Sample->Indices.Num();
		const uint32 NumFace = NumIndices / 3;

		// Append vertices
		SkeletalMeshData.Points.Append(Sample->Vertices);

		// Influences
		SkeletalMeshData.Influences.AddZeroed(NumVertices);

		int32 VertexIndex = VertexOffset;
		for (uint32 InfluenceIndex = 0; InfluenceIndex < NumVertices; ++InfluenceIndex)
		{
			VRawBoneInfluence& Influence = SkeletalMeshData.Influences[VertexIndex];
			Influence.BoneIndex = 0;
			Influence.VertexIndex = VertexIndex;
			Influence.Weight = 1.0f;
			++VertexIndex;
		}

		// Points to raw data
		SkeletalMeshData.PointToRawMap.AddZeroed(NumVertices);
		VertexIndex = VertexOffset;
		for (uint32 PointIndex = 0; PointIndex < NumVertices; ++PointIndex)
		{
			SkeletalMeshData.PointToRawMap[PointIndex + VertexOffset] = VertexIndex;
			++VertexIndex;
		}

		// Wedges
		SkeletalMeshData.Wedges.AddZeroed(NumIndices);

		for (uint32 WedgeIndex = 0; WedgeIndex < NumIndices; ++WedgeIndex)
		{
			VVertex& Wedge = SkeletalMeshData.Wedges[WedgeIndex + WedgeOffset];

			// Retrieve index (to vertices)
			const uint32& Index = Sample->Indices[WedgeIndex];
			Wedge.VertexIndex = Index + VertexOffset;
			Wedge.UVs[0] = Sample->UVs.Num() ? Sample->UVs[WedgeIndex] : FVector2D();
			Wedge.Color = FColor::White; //Sample->Colours.Num() ? Sample->Colours[WedgeIndex] : white for now TODO
			Wedge.MatIndex = 0;
		}

		// Faces
		SkeletalMeshData.Faces.AddZeroed(Sample->Indices.Num() / 3);

		for (uint32 FaceIndex = 0; FaceIndex < NumFace; ++FaceIndex)
		{
			VTriangle& Face = SkeletalMeshData.Faces[TriangleOffset + FaceIndex];

			Face.SmoothingGroups = Sample->SmoothingGroupIndices[FaceIndex];
			Face.MatIndex = (uint8)Sample->MaterialIndices[FaceIndex];
			for (int32 Index = 0; Index < 3; ++Index)
			{
				const int32 OffsetIndex = ((FaceIndex * 3) + Index);
				Face.WedgeIndex[Index] = OffsetIndex + WedgeOffset;
				Face.TangentX[Index] = Sample->TangentX[OffsetIndex];
				Face.TangentY[Index] = Sample->TangentY[OffsetIndex];
				Face.TangentZ[Index] = Sample->Normals[OffsetIndex];

				SkeletalMeshData.Wedges[OffsetIndex + TriangleOffset].MatIndex = Face.MatIndex;
			}
		}

		SkeletalMeshData.NumTexCoords = 1;
		WedgeOffset += NumIndices;
		VertexOffset += NumVertices;
		TriangleOffset += NumIndices / 3;
	}
}

void FAbcImporter::GenerateMorphTargetVertices(FAbcMeshSample* BaseSample, TArray<FMorphTargetDelta> &MorphDeltas, FAbcMeshSample* AverageSample, uint32 WedgeOffset)
{
	const uint32 BaseNumIndices = BaseSample->Indices.Num();
	MorphDeltas.AddDefaulted(BaseNumIndices);

	for (uint32 VertIndex = 0; VertIndex < BaseNumIndices; ++VertIndex)
	{
		// Vertex index
		const uint32& Index = BaseSample->Indices[VertIndex];

		FMorphTargetDelta& MorphVertex = MorphDeltas[VertIndex];
		// Position delta
		MorphVertex.PositionDelta = BaseSample->Vertices[Index] - AverageSample->Vertices[Index];
		// normal delta
		MorphVertex.TangentZDelta = BaseSample->Normals[VertIndex] - AverageSample->Normals[VertIndex];
		// index of base mesh vert this entry is to modify
		MorphVertex.SourceIdx = WedgeOffset + VertIndex;
	}
}

UMaterial* FAbcImporter::RetrieveMaterial(const FString& MaterialName, UObject* InParent, EObjectFlags Flags)
{
	UMaterial* Material = nullptr;
	UMaterial** CachedMaterial = ImportData->MaterialMap.Find(MaterialName);
	if (CachedMaterial)
	{
		Material = *CachedMaterial;

		if (Material->GetOuter() == GetTransientPackage())
		{			
			Material->Rename(*MaterialName, InParent);
			Material->SetFlags(Flags);
		}
	}
	else
	{
		Material = UMaterial::GetDefaultMaterial(MD_Surface);
		check(Material);
	}

	return Material;
}

void FAbcImporter::GetMatrixSamplesForObject(const Alembic::Abc::IObject& Object, TArray<FMatrix>& MatrixSamples, TArray<float>& SampleTimes)
{
	// Get Hierarchy for the current Object
	TDoubleLinkedList<Alembic::AbcGeom::IXform> Hierarchy;
	GetHierarchyForObject(Object, Hierarchy);

	// This is in here for safety, normally Alembic writes out same sample count for every node
	uint32 HighestNumSamples = 0;
	TDoubleLinkedList<Alembic::AbcGeom::IXform>::TConstIterator It(Hierarchy.GetHead());
	while (It)
	{
		const uint32 NumSamples = (*It).getSchema().getNumSamples();
		if (NumSamples > HighestNumSamples)
		{
			HighestNumSamples = NumSamples;
		}
		It++;
	}

	// If there are no samples available we push out one identity matrix
	if (HighestNumSamples == 0)
	{
		SampleTimes.Push(0.0f);
		MatrixSamples.Push(FMatrix(FPlane(1.0f, 0.0f, 0.0f, 0.0f), FPlane(0.0f, 1.0f, 0.0f, 0.0f), FPlane(0.0f, 0.0f, 1.0f, 0.0f), FPlane(0.0f, 0.0f, 0.0f, 1.0f)));
	}

	// For each sample in the hierarchy retrieve the world matrix for Object
	for (uint32 SampleIndex = 0; SampleIndex < HighestNumSamples; ++SampleIndex)
	{
		float AverageSampleTime = 0.0f;
		Alembic::Abc::M44d WorldMatrix;

		// Traverse DLinkedList back to front		
		It = TDoubleLinkedList<Alembic::AbcGeom::IXform>::TConstIterator(Hierarchy.GetTail());

		while (It)
		{
			// Get schema from parent object
			Alembic::AbcGeom::XformSample sample;
			Alembic::AbcGeom::IXformSchema Schema = (*It).getSchema();
			Schema.get(sample, SampleIndex);

			// Get matrix and concatenate
			Alembic::Abc::M44d Matrix = sample.getMatrix();
			WorldMatrix *= Matrix;

			// Get TimeSampler for this sample's time
			Alembic::Abc::TimeSamplingPtr TimeSampler = Schema.getTimeSampling();
			AverageSampleTime += (float)TimeSampler->getSampleTime(SampleIndex);

			It--;
		}

		AverageSampleTime /= Hierarchy.Num();

		// Store Sample data and time
		FMatrix Matrix = AbcImporterUtilities::ConvertAlembicMatrix(WorldMatrix);
		if (MatrixSamples.Num() > 0)
		{
			if (MatrixSamples.Last() == Matrix)
			{
				continue;
			}
		}
		SampleTimes.Push(AverageSampleTime);
		MatrixSamples.Push(Matrix);
	}
}

void FAbcImporter::GetHierarchyForObject(const Alembic::Abc::IObject& Object, TDoubleLinkedList<Alembic::AbcGeom::IXform>& Hierarchy)
{
	Alembic::Abc::IObject Parent;
	Parent = Object.getParent();

	// Traverse through parents until we reach RootNode
	while (Parent.valid())
	{
		// Only if the Object is of type IXform we need to store it in the hierarchy (since we only need them for matrix animation right now)
		if (AbcImporterUtilities::IsType<Alembic::AbcGeom::IXform>(Parent.getMetaData()))
		{
			Hierarchy.AddHead(Alembic::AbcGeom::IXform(Parent, Alembic::Abc::kWrapExisting));
		}
		Parent = Parent.getParent();
	}
}

#undef LOCTEXT_NAMESPACE
