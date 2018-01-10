// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

//#include "Engine.h"
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Features/IModularFeatures.h"
#include "IProxyLODPlugin.h"
#include "RawMesh.h"
#include "MeshMergeData.h"
#include "MaterialUtilities.h" // for FFlattenMaterial 
#include "Engine/StaticMesh.h"
#include "Misc/ScopedSlowTask.h"
#include "Stats/StatsMisc.h"
//#include "ScopedTimers.h"
#include <openvdb/openvdb.h>

#define PROXYLOD_CLOCKWISE_TRIANGLES  1

#define LOCTEXT_NAMESPACE "ProxyLODMeshReduction"

#include "ProxyLODMaterialTransferUtilities.h"
#include "ProxyLODMeshAttrTransfer.h"
#include "ProxyLODMeshConvertUtils.h"
#include "ProxyLODMeshTypes.h"
#include "ProxyLODMeshUtilities.h"
#include "ProxyLODMeshParameterization.h"
#include "ProxyLODMeshSDFConversions.h"

#include "ProxyLODParallelSimplifier.h"
#include "ProxyLODRasterizer.h"
#include "ProxyLODSimplifier.h"


#include "ProxyLODkDOPInterface.h"
#include "ProxyLODThreadedWrappers.h"



#define PROXYLOD_USE_TEST_CVARS 1

#if PROXYLOD_USE_TEST_CVARS	

// Disable any mesh simplification and UV-generation.
// Will result in very high poly red geometry.
static TAutoConsoleVariable<int32> CVarProxyLODRemeshOnly(
	TEXT("r.ProxyLODRemeshOnly"),
	0,
	TEXT("Only remesh.  No simplification or materials. Default off.\n")
	TEXT("0: Disabled - will simplify and generate materials \n")
	TEXT("1: Enabled  - will not simplfy or generate materials."),
	ECVF_Default);

// Allow for adding vertex colors to the output mesh that correspond
// to the different charts in the uv-atlas.
static TAutoConsoleVariable<int32> CVarProxyLODChartColorVerts(
	TEXT("r.ProxyLODChartColorVerts"),
	0,
	TEXT("Color verts by uv chart.  Default off.\n")
	TEXT("0: Disabled \n")
	TEXT("1: Enabled."),
	ECVF_Default);

// Testing methods to choose correct hit when shooting rays between
// simplified and source goemetry.
static TAutoConsoleVariable<int32> CVarProxyLODTransfer(
	TEXT("r.ProxyLODTransfer"),
	1,
	TEXT("0: shoot both ways, 1: preference for forward (default)"),
	ECVF_Default);


// Attempt to separate the sides of collapsed walls.
static TAutoConsoleVariable<int32> CVarProxyLODCorrectCollapsedWalls(
	TEXT("r.ProxyLODCorrectCollapsedWalls"),
	0,
	TEXT("Shall the ProxyLOD system attemp to correct walls with interpenetrating faces")
	TEXT("0: disabled (default)\n")
	TEXT("1: endable, may cause cracks."),
	ECVF_Default);


// Testing tangent space encoding of normal maps by optionally using
// a default wold-space aligned tangent space.
static TAutoConsoleVariable<int32> CVarProxyLODUseTangentSpace(
	TEXT("r.ProxyLODUseTangentSpace"),
	1,
	TEXT("Shall the ProxyLOD system generate a true tangent space at each vertex")
	TEXT("0: world space at each vertex\n")
	TEXT("1: tangent space at each vertex (default)"),
	ECVF_Default);


// Force the simplifier to use single threaded code path.
static TAutoConsoleVariable<int32> CVarProxyLODSingleThreadSimplify(
	TEXT("r.ProxyLODSingleThreadSimplify"),
	0,
	TEXT("Use single threaded code path. Default off.\n")
	TEXT("0: Multithreaded \n")
	TEXT("1: Single threaded."),
	ECVF_Default);

// Disable the parallel flattening of the source textures.
static TAutoConsoleVariable<int32> CVarProxyLODMaterialInParallel(
	TEXT("r.ProxyLODMaterialInParallel"),
	1,
	TEXT("0: disable doing material work in parallel with mesh simplification\n")
	TEXT("1: enable - default"),
	ECVF_Default);

#endif

/**
* Implementation of the required IMeshMerging Interface.
* This class does the actual work.
*
* See HieararchicalLOD.cpp
*/
class FVoxelizeMeshMerging : public IMeshMerging
{
public:

	typedef TArray<FMeshMergeData>    FMeshMergeDataArray;
	typedef TArray<FFlattenMaterial>  FFlattenMaterialArray;
	typedef openvdb::math::Transform  OpenVDBTransform;

	FVoxelizeMeshMerging();

	~FVoxelizeMeshMerging();

	// Construct the proxy geometry and materials - the results
	// are captured by a call back.

	virtual void ProxyLOD(const FMeshMergeDataArray& InData,
		                  const FMeshProxySettings& InProxySettings,
		                  const FFlattenMaterialArray& InputMaterials,
		                  const FGuid InJobGUID) override;

	virtual void AggregateLOD() override
	{};

	virtual bool bSupportsParallelMaterialBake() override;
	

	virtual FString GetName() override
	{
		return FString("ProxyLODMeshMerging");
	}

	//  Update internal options from current CVar values.
	void CaptureCVars();


private:

	// Restore default parameters
	void RestoreDefaultParameters();

	// Compute the voxel resolution and create XForm from world to voxel space
	// @param InProxySetting  Control setting from UI
	// @param ObjectSize      Major Axis length of the geometry
	OpenVDBTransform::Ptr ComputeResolution(const FMeshProxySettings& InProxySettings, float ObjectSize);

private:

	// In voxel units.  This defines the isosurface to be extracted
	// from the levelset.

	float IsoSurfaceValue = 0.5;

	// Determine the method used to determine the correct ray hit
	// when creating a correspondence between simplified geometry
	// and src geometry.

	int32 RayHitOrder = 1;

	// Flag to set if the verts should be colored by char in the UV atlas.
	// Useful for debuging.

	bool bChartColorVerts = false;

	// Flag to set that determines if a true tangent space is generated.
	// if false, a (1,0,0), (0, 1,0), (0, 0,1) space is used on each vert.

	bool bUseTangentSpace = true;

	// Flag to disable the simplification and transfer of materials.
	// When set to true the input material will be voxelized and remeshed
	// only.

	bool bRemeshOnly = false;

	// Flag that can enable/disable the multi-threaded aspect of the simplifier.

	bool bMutliThreadSimplify = true;

	// Flag to enable the thin wall correction.
	// Very thin walls can develop mesh interpenetration(opposing wall surfaces meet) during simplification.This
	// can produce rendering artifacts(related to distance field shadows and ao).

	bool bCorrectCollapsedWalls = false;
};

/**
*  Required MeshReduction Interface.
*/
class FProxyLODMeshReduction : public IProxyLODMeshReduction
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;


	/** IMeshReductionModule interface.*/
	// not supported !
	virtual class IMeshReduction* GetSkeletalMeshReductionInterface() override;
	
	// not supported !
	virtual class IMeshReduction* GetStaticMeshReductionInterface()   override;

	/** 
	* Voxel-based merging & remeshing.  
	* @return pointer to a member data instance of specialized IMeshMerging subclass
	*/
	virtual class IMeshMerging*   GetMeshMergingInterface()           override;

	/**
	* Retrieve the distributed mesh merging interface.
	* NB: currently not supported
	* @todo: add support.
	*/
	virtual class IMeshMerging* GetDistributedMeshMergingInterface()  override;
	

	virtual FString GetName() override
	{
		return FString("ProxyLODMeshReduction");
	};

private:

	FVoxelizeMeshMerging MeshMergingTool;
};

DEFINE_LOG_CATEGORY_STATIC(LogProxyLODMeshReduction, Log, All);

IMPLEMENT_MODULE(FProxyLODMeshReduction, ProxyLODMeshReduction)




void FProxyLODMeshReduction::StartupModule()
{
	// Global registration of  the vdb types.

	openvdb::initialize();

	IModularFeatures::Get().RegisterModularFeature(IMeshReductionModule::GetModularFeatureName(), this);
}


void FProxyLODMeshReduction::ShutdownModule()
{

	// Global deregistration of vdb types

	openvdb::uninitialize();

	IModularFeatures::Get().UnregisterModularFeature(IMeshReductionModule::GetModularFeatureName(), this);

}


IMeshReduction*  FProxyLODMeshReduction::GetStaticMeshReductionInterface()
{
	return nullptr;
}

IMeshReduction*  FProxyLODMeshReduction::GetSkeletalMeshReductionInterface()
{
	return nullptr;
}

IMeshMerging* FProxyLODMeshReduction::GetDistributedMeshMergingInterface()  
{
	return nullptr;
}

IMeshMerging*  FProxyLODMeshReduction::GetMeshMergingInterface()
{

	return &MeshMergingTool;
}


FVoxelizeMeshMerging::FVoxelizeMeshMerging()
{}

FVoxelizeMeshMerging::~FVoxelizeMeshMerging()
{}


bool FVoxelizeMeshMerging::bSupportsParallelMaterialBake()
{
#if PROXYLOD_USE_TEST_CVARS	
	bool bDoParallel = (CVarProxyLODMaterialInParallel.GetValueOnAnyThread() == 1);
#else
	bool bDoParallel = true;
#endif 
	return bDoParallel;
}

void FVoxelizeMeshMerging::CaptureCVars()
{

#if PROXYLOD_USE_TEST_CVARS
	// Allow CVars to be used.
	{
		int32 RayOrder                 = CVarProxyLODTransfer.GetValueOnGameThread();
		bool bAddChartColorVerts       = (CVarProxyLODChartColorVerts.GetValueOnGameThread() == 1);
		bool bUseTrueTangentSpace      = (CVarProxyLODUseTangentSpace.GetValueOnGameThread() == 1);
		bool bVoxelizeAndRemeshOnly    = (CVarProxyLODRemeshOnly.GetValueOnGameThread() == 1);
		bool bSingleThreadedSimplify   = (CVarProxyLODSingleThreadSimplify.GetValueOnAnyThread() == 1);
		bool bWallCorreciton           = (CVarProxyLODCorrectCollapsedWalls.GetValueOnGameThread() == 1);

		// set values  - note, this class is a global (singleton) instance.
		this->RayHitOrder              = RayOrder;
		this->bChartColorVerts         = bAddChartColorVerts;
		this->bUseTangentSpace         = bUseTrueTangentSpace;
		this->bRemeshOnly              = bVoxelizeAndRemeshOnly;
		this->bMutliThreadSimplify     = !bSingleThreadedSimplify;
		this->bCorrectCollapsedWalls   = bWallCorreciton;
	}
#else
	RestoreDefaultParameters();
#endif 
}

void FVoxelizeMeshMerging::RestoreDefaultParameters()
{
	IsoSurfaceValue   = 0.5;
	RayHitOrder       = 1;
	bChartColorVerts  = false;
	bUseTangentSpace  = true;
	bRemeshOnly       = false;
}

FVoxelizeMeshMerging::OpenVDBTransform::Ptr 
FVoxelizeMeshMerging::ComputeResolution(const FMeshProxySettings& InProxySettings, float OjbectSize)
{
	// Compute the required voxel size in world units.
	// if the requested voxel size is non-physical, use a default of 3
	double VoxelSize = 3.;
	int32 PixelCount = FMath::Max(InProxySettings.ScreenSize, (int32)50);
	PixelCount = FMath::Min(PixelCount, (int32)900);
	
	if (OjbectSize > 0.f ) 
	{
		// pixels per length scale
		const double LengthPerPixel = double(OjbectSize) / double(PixelCount);
		VoxelSize = ( LengthPerPixel * 1.95 / 3.); // magic scale.
	}
	
	// Maybe override the computed VoxelSize

	if (InProxySettings.bOverrideVoxelSize)
	{
		VoxelSize = InProxySettings.VoxelSize;
	}

	UE_LOG(LogProxyLODMeshReduction, Log, TEXT("Spatial Sampling Distance Scale used %f"), VoxelSize);
	
	return  OpenVDBTransform::createLinearTransform(VoxelSize);
}



typedef FAOSMesh           FSimplifierMeshType;


/**
* Replace the AOSMesh with a simplified version of itself.
*
* @param SrcGeometryPolyField   Container that holds the original geometry - primarily used to determine the number of polys the
*                               original geometry would bin in each parallel partition. 
* @param PixelCoverage          PixelCoverage and PercentToRetain are used in the heuristic that determines how much simplification is needed.          
* @param PercentToRetain
* @param InOutMesh              Simplfied mesh, replaces the input mesh.
* @param bSingleThreaded        Control for single threaded evaluation of the simplifier.
*/

static void SimplifyMesh( const FClosestPolyField& SrcGeometryPolyField, 
	                      const int32 PixelCoverage, 
	                      const float PercentToRetain, 
	                      FSimplifierMeshType& InOutMesh, 
	                      bool bSingleThreaded)
{
	//SCOPE_LOG_TIME(TEXT("UE4_ProxyLOD_Simplifier"), nullptr);

	// Compute some of the metrics that relate the desired resolution to simplifier parameters.
	const FRawMeshArrayAdapter& SrcGeometryAdapter = SrcGeometryPolyField.MeshAdapter();
	const ProxyLOD::FBBox& SrcBBox = SrcGeometryAdapter.GetBBox();
	const float MaxSide = SrcBBox.extents().length();

	const float DistPerPixel = MaxSide / (float)PixelCoverage;

	// Determine the cost of removing a vert from a cube of the designated feature size.
	// NB: This is a magic number
	
	const float LengthScale = 170.f;
	float MaxFeatureCost = DistPerPixel * DistPerPixel * DistPerPixel * LengthScale;

	if (!bSingleThreaded)
	{
		ProxyLOD::ParallelSimplifyMesh(SrcGeometryPolyField, PercentToRetain, MaxFeatureCost, InOutMesh);
	}
	else
	{
		// The iso-surface mesh initially has more polys than the src geometry.
		// Setting the max number to retain to the poly count of the original geometry
		// will still result in a lot of simplification of the iso-surface mesh.

		const int32 MaxTriNumToRetain = (int32)SrcGeometryAdapter.polygonCount();
		const int32 MinTriNumToRetain = FMath::CeilToInt(MaxTriNumToRetain * PercentToRetain);
		ProxyLOD::FSimplifierTerminator Terminator(MinTriNumToRetain, MaxTriNumToRetain, MaxFeatureCost);
		ProxyLOD::SimplifyMesh(Terminator, InOutMesh);
	}

}

static float BBoxMajorAxisLength(const ProxyLOD::FBBox& BBox)
{
	return BBox.extents()[BBox.maxExtent()];
}

/**
* Compute the size of the UV texture atlas.  This will be square, and the length of a side
* will correspond to the longest side of requested textures.
*
* NB: A min size of 64x64 is enforced.
*/
static FIntPoint GetTexelGridSize(const  FMaterialProxySettings& MaterialSettings)
{
	auto MaxBBox = [](const FIntPoint& A, const FIntPoint B)
	{
		return FIntPoint(FMath::Max(A.X, B.X), FMath::Max(A.Y, B.Y));
	};

	// Start with a min value of 64x64.  
	FIntPoint MaxTextureSize(64, 64);

	const bool bUseTextureSize =
		(MaterialSettings.TextureSizingType == ETextureSizingType::TextureSizingType_UseSingleTextureSize) ||
		(MaterialSettings.TextureSizingType == ETextureSizingType::TextureSizingType_UseAutomaticBiasedSizes);

	if (bUseTextureSize)
	{
		MaxTextureSize = MaxBBox(MaxTextureSize, MaterialSettings.TextureSize);
	}
	else if (MaterialSettings.TextureSizingType == ETextureSizingType::TextureSizingType_UseManualOverrideTextureSize)
	{
		MaxTextureSize = MaxBBox(MaxTextureSize, MaterialSettings.DiffuseTextureSize);

		if (MaterialSettings.bNormalMap)
		{
			MaxTextureSize = MaxBBox(MaxTextureSize, MaterialSettings.NormalTextureSize);
		}
		if (MaterialSettings.bMetallicMap)
		{
			MaxTextureSize = MaxBBox(MaxTextureSize, MaterialSettings.MetallicTextureSize);
		}
		if (MaterialSettings.bRoughnessMap)
		{
			MaxTextureSize = MaxBBox(MaxTextureSize, MaterialSettings.RoughnessTextureSize);
		}
		if (MaterialSettings.bSpecularMap)
		{
			MaxTextureSize = MaxBBox(MaxTextureSize, MaterialSettings.SpecularTextureSize);
		}
		if (MaterialSettings.bEmissiveMap)
		{
			MaxTextureSize = MaxBBox(MaxTextureSize, MaterialSettings.EmissiveTextureSize);
		}
		if (MaterialSettings.bOpacityMap)
		{
			MaxTextureSize = MaxBBox(MaxTextureSize, MaterialSettings.OpacityTextureSize);
		}
		if (MaterialSettings.bOpacityMaskMap)
		{
			MaxTextureSize = MaxBBox(MaxTextureSize, MaterialSettings.OpacityMaskTextureSize);
		}
		if (MaterialSettings.bAmbientOcclusionMap)
		{
			MaxTextureSize = MaxBBox(MaxTextureSize, MaterialSettings.AmbientOcclusionTextureSize);
		}
	}

	// Make the UVs square
	int32 MaxLength = FMath::Max(MaxTextureSize.X, MaxTextureSize.Y);
	return FIntPoint(MaxLength, MaxLength);
}

void FVoxelizeMeshMerging::ProxyLOD(const FMeshMergeDataArray& InData, const FMeshProxySettings& InProxySettings, const FFlattenMaterialArray& InputMaterials, const FGuid InJobGUID)
{

	// Update any pipeline controlling parameters.

	CaptureCVars();


	// Container for the raw mesh that will hold the simplified geometry
	// and the FlattenMaterial that will hold the materials for the output mesh.
	// NB: These will be the product of this function and 
	//     will be captured by the CompleteDelegate. 

	FRawMesh OutRawMesh;

	FFlattenMaterial OutMaterial = FMaterialUtilities::CreateFlattenMaterialWithSettings(InProxySettings.MaterialSettings);

	bool bProxyGenerationSuccess = true;
	// Compute the simplified mesh and related materials.
	{
	
		// Create an adapter to make the data appear as a single mesh as required by the voxelization code.

		FRawMeshArrayAdapter SrcGeometryAdapter(InData);

		{
			const auto& BBox = SrcGeometryAdapter.GetBBox();
			// Determine the voxel size the corresponds to the InProxySettings
			// The transform defines resolution of the voxel grid.

			OpenVDBTransform::Ptr XForm = ComputeResolution(InProxySettings, BBoxMajorAxisLength(BBox) );
			SrcGeometryAdapter.SetTransform(XForm);
		}

		const double VoxelSize = SrcGeometryAdapter.GetTransform().voxelSize()[0];

		


		// --- Set pointers and containers shared by the various threaded stages ---
		// NB: These need to be declared outside of the thread task scope.

		// Bake the materials pointers and containers. 

		FFlattenMaterialArray LocalBakedMaterials;
		const FFlattenMaterialArray*  BakedMaterials = &InputMaterials;

		// Smart pointer use to manage memory

		FClosestPolyField::ConstPtr SrcGeometryPolyField;

		// Mesh types that will be shared by various stages.
		FSimplifierMeshType AOSMeshedVolume; 
		FVertexDataMesh VertexDataMesh;

		// Description of texture atlas.
		// Make the UV space work with the largest texture size.
		
		const  FMaterialProxySettings& MaterialSettings = InProxySettings.MaterialSettings;
		const FIntPoint UVSize = GetTexelGridSize(MaterialSettings);
	
		
		ProxyLOD::FTextureAtlasDesc TextureAtlasDesc(UVSize, MaterialSettings.GutterSpace);
	    
		// --- Create New (High Poly) Geometry --
		// 1) Voxelize the source geometry
		// 2) Extract high-poly surfaces
		// 3) Capture closest poly-field grid that allows quick identification of the poly closest to a voxel center.
		// 4) Transfer normals to the new geometry. 
		// The steps in the following scope are fully parallelized
		{
			{
				// Parameters for the voxelization and iso-surface extraction. 

				const double WSIsoValue = VoxelSize * this->IsoSurfaceValue; //  convert to world space units
				const double RemeshAdaptivity = 0.01f; // the re-meshing code does some internal adaptivity based on local normals.


				openvdb::FloatGrid::Ptr SDFVolume; 
				openvdb::Int32Grid::Ptr SrcPolyIndexGrid = openvdb::Int32Grid::create();

				// 1) Voxelize - this can potentially run out of memory when very large objecs (or very small voxel sizes) are used.

				bool bSuccess = ProxyLOD::MeshArrayToSDFVolume(SrcGeometryAdapter, SDFVolume, SrcPolyIndexGrid.get());

				if (bSuccess)
				{
					// 2) Extract the iso-surface into a mesh format directly consumable by the simplifier

					ProxyLOD::ExtractIsosurfaceWithNormals(SDFVolume, WSIsoValue, RemeshAdaptivity, AOSMeshedVolume);
				}
				else
				{
					UE_LOG(LogProxyLODMeshReduction, Warning, TEXT("Allocation Error: The objects were too large for the selected Spatial Sampling Distance"));

					// Make a simple cube for output.

					ProxyLOD::MakeCube(AOSMeshedVolume, 100);

					// Skip the UV and material steps

					this->bRemeshOnly = true;
					bProxyGenerationSuccess = false;
				}

				// 3) Create an object that allows for closest poly queries against the source mesh.

				SrcGeometryPolyField = FClosestPolyField::create(SrcGeometryAdapter, SrcPolyIndexGrid);

				// Let the SDFVolume go out of scope, no longer needed.
			}

			// 4) Transfers the src geometry normals to the simplifier mesh.  

			ProxyLOD::TransferSrcNormals(*SrcGeometryPolyField, AOSMeshedVolume);

		}


		// Project the verts onto the source geometry.  This helps
		// insure that single-sided objects don't become too thick.

		ProxyLOD::ProjectVertexOntoSrcSurface(*SrcGeometryPolyField, AOSMeshedVolume);

		// Allow for the parallel execution of baking the source textures and generated the simplified geometry with UVs.
	
		bool bUVGenerationSucess       = false;
		const bool bColorVertsByChart      = this->bChartColorVerts;
		const bool bDoCollapsedWallFix     = this->bCorrectCollapsedWalls;
		const bool bSingleThreadedSimplify = !(this->bMutliThreadSimplify);
			
		if (!this->bRemeshOnly)
		{
			ProxyLOD::FTaskGroup PrepGeometryAndBakeMaterialsTaskGroup;
			
			// --- Convert High Poly Iso Surface To Simplifed Geometry and Prepare for Materials ---
			// 1) Simplify Geometry
			// 2) Compute Normals for Simplified Geometry
			// 3) Generate UVs for Simplified Geometry

			PrepGeometryAndBakeMaterialsTaskGroup.Run([&AOSMeshedVolume,
				&SrcGeometryPolyField, &VertexDataMesh,
				&TextureAtlasDesc, &bUVGenerationSucess, &InProxySettings,
				VoxelSize, bColorVertsByChart, bSingleThreadedSimplify, bDoCollapsedWallFix]()
			{

				// 1) Simplified mesh and convert it to the correct format for UV generation

				{
					
					// Compute the number target number of polys.
					
					const int32 PixelCoverage = InProxySettings.ScreenSize;
				
					// By default, we don't want the simplifier to toss much more than 98% of the triangles.
				
					float PercentToRetain = 0.02f; 

					// Replaces the AOS mesh with a simplified version

					FSimplifierMeshType& SimplifierMesh = AOSMeshedVolume;
					SimplifyMesh(*SrcGeometryPolyField, PixelCoverage, PercentToRetain, SimplifierMesh, bSingleThreadedSimplify);

					// Project the verts onto the source geometry.  This helps
					// insure that single-sided objects don't become too thick.

					//ProjectVerticesOntoSrc(*SrcRefernce, AOSMeshedVolume);

					// Convert to format used by the UV generation code

					ProxyLOD::ConvertMesh(SimplifierMesh, VertexDataMesh);

					// --- Attempt to fix-up the geometry if needed ---

					if (bDoCollapsedWallFix)
					{
						// Use heuristics to try to improve the finial mesh by comparing with the source meshes.
						// Todo:  try moving this to before the Correspondence creation.
						// 	ProjectVerticesOntoSrc<true /*snap to vertex*/>(*SrcGeometryPolyField, OutRawMesh);

						// Very thin walls may have generated inter-penetration. Attempt to fix.

						int32 testCount = ProxyLOD::CorrectCollapsedWalls(VertexDataMesh, VoxelSize);
					}
				}

				// 2) Add angle weighted vertex normals

				ProxyLOD::ComputeVertexNormals(VertexDataMesh, true /*angle weighted*/);

				// 3) Generate UVs for Simplified Geometry
				// UV Atlas create, this can fail.
				// NB: Vertices are split on UV seams. 

				bUVGenerationSucess = ProxyLOD::GenerateUVs(VertexDataMesh, TextureAtlasDesc, bColorVertsByChart);


			});

			// --- Bake the materials ---
			// This only needs to be done before the material transfer step, but it can be the slowest
			// step in the process, so we overlap simplification by doing it in this task group.
			//
			// This guarantees that the BakeMaterialsDelegate task will run on the main thread ( i.e. the game thread)
			// NB: The BakeMaterialsDelegate is required by other code to run on the game thread
			// The wait acts like a join, e.g. both tasks will complete.

			PrepGeometryAndBakeMaterialsTaskGroup.RunAndWait(
				[&]()->void 
				{
					if (this->bSupportsParallelMaterialBake() && BakeMaterialsDelegate.IsBound())
					{
						IMeshMerging::BakeMaterialsDelegate.Execute(LocalBakedMaterials);
						BakedMaterials = &LocalBakedMaterials;
					}
				}
			);

		}
		else
		{
			// Convert to the expected mesh type.

			ProxyLOD::ConvertMesh(AOSMeshedVolume, VertexDataMesh);
		}

		// Update the locations of the vertexes.  This has no affect on the tangent space
		// ProxyLOD::ProjectVertexWithSnapToNearest(*SrcGeometryPolyField, VertexDataMesh);

		// Using the new UVs fill OutMaterial texture atlas. 

		if (bUVGenerationSucess && SrcGeometryPolyField)
		{
			ProxyLOD::FRasterGrid::Ptr DstUVGrid;
			ProxyLOD::FRasterGrid::Ptr DstSuperSampledUVGrid;

			ProxyLOD::FTaskGroup MapTextureAtlasAndAddTangentSpaceTaskGroup;


			// -- Map the texture atlas texels to triangles on the Simplified Geometry --
			// 1) Map at final resolution
			// 2) Map at supper sample resolution
			{
				// 1) Rasterize the triangles into a grid of the same resolution as the texture atlas.

				MapTextureAtlasAndAddTangentSpaceTaskGroup.Run(
					[&DstUVGrid, &VertexDataMesh, &TextureAtlasDesc]()->void
				{
					DstUVGrid = ProxyLOD::RasterizeTriangles(VertexDataMesh, TextureAtlasDesc, 2/*padding*/);
				}
				);

				// 2) Generate a UV Grid used to map the mesh UVs to texels.

				MapTextureAtlasAndAddTangentSpaceTaskGroup.Run(
					[&VertexDataMesh, &TextureAtlasDesc, &DstSuperSampledUVGrid]()->void
				{
					const int32 SuperSampleNum = 16;

					DstSuperSampledUVGrid = ProxyLOD::RasterizeTriangles(VertexDataMesh, TextureAtlasDesc, 1/*padding*/, SuperSampleNum/* super samples*/);
				}
				);
			}


			// --- Compute the tangent space on the simplified mesh ---
			// Convert to FRawMesh because it has the per-index attribute structure needed for the tangent space.
			// Generate the tangent space, but retain our current normals.

			MapTextureAtlasAndAddTangentSpaceTaskGroup.RunAndWait(
				[&VertexDataMesh, &OutRawMesh]()->void
				{
					const bool bDoMikkT = false;
					const bool bRecomputeNormals = false;
					if (bDoMikkT)
					{
						ProxyLOD::ConvertMesh(VertexDataMesh, OutRawMesh);

						// Compute Mikk-T version of tangent space.  Requires the smoothing groups, and 
						// is slower than the per-vertex  tangent space computation.

						ProxyLOD::ComputeTangentSpace(OutRawMesh, bRecomputeNormals);
					}
					else
					{
						// Per-vertex tangent space computation

						ProxyLOD::ComputeTangentSpace(VertexDataMesh, bRecomputeNormals);

						ProxyLOD::ConvertMesh(VertexDataMesh, OutRawMesh);
					}
				
				}
			);


			// --- Populate the output materials ---
			// --- By mapping the texels to the source geometry.
			{
				// --- Map a correspondence between texels in the texture atlas and locations on the source geometry --

				// The termination distance when we fire rays.  These rays should only be traveling  short 
				// distance between polygons in the simplified geometry and polygons in the source geometry.
				const double BBoxMajorAxisLength = SrcGeometryAdapter.GetBBox().extents()[SrcGeometryAdapter.GetBBox().maxExtent()];
				const double MaxRayLength = FMath::Min(5. * VoxelSize, BBoxMajorAxisLength);

				// Fire rays from the simplified mesh to the original collection of meshes 
				// to determine a correspondence between the SrcMesh and the Simplified mesh.

				ProxyLOD::FSrcDataGrid::Ptr SuperSampledCorrespondenceGrid =
					ProxyLOD::CreateCorrespondence(SrcGeometryAdapter, VertexDataMesh, *DstSuperSampledUVGrid, this->RayHitOrder, MaxRayLength);

				// just for testing, this should force it to save in world space
				const bool bUseWorldSpaceNormals = (!this->bUseTangentSpace);
				if (bUseWorldSpaceNormals)
				{
					ProxyLOD::ComputeBogusNormalTangentAndBiTangent(VertexDataMesh);
					ProxyLOD::ConvertMesh(VertexDataMesh, OutRawMesh);
				}

				// --- Use the correspondence between texels in the texture atlas and simplified geometry ---
				// --- and the correspondence between the same texels and the source geometry            ---
				// --- to generate flattened materials for the simplified geometry                       ---
				// Compute the baked-down maps

				ProxyLOD::MapFlattenMaterials(OutRawMesh, SrcGeometryAdapter, *SuperSampledCorrespondenceGrid, *DstSuperSampledUVGrid, *DstUVGrid, *BakedMaterials, OutMaterial);



				// Transfer the vertex colors unless we want to display the Charts as vertex colors for debugging

				bool bTransferVertexColors = (!this->bChartColorVerts);
				if (bTransferVertexColors)
				{
					ProxyLOD::TransferVertexColors(*SrcGeometryPolyField, OutRawMesh);
				}
			}
		}
		else  // UV-generation failed! 
		{
			bProxyGenerationSuccess = false;

			UE_LOG(LogProxyLODMeshReduction, Warning, TEXT("UV Generation failed."));

			// --- Add default UVs and tangents to the mesh and add a Red Diffuse material ---

			// Convert the mesh with bogus UVs and tangents.

			ProxyLOD::ConvertMesh(VertexDataMesh, OutRawMesh);

			// Generate default out-textures.  Color the failed mesh red.
			
			const FIntPoint DstSize      = OutMaterial.GetPropertySize(EFlattenMaterialProperties::Diffuse);
			TArray<FColor>& TargetBuffer = OutMaterial.GetPropertySamples(EFlattenMaterialProperties::Diffuse);
			ResizeArray(TargetBuffer, DstSize.X * DstSize.Y);
			for (int32 i = 0; i < DstSize.X * DstSize.Y; ++i) TargetBuffer[i] = FColor::Red;
		}
	
	}

	// testing 
	bProxyGenerationSuccess = bProxyGenerationSuccess && (!this->bRemeshOnly);

	if (bProxyGenerationSuccess)
	{

		// Revert the smoothing group to a default.

		for (int32 i = 0; i < OutRawMesh.FaceSmoothingMasks.Num(); ++i)
		{
			OutRawMesh.FaceSmoothingMasks[i] = 1;
		}

		// Debuging

		checkSlow(OutRawMesh.IsValid());

		// Done with the material baking, free the delegate

		if (BakeMaterialsDelegate.IsBound())
		{
			BakeMaterialsDelegate.Unbind();
		}



		// NB: FProxyGenerationProcessor::ProxyGenerationComplete

		CompleteDelegate.ExecuteIfBound(OutRawMesh, OutMaterial, InJobGUID);
	}
	else
	{
		FailedDelegate.ExecuteIfBound(InJobGUID, TEXT("ProxyLOD UV Generation failed"));
	}
}

#undef LOCTEXT_NAMESPACE