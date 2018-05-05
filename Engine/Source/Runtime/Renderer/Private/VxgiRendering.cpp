// Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.


// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI

#include "VxgiRendering.h"
#include "RendererPrivate.h"
#include "DeferredShadingRenderer.h"
#include "ScreenRendering.h"
#include "SceneFilterRendering.h"
#include "SceneUtils.h"
#include "EngineUtils.h"
#include "Engine/VxgiAnchor.h"
#include "Engine/AreaLightActor.h"
#include "Engine/TextureCube.h"
#include "PipelineStateCache.h"
#include "Components/StaticMeshComponent.h"

#define VXGI_EXPOSE_INTERNAL_PARAMETERS 0

static TAutoConsoleVariable<int32> CVarVxgiMapSizeX(
	TEXT("r.VXGI.MapSizeX"),
	128,
	TEXT("Size of every VXGI clipmap level in the X dimension, in voxels.\n")
	TEXT("Valid values are 16, 32, 64, 128, 256."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarVxgiMapSizeY(
	TEXT("r.VXGI.MapSizeY"),
	128,
	TEXT("Size of every VXGI clipmap level in the Y dimension, in voxels.\n")
	TEXT("Valid values are 16, 32, 64, 128, 256."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarVxgiMapSizeZ(
	TEXT("r.VXGI.MapSizeZ"),
	128,
	TEXT("Size of every VXGI clipmap level in Z dimension, in voxels.\n")
	TEXT("Valid values are 16, 32, 64, 128, 256."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarVxgiStackLevels(
	TEXT("r.VXGI.StackLevels"),
	5,
	TEXT("Number of clip stack levels in VXGI clipmap (1-5)."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarVxgiEmittanceStorageScale(
	TEXT("r.VXGI.EmittanceStorageScale"),
	1.0f,
	TEXT("Multiplier for the values stored in VXGI emittance textures (any value greater than 0).\n")
	TEXT("If you observe emittance clamping (e.g. white voxels on colored objects)\n") 
	TEXT("or quantization (color distortion in dim areas), try to change this parameter."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarVxgiHighQualityEmittanceDownsamplingEnable(
	TEXT("r.VXGI.HighQualityEmittanceDownsamplingEnable"),
	0,
	TEXT("Whether to use a larger triangular filter for emittance downsampling.\n")
	TEXT("This filter improves stability of indirect lighting caused by moving objects, but has a negative effect on performance.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarVxgiDiffuseTracingEnable(
	TEXT("r.VXGI.DiffuseTracingEnable"),
	1,
	TEXT("Whether to enable VXGI indirect lighting.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarVxgiSpecularTracingEnable(
	TEXT("r.VXGI.SpecularTracingEnable"),
	1,
	TEXT("Whether to enable VXGI reflections.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarVxgiTemporalReprojectionEnable(
	TEXT("r.VXGI.TemporalReprojectionEnable"),
	1,
	TEXT("Whether to enable temporal reprojection.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarVxgiDiffuseStereoReprojectionEnable(
	TEXT("r.VXGI.DiffuseStereoReprojectionEnable"),
	1,
	TEXT("Whether to use stereo reprojection for VXGI diffuse tracing.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarVxgiSpecularStereoReprojectionEnable(
	TEXT("r.VXGI.SpecularStereoReprojectionEnable"),
	1,
	TEXT("Whether to use stereo reprojection for VXGI specular tracing.\n")
	TEXT("Enabling this mode will make all VXGI reflections computed for the middle eye position, with no stereo disparity.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarVxgiSpecularRoughnessScale(
	TEXT("r.VXGI.SpecularRoughnessScale"),
	1.0f,
	TEXT("Multiplier for roughness that's applied before VXGI specular tracing.\n")
	TEXT("Use this as a knob to match the material look with other lighting techniques."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarVxgiSpecularMinRoughness(
	TEXT("r.VXGI.SpecularMinRoughness"),
	0.0f,
	TEXT("Minimum material roughness for which to perform VXGI specular tracing."),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarVxgiMultiBounceEnable(
	TEXT("r.VXGI.MultiBounceEnable"),
	1,
	TEXT("Whether to enable multi-bounce diffuse VXGI.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarVxgiMultiBounceNormalizationEnable(
	TEXT("r.VXGI.MultiBounceNormalizationEnable"),
	1,
	TEXT("Whether to try preventing the indirect irradiance from blowing up exponentially due to high feedback.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarVxgiVoxelSize(
	TEXT("r.VXGI.VoxelSize"),
	8.0f,
	TEXT("Size of the smallest voxels, in world units."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarVxgiViewOffsetScale(
	TEXT("r.VXGI.ViewOffsetScale"),
	1.f,
	TEXT("Scale factor for the distance between the camera and the VXGI clipmap anchor point"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarVxgiEmissiveMaterialsEnable(
	TEXT("r.VXGI.EmissiveMaterialsEnable"),
	1,
	TEXT("Whether to include emissive materials in the VXGI voxelized emittance.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarVxgiEmittanceShadingMode(
	TEXT("r.VXGI.EmittanceShadingMode"),
	0,
	TEXT("0: Use DiffuseColor = BaseColor * (1 - Metallic)")
	TEXT("1: Use DiffuseColor = BaseColor"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarVxgiEmittanceShadowEnable(
	TEXT("r.VXGI.EmittanceShadowEnable"),
	1,
	TEXT("[Debug] Whether to enable the emittance shadow term.\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarVxgiEmittanceShadowQuality(
	TEXT("r.VXGI.EmittanceShadowQuality"),
	1,
	TEXT("0: no filtering\n")
	TEXT("1: 2x2 samples"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarVxgiDebugClipmapLevel(
	TEXT("r.VXGI.DebugClipmapLevel"),
	-1,
	TEXT("Current clipmap level visualized (for the opacity and emittance debug modes).\n")
	TEXT("-1: visualize all levels at once"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarVxgiCompositingMode(
	TEXT("r.VXGI.CompositingMode"),
	0,
	TEXT("0: add the VXGI diffuse result over the UE lighting using additive blending (default)\n")
	TEXT("1: visualize the VXGI indirect lighting only, with no albedo and no AO\n")
	TEXT("2: visualize the direct lighting only"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarVxgiAmbientOcclusionMode(
	TEXT("r.VXGI.AmbientOcclusionMode"),
	0,
	TEXT("0: Default\n")
	TEXT("1: Replace lighting with Voxel AO"),
	ECVF_Default);

static TAutoConsoleVariable<int32> CVarVxgiAreaLightsEnable(
	TEXT("r.VXGI.AreaLightsEnable"),
	3,
	TEXT("Whether to enable VXGI area lights.\n")
	TEXT("0: Disable, 1: Enable, no occlusion; 2: Enable, voxel occlusion;\n")
	TEXT("3: Enable, voxel + screen-space occlusion"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarVxgiAreaLightMinRoughness(
	TEXT("r.VXGI.AreaLightMinRoughness"),
	0.0f,
	TEXT("Minimum material roughness for which to calculate area light specular lighting."),
	ECVF_Default);

#if VXGI_EXPOSE_INTERNAL_PARAMETERS

static TAutoConsoleVariable<float> CVarVxgiInternalParamX(
	TEXT("r.VXGI._X"),
	0,
	TEXT("VXGI development knob. Has no effect on normal builds."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarVxgiInternalParamY(
	TEXT("r.VXGI._Y"),
	0,
	TEXT("VXGI development knob. Has no effect on normal builds."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarVxgiInternalParamZ(
	TEXT("r.VXGI._Z"),
	0,
	TEXT("VXGI development knob. Has no effect on normal builds."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarVxgiInternalParamW(
	TEXT("r.VXGI._W"),
	0,
	TEXT("VXGI development knob. Has no effect on normal builds."),
	ECVF_Default);

#endif

static FBox FBoxFromBox3f(const VXGI::Box3f& VxgiBox)
{
	return FBox(
		FVector(VxgiBox.lower.x, VxgiBox.lower.y, VxgiBox.lower.z),
		FVector(VxgiBox.upper.x, VxgiBox.upper.y, VxgiBox.upper.z));
}

uint64_t GetVxgiShaderHash()
{
	static uint64_t VxgiShaderHash = 0;

	if (VxgiShaderHash == 0)
	{
		FWindowsPlatformMisc::LoadVxgiModule(); // Might not be loaded with a no-op RHI
		auto Status = VXGI::VFX_VXGI_VerifyInterfaceVersion();
		check(VXGI_SUCCEEDED(Status));
		VxgiShaderHash = VXGI::VFX_VXGI_GetInternalShaderHash();
		FWindowsPlatformMisc::UnloadVxgiModule(); // Do we want to bother unloading? Might be slower if we get called a bunch of times
	}

	return VxgiShaderHash;
}

FSHAHash AmendHashWithVxgiHash(const FSHAHash& FileHash)
{
	FSHA1 HashState;
	HashState.Update(&FileHash.Hash[0], sizeof(FileHash.Hash));

	uint64_t VXGIHash = GetVxgiShaderHash();
	HashState.Update((uint8*)&VXGIHash, sizeof(VXGIHash));
	HashState.Final();

	FSHAHash Result;
	HashState.GetHash(&Result.Hash[0]);
	return Result;
}

const FSHAHash& FVxgiGlobalShaderType::GetSourceHash() const
{
	if (!bHashInitialized)
	{
		AmendedSourceHash = AmendHashWithVxgiHash(FGlobalShaderType::GetSourceHash());
		bHashInitialized = true;
	}
	return AmendedSourceHash;
}

const FSHAHash& FVxgiMeshMaterialShaderType::GetSourceHash() const
{
	if (!bHashInitialized)
	{
		AmendedSourceHash = AmendHashWithVxgiHash(FMeshMaterialShaderType::GetSourceHash());
		bHashInitialized = true;
	}
	return AmendedSourceHash;
}

class FVxgiGBufferAccessShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FVxgiGBufferAccessShader, VxgiGlobal);

	/** Default constructor. */
	FVxgiGBufferAccessShader() {}

public:
	struct GBufferParameters
	{
		FMatrix     viewProjMatrix;
		FMatrix     viewProjMatrixInv;
		FMatrix     viewMatrixInv;
		FVector4    cameraPosition;

		FVector2D   viewportOrigin;
		FVector2D   viewportSize;

		FVector2D   viewportSizeInv;
		float       projectionA;
		float       projectionB;
	};

	static const int MaxViews = 4;

	// Layout must match the constant buffer in VXGIGBufferAccess.usf
	__declspec(align(16))
	struct ConstantBufferData
	{
		GBufferParameters g_GBuffer[MaxViews];
		GBufferParameters g_PreviousGBuffer[MaxViews];

		float g_DiffuseIrradianceScale;
		float g_DiffuseTracingStep;
		float g_DiffuseOpacityCorrectionFactor;
		float g_AmbientRange;
		float g_AmbientScale;
		float g_AmbientBias;
		float g_AmbientPower;
		float g_DiffuseInitialOffsetBias;
		float g_DiffuseInitialOffsetDistanceFactor;

		float g_SpecularIrradianceScale;
		float g_SpecularTracingStep;
		float g_SpecularOpacityCorrectionFactor;
		float g_SpecularInitialOffsetBias;
		float g_SpecularInitialOffsetDistanceFactor;
		float g_SpecularRoughnessScale;
		float g_SpecularMinRoughness;

		float g_AreaLightTracingStep;
		float g_AreaLightOpacityCorrectionFactor;
		float g_AreaLightInitialOffsetBias;
		float g_AreaLightInitialOffsetDistanceFactor;
		float g_AreaLightMinRoughness;
	};

	NVRHI::ConstantBufferHandle ConstantBuffer = NULL;

	FVxgiGBufferAccessShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{ }

	~FVxgiGBufferAccessShader()
	{
		if (ConstantBuffer != NULL)
		{
			NVRHI::IRendererInterface* RI = GDynamicRHI->RHIVXGIGetRendererInterface();

			if (RI)
			{
				RI->destroyConstantBuffer(ConstantBuffer);
			}

			ConstantBuffer = NULL;
		}
	}

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Params)
	{
		return IsFeatureLevelSupported(Params.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		// A flag to D3D11ShaderCompiler that requests special processing
		OutEnvironment.SetDefine(TEXT("VXGI_GBUFFER_ACCESS"), 1);
	}
};

IMPLEMENT_SHADER_TYPE(, FVxgiGBufferAccessShader, TEXT("/Engine/VXGIGBufferAccess.usf"), TEXT("DummyPS"), SF_Pixel);

void FSceneRenderer::InitVxgiView()
{
	if (!IsVxgiEnabled())
		return;

	FSceneViewInitOptions ViewInitOptions;
	ViewInitOptions.ViewFamily = &ViewFamily;
	ViewInitOptions.SetViewRectangle(FIntRect(0, 0, 1, 1));

	VXGI::VoxelizationParameters FullVoxelizationParams;
	SetVxgiVoxelizationParameters(FullVoxelizationParams);

	VXGI::ComputeVoxelizationViewParametersInput VoxelizationParams;
	VoxelizationParams.clipmapAnchor = VXGI::float3(VxgiAnchorPoint.X, VxgiAnchorPoint.Y, VxgiAnchorPoint.Z);
	VoxelizationParams.finestVoxelSize = VxgiVoxelSize;
	VoxelizationParams.allocationMapLodBias = FullVoxelizationParams.allocationMapLodBias;
	VoxelizationParams.mapSize = FullVoxelizationParams.mapSize;
	VoxelizationParams.stackLevels = FullVoxelizationParams.stackLevels;

	VXGI::VoxelizationViewParameters ViewParams;
	VXGI::Status::Enum Status = VXGI::VFX_VXGI_ComputeVoxelizationViewParameters(VoxelizationParams, ViewParams);
	check(VXGI_SUCCEEDED(Status));

	VxgiClipmapBounds = FBoxSphereBounds(FBoxFromBox3f(ViewParams.clipmapExtents));

	ViewInitOptions.ProjectionMatrix = *(FMatrix*)&ViewParams.projectionMatrix;
	ViewInitOptions.ViewOrigin = FVector(0.f);
	ViewInitOptions.ViewRotationMatrix = *(FMatrix*)&ViewParams.viewMatrix;

	VxgiView = new FViewInfo(ViewInitOptions);
	VxgiView->ViewRect = ViewInitOptions.GetViewRect();

	// Setup the prev matrices for particle system factories
	VxgiView->PrevViewMatrices = VxgiView->ViewMatrices;
	VxgiView->bPrevTransformsReset = true;

	VxgiView->VxgiClipmapBounds = VxgiClipmapBounds;
	VxgiView->VxgiNumClipmapLevels = VoxelizationParams.stackLevels;
	VxgiView->AntiAliasingMethod = AAM_None; //Turn off temporal AA jitter
	VxgiView->bIsVxgiVoxelization = true;
	VxgiView->bDisableDistanceBasedFadeTransitions = true;
	VxgiView->VxgiVoxelizationPass = EVxgiVoxelizationPass::NONE;
}

VXGI::float3 VectorToVxgi(const FVector& V)
{
	return VXGI::float3(V.X, V.Y, V.Z);
}

VXGI::float3 ColorToVxgi(const FLinearColor& C)
{
	return VXGI::float3(C.R, C.G, C.B);
}

uint32 NormalizeVxgiMapSize(uint32 Size)
{
	// Round down to the nearest power of 2, keep the result within [32-256] range
	if (Size < 64) return 32;
	if (Size < 128) return 64;
	if (Size < 256) return 128;
	return 256;
}

void FSceneRenderer::InitVxgiRenderingState(const FSceneViewFamily* InViewFamily)
{
	bVxgiPerformOpacityVoxelization = false;
	bVxgiPerformEmittanceVoxelization = false;

	//This must be done on the game thread
	const auto& PrimaryView = *(InViewFamily->Views[0]);
	bVxgiDebugRendering = ViewFamily.EngineShowFlags.VxgiOpacityVoxels || ViewFamily.EngineShowFlags.VxgiEmittanceVoxels || ViewFamily.EngineShowFlags.VxgiIrradianceVoxels;
	VxgiVoxelSize = CVarVxgiVoxelSize.GetValueOnGameThread();

	VxgiVoxelSize = FMath::Max(VxgiVoxelSize, 0.1f);

	float FinestLevelSize = VxgiVoxelSize * FMath::Min3(
		NormalizeVxgiMapSize(CVarVxgiMapSizeX.GetValueOnGameThread()),
		NormalizeVxgiMapSize(CVarVxgiMapSizeY.GetValueOnGameThread()),
		NormalizeVxgiMapSize(CVarVxgiMapSizeZ.GetValueOnGameThread()));

	VxgiAnchorPoint = PrimaryView.ViewMatrices.GetViewOrigin() + PrimaryView.GetViewDirection() * FinestLevelSize * CVarVxgiViewOffsetScale.GetValueOnGameThread() * 0.5f;
	
	for (TActorIterator<AVxgiAnchor> Itr(Scene->World); Itr; ++Itr)
	{
		if (Itr->bEnabled)
		{
			VxgiAnchorPoint = Itr->GetActorLocation();
			break;
		}
	}

	VxgiAreaLights.Empty();
    VxgiAreaLightActors.Empty();

	uint32 VxgiAreaLightsEnable = CVarVxgiAreaLightsEnable.GetValueOnGameThread();

	if (VxgiAreaLightsEnable != 0)
	{
		for (TActorIterator<AAreaLightActor> Itr(Scene->World); Itr; ++Itr)
		{
			VXGI::AreaLight Light;
			Light.center = VectorToVxgi(Itr->GetActorLocation());
			Light.majorAxis = VectorToVxgi(Itr->GetActorRightVector()) * Itr->GetActorScale().Y * 50.f;
			Light.minorAxis = VectorToVxgi(Itr->GetActorForwardVector()) * Itr->GetActorScale().X * 50.f;
			Light.color = ColorToVxgi(Itr->LightColor);
			Light.diffuseIntensity = Itr->LightIntensityDiffuse;
			Light.specularIntensity = Itr->LightIntensitySpecular;
			Light.quality = Itr->Quality;
			Light.directionalSamplingRate = Itr->DirectionalSamplingRate;
			Light.maxProjectedArea = Itr->MaxProjectedArea;
			Light.lightSurfaceOffset = Itr->LightSurfaceOffset;
			Light.enableOcclusion = Itr->bEnableShadows && VxgiAreaLightsEnable >= 2;
			Light.enableScreenSpaceOcclusion = Itr->bEnableScreenSpaceShadows && VxgiAreaLightsEnable >= 3;
			Light.screenSpaceOcclusionQuality = Itr->ScreenSpaceShadowQuality;
			Light.temporalReprojectionWeight = Itr->TemporalWeight;
			Light.temporalReprojectionDetailReconstruction = Itr->TemporalDetailReconstruction;
			Light.enableNeighborhoodClamping = Itr->bEnableNeighborhoodColorClamping;
			Light.neighborhoodClampingWidth = Itr->NeighborhoodClampingWidth;
			Light.identifier = *Itr;
			Light.texturedShadows = Itr->bTexturedShadows;
			Light.highQualityTextureFilter = Itr->bHighQualityTextureFilter;
			Light.attenuationRadius = Itr->AttenuationRadius;
			Light.transitionLength = Itr->TransitionLength;

			if (Light.color.lengthSq() == 0 || Light.diffuseIntensity <= 0 && Light.specularIntensity <= 0 || !Itr->bEnableLight)
				continue;

			VxgiAreaLights.Add(Light);
			VxgiAreaLightActors.Add(*Itr);
		}
	}

	bVxgiUseEmissiveMaterials = !!CVarVxgiEmissiveMaterialsEnable.GetValueOnGameThread();
	bVxgiTemporalReprojectionEnable = !!CVarVxgiTemporalReprojectionEnable.GetValueOnGameThread();
	bVxgiAmbientOcclusionMode = !!CVarVxgiAmbientOcclusionMode.GetValueOnGameThread() || (GDynamicRHI->RHIGetVXGITier() == EVxgiTier::OcclusionOnly);
	bVxgiMultiBounceEnable = !bVxgiAmbientOcclusionMode && !!CVarVxgiMultiBounceEnable.GetValueOnGameThread() && PrimaryView.FinalPostProcessSettings.VxgiMultiBounceEnabled;

	bVxgiSkyLightEnable = !bVxgiAmbientOcclusionMode
		&& Scene->SkyLight
		&& Scene->SkyLight->ProcessedTexture
		&& !Scene->SkyLight->LightColor.IsAlmostBlack()
		&& ViewFamily.EngineShowFlags.SkyLighting
		&& Scene->SkyLight->bCastVxgiIndirectLighting;
}

bool FSceneRenderer::IsVxgiEnabled(const FViewInfo& View)
{
	if (GDynamicRHI->RHIGetVXGITier() == EVxgiTier::None)
	{
		return false;
	}

	if (IsForwardShadingEnabled(FeatureLevel))
	{
		return false; // VXGI is incompatible with forward shading
	}

	if (!View.State && !View.bEnableVxgiForSceneCapture)
	{
		return false; //some editor panel or something
	}

	if (!View.IsPerspectiveProjection())
	{
		return false;
	}

	if (View.bIsSceneCapture && !View.bEnableVxgiForSceneCapture || View.bIsReflectionCapture || View.bIsPlanarReflection)
	{
		return false;
	}

	if (View.Family->ViewMode != VMI_Lit
		&& View.Family->ViewMode != VMI_Lit_DetailLighting
		&& View.Family->ViewMode != VMI_VxgiEmittanceVoxels
		&& View.Family->ViewMode != VMI_VxgiOpacityVoxels
		&& View.Family->ViewMode != VMI_VxgiIrradianceVoxels
		&& View.Family->ViewMode != VMI_ReflectionOverride
		&& View.Family->ViewMode != VMI_VisualizeBuffer)
	{
		return false;
	}
	
	if (!View.Family->EngineShowFlags.VxgiDiffuse && !View.Family->EngineShowFlags.VxgiSpecular)
	{
		return false;
	}
	
	if (bVxgiDebugRendering)
	{
		return true;
	}

	const auto& PostSettings = View.FinalPostProcessSettings;
	return bVxgiAmbientOcclusionMode
		? PostSettings.bVxgiAmbientOcclusionEnabled || PostSettings.VxgiAreaLightsEnabled
		: PostSettings.VxgiDiffuseTracingEnabled || PostSettings.VxgiSpecularTracingEnabled || PostSettings.VxgiAreaLightsEnabled;
}

bool FSceneRenderer::IsVxgiEnabled()
{
	check(Views.Num() > 0);
	const FViewInfo& PrimaryView = Views[0];
	return IsVxgiEnabled(PrimaryView);
}

void FSceneRenderer::SetVxgiVoxelizationParameters(VXGI::VoxelizationParameters& Params)
{
	Params.mapSize = VXGI::uint3(
		NormalizeVxgiMapSize(CVarVxgiMapSizeX.GetValueOnAnyThread()), 
		NormalizeVxgiMapSize(CVarVxgiMapSizeY.GetValueOnAnyThread()),
		NormalizeVxgiMapSize(CVarVxgiMapSizeZ.GetValueOnAnyThread()));
	
	Params.stackLevels = CVarVxgiStackLevels.GetValueOnAnyThread();
	Params.stackLevels = FMath::Max(1u, FMath::Min(5u, Params.stackLevels));

	Params.allocationMapLodBias = uint32(FMath::Max(2 - int(Params.stackLevels), (Params.mapSize.vmax() == 256) ? 1 : 0));
	Params.indirectIrradianceMapLodBias = Params.allocationMapLodBias;
	Params.mipLevels = log2(Params.mapSize.vmin()) - 2;
	Params.persistentVoxelData = false;
    Params.ambientOcclusionMode = bVxgiAmbientOcclusionMode;
	Params.emittanceStorageScale = FMath::Max(1e-10f, CVarVxgiEmittanceStorageScale.GetValueOnAnyThread());
	Params.useEmittanceInterpolation = true;
	Params.useHighQualityEmittanceDownsampling = !!CVarVxgiHighQualityEmittanceDownsamplingEnable.GetValueOnAnyThread();
	Params.enableMultiBounce = bVxgiMultiBounceEnable;

	// Workaround for a voxelization issue on Volta GPUs that incorrectly uses the wave match operation
	Params.enabledHardwareFeatures = VXGI::HardwareFeatures::Enum(VXGI::HardwareFeatures::ALL & ~VXGI::HardwareFeatures::NVAPI_WAVE_MATCH);
}

void FSceneRenderer::PrepareForVxgiVoxelization(FRHICommandList& RHICmdList)
{
	VXGI::IGlobalIllumination* VxgiInterface = GDynamicRHI->RHIVXGIGetInterface();
	auto Status = VXGI::VFX_VXGI_VerifyInterfaceVersion();
	check(VXGI_SUCCEEDED(Status));

#if VXGI_EXPOSE_INTERNAL_PARAMETERS
	VxgiInterface->setInternalParameters(VXGI::float4(
		CVarVxgiInternalParamX.GetValueOnRenderThread(), 
		CVarVxgiInternalParamY.GetValueOnRenderThread(),
		CVarVxgiInternalParamZ.GetValueOnRenderThread(),
		CVarVxgiInternalParamW.GetValueOnRenderThread()));
#endif

	VXGI::UpdateVoxelizationParameters Parameters;
	Parameters.clipmapAnchor = VXGI::float3(VxgiAnchorPoint.X, VxgiAnchorPoint.Y, VxgiAnchorPoint.Z);
	Parameters.sceneExtents = GetVxgiWorldSpaceSceneBounds();
	Parameters.finestVoxelSize = VxgiVoxelSize;
	Parameters.indirectIrradianceMapTracingParameters.irradianceScale = Views[0].FinalPostProcessSettings.VxgiMultiBounceFeedbackGain;
	Parameters.indirectIrradianceMapTracingParameters.useAutoNormalization = !!CVarVxgiMultiBounceNormalizationEnable.GetValueOnRenderThread();

	switch (Views[0].FinalPostProcessSettings.VxgiMultiBounceLightLeaking)
	{
    case VXGILLM_Minimal: Parameters.indirectIrradianceMapTracingParameters.lightLeakingAmount = VXGI::LightLeakingAmount::MINIMAL; break;
	case VXGILLM_Moderate: Parameters.indirectIrradianceMapTracingParameters.lightLeakingAmount = VXGI::LightLeakingAmount::MODERATE; break;
	case VXGILLM_Heavy: Parameters.indirectIrradianceMapTracingParameters.lightLeakingAmount = VXGI::LightLeakingAmount::HEAVY; break;
	}

	Status = VxgiInterface->prepareForVoxelization(
		Parameters,
		bVxgiPerformOpacityVoxelization,
		bVxgiPerformEmittanceVoxelization);

	check(VXGI_SUCCEEDED(Status));
}

struct FCompareFProjectedShadowInfoBySplitNear
{
	inline bool operator()(const FProjectedShadowInfo& A, const FProjectedShadowInfo& B) const
	{
		return A.CascadeSettings.SplitNear < B.CascadeSettings.SplitNear;
	}
};

bool FSceneRenderer::InitializeVxgiVoxelizationParameters()
{	
	// Fill the voxelization params structure to latch the console vars, specifically AmbientOcclusionMode
	SetVxgiVoxelizationParameters(VxgiVoxelizationParameters);

	if (!IsVxgiEnabled())
	{
		return false;
	}

	// Reset the VxgiLastVoxelizationPass values for all primitives
	for (int32 Index = 0; Index < Scene->Primitives.Num(); ++Index)
	{
		FPrimitiveSceneInfo* PrimitiveSceneInfo = Scene->Primitives[Index];
		PrimitiveSceneInfo->VxgiLastVoxelizationPass = EVxgiVoxelizationPass::NONE;
	}

	GDynamicRHI->RHIVXGISetVoxelizationParameters(VxgiVoxelizationParameters);

    return GDynamicRHI->RHIVXGIIsInitialized();
}

void FSceneRenderer::RenderVxgiVoxelization(FRHICommandList& RHICmdList)
{
	SCOPED_DRAW_EVENT(RHICmdList, VXGIVoxelization);

	{
		SCOPED_DRAW_EVENT(RHICmdList, PrepareForVoxelization);

		PrepareForVxgiVoxelization(RHICmdList);
	}

	if (bVxgiAmbientOcclusionMode)
	{
		SCOPED_DRAW_EVENT(RHICmdList, VoxelizationOpacity);

		FVxgiVoxelizationArgs Args;
		RenderVxgiVoxelizationPass(RHICmdList, EVxgiVoxelizationPass::ONLY_OPACITY, Args);
	}
	else
	{
		for (TSparseArray<FLightSceneInfoCompact>::TConstIterator LightIt(Scene->Lights); LightIt; ++LightIt)
		{
			const FLightSceneInfoCompact& LightSceneInfoCompact = *LightIt;
			const FLightSceneInfo* const LightSceneInfo = LightSceneInfoCompact.LightSceneInfo;

			if (LightSceneInfo->ShouldRenderLightViewIndependent() &&
				LightSceneInfo->Proxy->CastVxgiIndirectLighting(nullptr) &&
				LightSceneInfo->Proxy->AffectsBounds(VxgiClipmapBounds))
			{
				FVisibleLightInfo& VisibleLightInfo = VisibleLightInfos[LightSceneInfo->Id];

				FString LightNameWithLevel;
				GetLightNameForDrawEvent(LightSceneInfo->Proxy, LightNameWithLevel);
				SCOPED_DRAW_EVENTF(RHICmdList, VoxelizationForLight, *LightNameWithLevel);

				FVxgiVoxelizationArgs Args;
				Args.LightSceneInfo = LightSceneInfo;

				bool bAnyWholeSceneShadows = false;

				for (FProjectedShadowInfo* Shadow : VisibleLightInfo.ShadowsToProject)
				{
					if (Shadow->RenderTargets.DepthTarget && Shadow->bWholeSceneShadow)
					{
						bAnyWholeSceneShadows = true;
						break;
					}
				}

				for (FProjectedShadowInfo* Shadow : VisibleLightInfo.ShadowsToProject)
				{
					if (Shadow->RenderTargets.DepthTarget && (!bAnyWholeSceneShadows || Shadow->bWholeSceneShadow))
						Args.Shadows.Add(Shadow);
				}

				Args.Shadows.Sort(FCompareFProjectedShadowInfoBySplitNear());
				RenderVxgiVoxelizationPass(RHICmdList, EVxgiVoxelizationPass::LIGHT0 + LightSceneInfo->Id, Args);
			}
		}

		{
			SCOPED_DRAW_EVENT(RHICmdList, VoxelizationNonLight);

			FVxgiVoxelizationArgs Args;
			RenderVxgiVoxelizationPass(RHICmdList, EVxgiVoxelizationPass::FINALIZATION, Args);
		}

		{
			SCOPED_DRAW_EVENT(RHICmdList, VoxelizationExtraOpacity);

			FVxgiVoxelizationArgs Args;
			RenderVxgiVoxelizationPass(RHICmdList, EVxgiVoxelizationPass::EXTRA_OPACITY, Args);
		}
	}

	{
		SCOPED_DRAW_EVENT(RHICmdList, FinalizeVoxelization);

		auto Status = GDynamicRHI->RHIVXGIGetInterface()->finalizeVoxelization();
		check(VXGI_SUCCEEDED(Status));

		ViewFamily.bVxgiAvailable = true;
	}
}

void SetVxgiDiffuseTracingParameters(const FViewInfo& View, VXGI::DiffuseTracingParameters &TracingParams, bool bEnableTemporalReprojection, bool bEnableStereoReprojection)
{
	const auto& PostSettings = View.FinalPostProcessSettings;

	TracingParams.quality = PostSettings.VxgiDiffuseTracingQuality;
	TracingParams.directionalSamplingRate = PostSettings.VxgiDiffuseTracingDirectionalSamplingRate;
	TracingParams.softness = PostSettings.VxgiDiffuseTracingSoftness;
	TracingParams.enableTemporalReprojection = !!PostSettings.bVxgiDiffuseTracingTemporalReprojectionEnabled && bEnableTemporalReprojection;
	TracingParams.enableTemporalJitter = TracingParams.enableTemporalReprojection;
	TracingParams.temporalReprojectionWeight = PostSettings.VxgiDiffuseTracingTemporalReprojectionPreviousFrameWeight;
	TracingParams.temporalReprojectionMaxDistanceInVoxels = PostSettings.VxgiDiffuseTracingTemporalReprojectionMaxDistanceInVoxels;
	TracingParams.temporalReprojectionNormalWeightExponent = PostSettings.VxgiDiffuseTracingTemporalReprojectionNormalWeightExponent;
	TracingParams.temporalReprojectionDetailReconstruction = PostSettings.VxgiDiffuseTracingTemporalReprojectionDetailReconstruction;
	TracingParams.enableViewReprojection = bEnableStereoReprojection;

	switch (PostSettings.VxgiDiffuseTracingResolution)
	{
	case VXGIDTR_Half: TracingParams.tracingResolution = VXGI::TracingResolution::HALF; break;
	case VXGIDTR_Third: TracingParams.tracingResolution = VXGI::TracingResolution::THIRD; break;
	case VXGIDTR_Quarter: TracingParams.tracingResolution = VXGI::TracingResolution::QUARTER; break;
	}

	switch (PostSettings.VxgiDiffuseLightLeaking)
	{
	case VXGILLM_Minimal: TracingParams.lightLeakingAmount = VXGI::LightLeakingAmount::MINIMAL; break;
	case VXGILLM_Moderate: TracingParams.lightLeakingAmount = VXGI::LightLeakingAmount::MODERATE; break;
	case VXGILLM_Heavy: TracingParams.lightLeakingAmount = VXGI::LightLeakingAmount::HEAVY; break;
	}
}

void SetVxgiSpecularTracingParameters(const FViewInfo& View, VXGI::SpecularTracingParameters &TracingParams, bool bEnableTemporalReprojection, bool bEnableStereoReprojection, float ViewSimilarity)
{
	const auto& PostSettings = View.FinalPostProcessSettings;

	if (PostSettings.bVxgiSpecularTracingTemporalFilterEnabled && bEnableTemporalReprojection)
	{
		TracingParams.enableTemporalJitter = true;
		TracingParams.filter = VXGI::SpecularTracingParameters::FILTER_TEMPORAL;
	}
	else
	{
		TracingParams.enableTemporalJitter = false;
		TracingParams.filter = VXGI::SpecularTracingParameters::FILTER_SIMPLE;
	}

	TracingParams.enableConeJitter = PostSettings.bVxgiSpecularTracingConeJitterEnabled;
	TracingParams.enableViewReprojection = bEnableStereoReprojection;
	TracingParams.perPixelRandomOffsetScale = 0.5f;

	TracingParams.temporalReprojectionWeight = PostSettings.VxgiSpecularTracingTemporalReprojectionPreviousFrameWeight * ViewSimilarity;
}

void SetVxgiAreaLightTracingParameters(const FViewInfo& View, VXGI::AreaLightTracingParameters &TracingParams, bool bEnableTemporalReprojection)
{
	const auto& PostSettings = View.FinalPostProcessSettings;

	TracingParams.enableTemporalReprojection = !!PostSettings.bVxgiAreaLightTemporalReprojectionEnabled && bEnableTemporalReprojection;
	TracingParams.enableTemporalJitter = TracingParams.enableTemporalReprojection;
	TracingParams.temporalReprojectionMaxDistanceInVoxels = PostSettings.VxgiAreaLightTemporalReprojectionMaxDistanceInVoxels;
	TracingParams.temporalReprojectionNormalWeightExponent = PostSettings.VxgiAreaLightTemporalReprojectionNormalWeightExponent;

	switch (PostSettings.VxgiAreaLightTracingResolution)
	{
	case VXGIDTR_Half: TracingParams.tracingResolution = VXGI::TracingResolution::HALF; break;
	case VXGIDTR_Third: TracingParams.tracingResolution = VXGI::TracingResolution::THIRD; break;
	case VXGIDTR_Quarter: TracingParams.tracingResolution = VXGI::TracingResolution::QUARTER; break;
	}
}

void FillVxgiConstantBuffer(const TArray<FViewInfo>& Views, int32 NumViewsToProcess, uint32 CurrentGPU, FVxgiGBufferAccessShader::ConstantBufferData& Constants)
{
	for (int32 ViewIndex = 0; ViewIndex < NumViewsToProcess; ViewIndex++)
	{
		const FViewInfo& View = Views[ViewIndex];

		FVxgiGBufferAccessShader::GBufferParameters& GBuffer = Constants.g_GBuffer[ViewIndex];
		FVxgiGBufferAccessShader::GBufferParameters& PreviousGBuffer = Constants.g_PreviousGBuffer[ViewIndex];

		GBuffer.viewportOrigin = FVector2D(View.ViewRect.Min);
		GBuffer.viewportSize = FVector2D(View.ViewRect.Size());
		GBuffer.viewportSizeInv = FVector2D(1.0f / GBuffer.viewportSize.X, 1.0f / GBuffer.viewportSize.Y);

		const FMatrix& ViewMatrix = View.ViewMatrices.GetViewMatrix();
		const FMatrix& ProjectionMatrix = View.ViewMatrices.GetProjectionMatrix();
		FMatrix ViewProjMatrix = ViewMatrix * ProjectionMatrix;

		GBuffer.viewProjMatrix = ViewProjMatrix;
		GBuffer.viewProjMatrixInv = ViewProjMatrix.Inverse();
		GBuffer.viewMatrixInv = ViewMatrix.Inverse();
		GBuffer.cameraPosition = View.ViewMatrices.GetViewOrigin();
		GBuffer.projectionA = ProjectionMatrix.M[2][2] / ProjectionMatrix.M[2][3];
		GBuffer.projectionB = ProjectionMatrix.M[3][2] / ProjectionMatrix.M[2][3];

		if (View.ViewState)
		{
			FIntRect& PrevViewRect = View.ViewState->PrevViewRectPerGPU[CurrentGPU];
			PreviousGBuffer.viewportOrigin = FVector2D(PrevViewRect.Min);
			PreviousGBuffer.viewportSize = FVector2D(PrevViewRect.Size());
			PreviousGBuffer.viewportSizeInv = FVector2D(1.0f / PreviousGBuffer.viewportSize.X, 1.0f / PreviousGBuffer.viewportSize.Y);

			FViewMatrices& PrevViewMatrices = View.ViewState->PrevViewMatricesPerGPU[CurrentGPU];
			const FMatrix& PrevViewMatrix = PrevViewMatrices.GetViewMatrix();
			const FMatrix& PrevProjectionMatrix = PrevViewMatrices.GetProjectionMatrix();
			FMatrix PrevViewProjMatrix = PrevViewMatrix * PrevProjectionMatrix;

			PreviousGBuffer.viewProjMatrix = PrevViewProjMatrix;
			PreviousGBuffer.viewProjMatrixInv = PrevViewProjMatrix.Inverse();
			PreviousGBuffer.cameraPosition = PrevViewMatrices.GetViewOrigin();
			PreviousGBuffer.projectionA = PrevProjectionMatrix.M[2][2] / PrevProjectionMatrix.M[2][3];
			PreviousGBuffer.projectionB = PrevProjectionMatrix.M[3][2] / PrevProjectionMatrix.M[2][3];

			PrevViewRect = View.ViewRect;
			PrevViewMatrices = View.ViewMatrices;
		}
	}

	FFinalPostProcessSettings PostProcessSettings = Views[0].FinalPostProcessSettings;

	Constants.g_DiffuseIrradianceScale = PostProcessSettings.VxgiDiffuseTracingIntensity;
	Constants.g_DiffuseTracingStep = PostProcessSettings.VxgiDiffuseTracingStep;
	Constants.g_DiffuseOpacityCorrectionFactor = PostProcessSettings.VxgiDiffuseTracingOpacityCorrectionFactor;
	Constants.g_AmbientRange = PostProcessSettings.VxgiAmbientRange;
	Constants.g_AmbientScale = PostProcessSettings.VxgiAmbientScale;
	Constants.g_AmbientBias = PostProcessSettings.VxgiAmbientBias;
	Constants.g_AmbientPower = PostProcessSettings.VxgiAmbientPowerExponent;
	Constants.g_DiffuseInitialOffsetBias = PostProcessSettings.VxgiDiffuseTracingInitialOffsetBias;
	Constants.g_DiffuseInitialOffsetDistanceFactor = PostProcessSettings.VxgiDiffuseTracingInitialOffsetDistanceFactor;

	Constants.g_SpecularIrradianceScale = PostProcessSettings.VxgiSpecularTracingIntensity;
	Constants.g_SpecularTracingStep = PostProcessSettings.VxgiSpecularTracingStep;
	Constants.g_SpecularOpacityCorrectionFactor = PostProcessSettings.VxgiSpecularTracingOpacityCorrectionFactor;
	Constants.g_SpecularInitialOffsetBias = PostProcessSettings.VxgiSpecularTracingInitialOffsetBias;
	Constants.g_SpecularInitialOffsetDistanceFactor = PostProcessSettings.VxgiSpecularTracingInitialOffsetDistanceFactor;
	Constants.g_SpecularRoughnessScale = CVarVxgiSpecularRoughnessScale.GetValueOnRenderThread();
	Constants.g_SpecularMinRoughness = CVarVxgiSpecularMinRoughness.GetValueOnRenderThread();

	Constants.g_AreaLightTracingStep = PostProcessSettings.VxgiAreaLightTracingStep;
	Constants.g_AreaLightOpacityCorrectionFactor = PostProcessSettings.VxgiAreaLightOpacityCorrectionFactor;
	Constants.g_AreaLightInitialOffsetBias = PostProcessSettings.VxgiAreaLightInitialOffsetBias;
	Constants.g_AreaLightInitialOffsetDistanceFactor = PostProcessSettings.VxgiAreaLightInitialOffsetDistanceFactor;
	Constants.g_AreaLightMinRoughness = CVarVxgiAreaLightMinRoughness.GetValueOnRenderThread();
}

NVRHI::TextureHandle GetNVRHITexture2DHandle(IPooledRenderTarget* RenderTarget)
{
	return GDynamicRHI->GetVXGITextureFromRHI(RenderTarget->GetRenderTargetItem().ShaderResourceTexture);
}

void FSceneRenderer::RenderVxgiTracing(FRHICommandListImmediate& RHICmdList)
{
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);
	
	const FViewInfo& PrimaryView = Views[0];
	int32 NumViewsToProcess = FMath::Min(Views.Num(), FVxgiGBufferAccessShader::MaxViews);

	if (!IsVxgiEnabled(PrimaryView) || bVxgiDebugRendering)
	{
		return;
	}

	VXGI::IGlobalIllumination* VxgiInterface = GDynamicRHI->RHIVXGIGetInterface();
	VXGI::IViewTracer* VxgiViewTracer = nullptr;

	bool bVxgiTemporaryTracer = true;
	if (PrimaryView.ViewState != nullptr)
	{
		VxgiViewTracer = PrimaryView.ViewState->VxgiViewTracer;
		bVxgiTemporaryTracer = false;
	}

	// The reinterpret_cast is ugly, but it works.
	TShaderMapRef<FVxgiGBufferAccessShader> GBufferAccessShader(reinterpret_cast<TShaderMap<FVxgiGlobalShaderType>*>(PrimaryView.ShaderMap));

	if (VxgiViewTracer == NULL)
	{
		VXGI::Status::Enum Status = VxgiInterface->createCustomTracer(&VxgiViewTracer, GBufferAccessShader->GetVxgiUserDefinedShaderSet());

		if (VXGI_FAILED(Status))
			return;

		if (!bVxgiTemporaryTracer)
			PrimaryView.ViewState->VxgiViewTracer = VxgiViewTracer;
	}

	NVRHI::IRendererInterface* RI = GDynamicRHI->RHIVXGIGetRendererInterface();

	if (GBufferAccessShader->ConstantBuffer == NULL)
	{
		NVRHI::ConstantBufferDesc Desc;
		Desc.byteSize = sizeof(FVxgiGBufferAccessShader::ConstantBufferData);
		Desc.debugName = "VxgiGBufferAccessConstants";
		GBufferAccessShader->ConstantBuffer = RI->createConstantBuffer(Desc, NULL);
	}

	uint32 CurrentGPU = RI->getAFRGroupOfCurrentFrame(GNumActiveGPUsForRendering);

	NVRHI::PipelineStageBindings GBufferBindings;
	GBufferBindings.textureBindingCount = 6;
	GBufferBindings.textures[0].slot = 0; GBufferBindings.textures[0].texture = GetNVRHITexture2DHandle(SceneContext.SceneDepthZ);
	GBufferBindings.textures[1].slot = 1; GBufferBindings.textures[1].texture = GetNVRHITexture2DHandle(SceneContext.GBufferA);
	GBufferBindings.textures[2].slot = 2; GBufferBindings.textures[2].texture = GetNVRHITexture2DHandle(SceneContext.GBufferB);
	GBufferBindings.textures[3].slot = 3; GBufferBindings.textures[3].texture = GetNVRHITexture2DHandle(SceneContext.PrevSceneDepthZ);
	GBufferBindings.textures[4].slot = 4; GBufferBindings.textures[4].texture = GetNVRHITexture2DHandle(PrimaryView.ViewState && PrimaryView.ViewState->PrevGBufferA[CurrentGPU] ? PrimaryView.ViewState->PrevGBufferA[CurrentGPU] : SceneContext.GBufferA);
	GBufferBindings.textures[5].slot = 5; GBufferBindings.textures[5].texture = GetNVRHITexture2DHandle(PrimaryView.ViewState && PrimaryView.ViewState->PrevGBufferB[CurrentGPU] ? PrimaryView.ViewState->PrevGBufferB[CurrentGPU] : SceneContext.GBufferB);

	GBufferBindings.constantBufferBindingCount = 1;
	GBufferBindings.constantBuffers[0].slot = 0;
	GBufferBindings.constantBuffers[0].buffer = GBufferAccessShader->ConstantBuffer;

	FVxgiGBufferAccessShader::ConstantBufferData Constants = {};
	FillVxgiConstantBuffer(Views, NumViewsToProcess, CurrentGPU, Constants);

	bool bEnableTemporalReprojection = bVxgiTemporalReprojectionEnable;
	bool bEnableDiffuseStereoReprojection = !!CVarVxgiDiffuseStereoReprojectionEnable.GetValueOnRenderThread();
	bool bEnableSpecularStereoReprojection = !!CVarVxgiSpecularStereoReprojectionEnable.GetValueOnRenderThread();

	VXGI::IViewTracer::ViewInfo VxgiViewInfos[FVxgiGBufferAccessShader::MaxViews];
	float ViewSimilarity = 1.0f;

	for (int32 ViewIndex = 0; ViewIndex < NumViewsToProcess; ViewIndex++)
	{
		FViewInfo& View = Views[ViewIndex];
		VxgiViewInfos[ViewIndex].extents = NVRHI::Rect(View.ViewRect.Min.X, View.ViewRect.Max.X, View.ViewRect.Min.Y, View.ViewRect.Max.Y);

		if (View.ViewState == NULL || View.ViewState->PrevGBufferA[CurrentGPU] == NULL || View.bPrevTransformsReset)
			bEnableTemporalReprojection = false;

		if (ViewIndex > 0 && View.StereoPass == eSSP_RIGHT_EYE && Views[ViewIndex - 1].StereoPass == eSSP_LEFT_EYE)
		{
			VxgiViewInfos[ViewIndex].reprojectionSourceViewIndex = ViewIndex - 1;

			if (bEnableSpecularStereoReprojection)
			{
				// Compute the average eye position for specular reflections if reprojection is enabled.
				// It's only required for the current view state, not the previous one.
				FVector MiddleEyePosition = (Constants.g_GBuffer[ViewIndex].cameraPosition + Constants.g_GBuffer[ViewIndex - 1].cameraPosition) * 0.5f;
				Constants.g_GBuffer[ViewIndex].cameraPosition = MiddleEyePosition;
				Constants.g_GBuffer[ViewIndex - 1].cameraPosition = MiddleEyePosition;
			}
		}

		if (View.ViewState != NULL)
		{
			float CameraOffset = FMath::Sqrt((Constants.g_GBuffer[ViewIndex].cameraPosition - Constants.g_PreviousGBuffer[ViewIndex].cameraPosition).SizeSquared3());
			float Similarity = FMath::Max(0.f, 1.0f - 0.5f * CameraOffset / VxgiVoxelSize);
			ViewSimilarity = FMath::Min(ViewSimilarity, Similarity);
		}

		VxgiViewInfos[ViewIndex].matricesAreValid = true;
		VxgiViewInfos[ViewIndex].reverseProjection = true;
		FMemory::Memcpy(&VxgiViewInfos[ViewIndex].viewMatrix, &View.ViewMatrices.GetViewMatrix(), sizeof(VXGI::float4x4));
		FMemory::Memcpy(&VxgiViewInfos[ViewIndex].projMatrix, &View.ViewMatrices.GetProjectionMatrix(), sizeof(VXGI::float4x4));
	}

	// This needs to happen after the loop above because the loop patches some constants
	RI->writeConstantBuffer(GBufferAccessShader->ConstantBuffer, &Constants, sizeof(Constants));

	FIntPoint GBufferSize = SceneContext.GetBufferSizeXY();
	
	SCOPED_DRAW_EVENT(RHICmdList, VXGITracing);

    VxgiViewTracer->beginFrame();

	{
		SCOPED_DRAW_EVENT(RHICmdList, DiffuseConeTracing);
		NVRHI::TextureHandle IlluminationDiffuseHandle = NULL;
		NVRHI::TextureHandle ConfidenceHandle = NULL;

		VXGI::DiffuseTracingParameters DiffuseTracingParams;
		SetVxgiDiffuseTracingParameters(PrimaryView, DiffuseTracingParams, bEnableTemporalReprojection, bEnableDiffuseStereoReprojection);

		if (PrimaryView.FinalPostProcessSettings.VxgiDiffuseTracingEnabled || PrimaryView.FinalPostProcessSettings.bVxgiAmbientOcclusionEnabled)
		{
			auto Status = VxgiViewTracer->computeDiffuseChannel(
				DiffuseTracingParams,
				GBufferBindings,
				VXGI::int2(GBufferSize.X, GBufferSize.Y),
				VxgiViewInfos,
				NumViewsToProcess,
				IlluminationDiffuseHandle,
				ConfidenceHandle);
			check(VXGI_SUCCEEDED(Status));
		}

		FTextureRHIRef IlluminationDiffuse(GDynamicRHI->GetRHITextureFromVXGI(IlluminationDiffuseHandle));
		FTextureRHIRef Confidence(GDynamicRHI->GetRHITextureFromVXGI(ConfidenceHandle));
		if (IlluminationDiffuse)
		{
			SceneContext.VxgiOutputDiffuse = IlluminationDiffuse->GetTexture2D();
		}
		if (Confidence)
		{
			SceneContext.VxgiOutputConfidence = Confidence->GetTexture2D();
		}
	}

	{
		SCOPED_DRAW_EVENT(RHICmdList, SpecularConeTracing);
		NVRHI::TextureHandle IlluminationSpecHandle = NULL;

		VXGI::BasicSpecularTracingParameters SpecularTracingParams;
		SetVxgiSpecularTracingParameters(PrimaryView, SpecularTracingParams, bEnableTemporalReprojection, bEnableSpecularStereoReprojection, ViewSimilarity);

		if (PrimaryView.FinalPostProcessSettings.VxgiSpecularTracingEnabled)
		{
			auto Status = VxgiViewTracer->computeSpecularChannel(
				SpecularTracingParams,
				GBufferBindings,
				VXGI::int2(GBufferSize.X, GBufferSize.Y),
				VxgiViewInfos,
				NumViewsToProcess,
				IlluminationSpecHandle);
			check(VXGI_SUCCEEDED(Status));
		}

		FTextureRHIRef IlluminationSpec(GDynamicRHI->GetRHITextureFromVXGI(IlluminationSpecHandle));
		if (IlluminationSpec)
		{
			SceneContext.VxgiOutputSpec = IlluminationSpec->GetTexture2D();
		}
	}

	{
		SCOPED_DRAW_EVENT(RHICmdList, AreaLights);
		NVRHI::TextureHandle AreaLightsDiffuseHandle = NULL;
		NVRHI::TextureHandle AreaLightsSpecularHandle = NULL;

		VXGI::AreaLightTracingParameters AreaLightTracingParams;
		SetVxgiAreaLightTracingParameters(PrimaryView, AreaLightTracingParams, bEnableTemporalReprojection);
		
		if (PrimaryView.FinalPostProcessSettings.VxgiAreaLightsEnabled && VxgiAreaLights.Num() != 0)
		{
			for (int32 LightIndex = 0; LightIndex < VxgiAreaLights.Num(); LightIndex++)
			{
				VXGI::AreaLight& Light = VxgiAreaLights[LightIndex];
				AAreaLightActor* Actor = VxgiAreaLightActors[LightIndex];

				if (Actor->LightTexture)
				{
					Light.texture = GDynamicRHI->GetVXGITextureFromRHI(Actor->LightTexture->Resource->TextureRHI->GetTexture2D());
				}
			}

			auto Status = VxgiViewTracer->computeAreaLightChannels(
				AreaLightTracingParams,
				GBufferBindings,
				VXGI::int2(GBufferSize.X, GBufferSize.Y),
				VxgiViewInfos,
				NumViewsToProcess,
				&VxgiAreaLights[0],
				VxgiAreaLights.Num(),
				AreaLightsDiffuseHandle,
				AreaLightsSpecularHandle);
			check(VXGI_SUCCEEDED(Status));
		}

		FTextureRHIRef AreaLightDiffuse(GDynamicRHI->GetRHITextureFromVXGI(AreaLightsDiffuseHandle));
		FTextureRHIRef AreaLightSpecular(GDynamicRHI->GetRHITextureFromVXGI(AreaLightsSpecularHandle));
		if (AreaLightDiffuse)
		{
			SceneContext.VxgiOutputAreaLightDiffuse = AreaLightDiffuse->GetTexture2D();
		}
		if (AreaLightSpecular)
		{
			SceneContext.VxgiOutputAreaLightSpecular = AreaLightSpecular->GetTexture2D();
		}
	}

	if (bVxgiTemporaryTracer)
	{
		VxgiInterface->destroyTracer(VxgiViewTracer);
	}

	RHICmdList.SetScissorRect(false, 0, 0, 0, 0);

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		FSceneViewState* ViewState = Views[ViewIndex].ViewState;
		if (ViewState != nullptr)
		{
			if (bVxgiTemporalReprojectionEnable)
			{
				ViewState->PrevGBufferA[CurrentGPU] = SceneContext.GBufferA;
				ViewState->PrevGBufferB[CurrentGPU] = SceneContext.GBufferB;
			}
			else
			{
				ViewState->PrevGBufferA[CurrentGPU].SafeRelease();
				ViewState->PrevGBufferB[CurrentGPU].SafeRelease();
			}
		}
	}

	if (bVxgiTemporalReprojectionEnable)
	{
		FResolveParams ResolveParams;
		RHICmdList.CopyToResolveTarget(SceneContext.GetSceneDepthTexture(), SceneContext.PrevSceneDepthZ->GetRenderTargetItem().TargetableTexture, true, ResolveParams);
	}
}

void FSceneRenderer::EndVxgiFinalPostProcessSettingsForAllViews()
{
	// Clamp the parameters first because they might affect the output of IsVxgiEnabled
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		EndVxgiFinalPostProcessSettings(Views[ViewIndex].FinalPostProcessSettings);
		if (Views[ViewIndex].State == nullptr)
		{
			//We need the viewstate to implement this
			Views[ViewIndex].FinalPostProcessSettings.bVxgiDiffuseTracingTemporalReprojectionEnabled = false;
		}

		const auto& PostSettings = Views[ViewIndex].FinalPostProcessSettings;
		if (PostSettings.bVxgiAmbientOcclusionEnabled)
		{
			Views[ViewIndex].VxgiAmbientOcclusionMode = bVxgiAmbientOcclusionMode
				? EVxgiAmbientOcclusionMode::RedChannel
				: EVxgiAmbientOcclusionMode::AlphaChannel;
		}
		else
		{
			Views[ViewIndex].VxgiAmbientOcclusionMode = EVxgiAmbientOcclusionMode::None;
		}
	}
}

void FSceneRenderer::EndVxgiFinalPostProcessSettings(FFinalPostProcessSettings& FinalPostProcessSettings)
{
	if (!CVarVxgiDiffuseTracingEnable.GetValueOnRenderThread() || !ViewFamily.EngineShowFlags.VxgiDiffuse)
	{
		FinalPostProcessSettings.VxgiDiffuseTracingEnabled = false;
		FinalPostProcessSettings.bVxgiAmbientOcclusionEnabled = false;
	}
	if (!CVarVxgiSpecularTracingEnable.GetValueOnRenderThread() || !ViewFamily.EngineShowFlags.VxgiSpecular)
	{
		FinalPostProcessSettings.VxgiSpecularTracingEnabled = false;
	}
	if (!bVxgiTemporalReprojectionEnable)
	{
		FinalPostProcessSettings.bVxgiDiffuseTracingTemporalReprojectionEnabled = false;
	}

	switch (CVarVxgiCompositingMode.GetValueOnRenderThread())
	{
	case 1: // Indirect Diffuse Only
		FinalPostProcessSettings.VxgiDiffuseTracingEnabled = true;
		FinalPostProcessSettings.VxgiSpecularTracingEnabled = false;
		FinalPostProcessSettings.ScreenSpaceReflectionIntensity = 0.f;
		break;
	case 2: // Direct Only
		FinalPostProcessSettings.VxgiDiffuseTracingEnabled = false;
		FinalPostProcessSettings.VxgiSpecularTracingEnabled = false;
		FinalPostProcessSettings.ScreenSpaceReflectionIntensity = 0.f;
		break;
	}

	if (bVxgiAmbientOcclusionMode)
	{
		// Occlusion-only mode

		FinalPostProcessSettings.VxgiDiffuseTracingEnabled = false;
		FinalPostProcessSettings.VxgiSpecularTracingEnabled = false;
	}
}

void FSceneRenderer::RenderVxgiVoxelizationPass(
	FRHICommandList& RHICmdList,
	int32 VoxelizationPass,
	const FVxgiVoxelizationArgs& Args)
{
	if (Args.LightSceneInfo && !Args.LightSceneInfo->Proxy->CastVxgiIndirectLighting(nullptr))
	{
		return;
	}

	RHIPushVoxelizationFlag();

	FViewInfo& View = *VxgiView;

	View.VxgiVoxelizationArgs = Args;
	View.VxgiVoxelizationArgs.bEnableEmissiveMaterials = bVxgiUseEmissiveMaterials;
	View.VxgiVoxelizationArgs.bEnableSkyLight = bVxgiSkyLightEnable;
	View.VxgiVoxelizationArgs.bAmbientOcclusionMode = bVxgiAmbientOcclusionMode;
	View.VxgiVoxelizationPass = VoxelizationPass;
	
	VXGI::IGlobalIllumination* VxgiInterface = GDynamicRHI->RHIVXGIGetInterface();
	VxgiInterface->beginVoxelizationDrawCallGroup();

	{
		SCOPED_DRAW_EVENT(RHICmdList, StaticGeometry);

		FSceneBitArray StaticMeshVisibilityMap = View.StaticMeshVisibilityMap;

		int NumCulled = 0;

		if (Args.LightSceneInfo)
		{
			// For passes that voxelize geometry for a light, perform culling against the light frustum:
			// iterate over meshes that intersect with the clipmap, and hide all that are not in affected by the light.

			FLightSceneInfoCompact LightSceneInfoCompact(const_cast<FLightSceneInfo*>(Args.LightSceneInfo));

			Scene->VxgiVoxelizationDrawList.IterateOverMeshes([&LightSceneInfoCompact, &StaticMeshVisibilityMap](FStaticMesh* Mesh)
			{
				FBitReference MeshBit = StaticMeshVisibilityMap.AccessCorrespondingBit(FRelativeBitReference(Mesh->Id));
				if (!MeshBit)
					return;

				FPrimitiveSceneProxy* Proxy = Mesh->PrimitiveSceneInfo->Proxy;
				if (!LightSceneInfoCompact.AffectsPrimitive(Proxy->GetBounds(), Proxy))
				{
					MeshBit = 0;
				}
			});
		}
		else if (VoxelizationPass == EVxgiVoxelizationPass::FINALIZATION)
		{
			// For the final pass, only draw meshes that were not drawn in any of the previous voxelization passes. 
			// If a mesh was drawn before, the opacity and emissive etc. components were added on the first emittance voxelization pass.

			Scene->VxgiVoxelizationDrawList.IterateOverMeshes([this, &StaticMeshVisibilityMap](FStaticMesh* Mesh)
			{
				FBitReference MeshBit = StaticMeshVisibilityMap.AccessCorrespondingBit(FRelativeBitReference(Mesh->Id));
				if (!MeshBit)
					return;

				if (Mesh->PrimitiveSceneInfo->VxgiLastVoxelizationPass != EVxgiVoxelizationPass::NONE)
				{
					MeshBit = 0;
				}
			});
		}
		else if (VoxelizationPass == EVxgiVoxelizationPass::EXTRA_OPACITY)
		{
			// Super-final pass: opacity for meshes that use Adaptive Material Sampling Rate.
			// Combined opacity / emittance voxelization works slower on such materials.

			Scene->VxgiVoxelizationDrawList.IterateOverMeshes([this, &StaticMeshVisibilityMap](FStaticMesh* Mesh)
			{
				FBitReference MeshBit = StaticMeshVisibilityMap.AccessCorrespondingBit(FRelativeBitReference(Mesh->Id));
				if (!MeshBit)
					return;

				FVxgiMaterialProperties MaterialProps = Mesh->MaterialRenderProxy->GetVxgiMaterialProperties();
				if (!MaterialProps.bVxgiAdaptiveMaterialSamplingRate)
				{
					MeshBit = 0;
				}
			});

		}
		else if (VoxelizationPass == EVxgiVoxelizationPass::ONLY_OPACITY)
		{
			// The only pass in ambient occlusion mode: draw everything where OpacityScale > 0

			Scene->VxgiVoxelizationDrawList.IterateOverMeshes([this, &StaticMeshVisibilityMap](FStaticMesh* Mesh)
			{
				FBitReference MeshBit = StaticMeshVisibilityMap.AccessCorrespondingBit(FRelativeBitReference(Mesh->Id));
				if (!MeshBit)
					return;

				FVxgiMaterialProperties MaterialProps = Mesh->MaterialRenderProxy->GetVxgiMaterialProperties();
				if (MaterialProps.VxgiOpacityScale <= 0.f)
				{
					MeshBit = 0;
				}
			});

		}

		FDrawingPolicyRenderState RenderState(View);
		RenderState.SetBlendState(TStaticBlendState<>::GetRHI());
		RenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
		Scene->VxgiVoxelizationDrawList.DrawVisible(RHICmdList, View, RenderState, StaticMeshVisibilityMap, View.StaticMeshBatchVisibility);
	}

	{
		SCOPED_DRAW_EVENT(RHICmdList, DynamicGeometry);

		TVXGIVoxelizationDrawingPolicyFactory::ContextType Context;

		FDrawingPolicyRenderState RenderState(View);
		RenderState.SetBlendState(TStaticBlendState<>::GetRHI());
		RenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());

		if (Args.LightSceneInfo)
		{
			FLightSceneInfoCompact LightSceneInfoCompact(const_cast<FLightSceneInfo*>(Args.LightSceneInfo));

			for (int32 MeshBatchIndex = 0; MeshBatchIndex < View.DynamicMeshElements.Num(); MeshBatchIndex++)
			{
				const FMeshBatchAndRelevance& MeshBatchAndRelevance = View.DynamicMeshElements[MeshBatchIndex];

				if (MeshBatchAndRelevance.GetHasOpaqueOrMaskedMaterial() && MeshBatchAndRelevance.GetRenderInMainPass())
				{
					const FPrimitiveSceneProxy* Proxy = MeshBatchAndRelevance.PrimitiveSceneProxy;
					if (!LightSceneInfoCompact.AffectsPrimitive(Proxy->GetBounds(), Proxy))
					{
						continue;
					}

					const FMeshBatch& MeshBatch = *MeshBatchAndRelevance.Mesh;

					TVXGIVoxelizationDrawingPolicyFactory::DrawDynamicMesh(RHICmdList, View, Context, MeshBatch, true, RenderState, MeshBatchAndRelevance.PrimitiveSceneProxy, MeshBatch.BatchHitProxyId);
				}
			}
		}
		else if(VoxelizationPass == EVxgiVoxelizationPass::FINALIZATION)
		{
			for (int32 MeshBatchIndex = 0; MeshBatchIndex < View.DynamicMeshElements.Num(); MeshBatchIndex++)
			{
				const FMeshBatchAndRelevance& MeshBatchAndRelevance = View.DynamicMeshElements[MeshBatchIndex];

				if (MeshBatchAndRelevance.GetHasOpaqueOrMaskedMaterial() && MeshBatchAndRelevance.GetRenderInMainPass())
				{
					const FMeshBatch& MeshBatch = *MeshBatchAndRelevance.Mesh;

					TVXGIVoxelizationDrawingPolicyFactory::DrawDynamicMesh(RHICmdList, View, Context, MeshBatch, true, RenderState, MeshBatchAndRelevance.PrimitiveSceneProxy, MeshBatch.BatchHitProxyId);
				}
			}
		}
		else if (VoxelizationPass == EVxgiVoxelizationPass::EXTRA_OPACITY)
		{
			for (int32 MeshBatchIndex = 0; MeshBatchIndex < View.DynamicMeshElements.Num(); MeshBatchIndex++)
			{
				const FMeshBatchAndRelevance& MeshBatchAndRelevance = View.DynamicMeshElements[MeshBatchIndex];

				if (MeshBatchAndRelevance.GetHasOpaqueOrMaskedMaterial() && MeshBatchAndRelevance.GetRenderInMainPass())
				{
					const FMeshBatch& MeshBatch = *MeshBatchAndRelevance.Mesh;

					FVxgiMaterialProperties MaterialProps = MeshBatch.MaterialRenderProxy->GetVxgiMaterialProperties();

					if (MaterialProps.bVxgiAdaptiveMaterialSamplingRate)
					{
						TVXGIVoxelizationDrawingPolicyFactory::DrawDynamicMesh(RHICmdList, View, Context, MeshBatch, true, RenderState, MeshBatchAndRelevance.PrimitiveSceneProxy, MeshBatch.BatchHitProxyId);
					}
				}
			}
		}
		else if (VoxelizationPass == EVxgiVoxelizationPass::ONLY_OPACITY)
		{
			for (int32 MeshBatchIndex = 0; MeshBatchIndex < View.DynamicMeshElements.Num(); MeshBatchIndex++)
			{
				const FMeshBatchAndRelevance& MeshBatchAndRelevance = View.DynamicMeshElements[MeshBatchIndex];

				if (MeshBatchAndRelevance.GetHasOpaqueOrMaskedMaterial() && MeshBatchAndRelevance.GetRenderInMainPass())
				{
					const FMeshBatch& MeshBatch = *MeshBatchAndRelevance.Mesh;

					FVxgiMaterialProperties MaterialProps = MeshBatch.MaterialRenderProxy->GetVxgiMaterialProperties();

					if (MaterialProps.VxgiOpacityScale > 0.f)
					{
						TVXGIVoxelizationDrawingPolicyFactory::DrawDynamicMesh(RHICmdList, View, Context, MeshBatch, true, RenderState, MeshBatchAndRelevance.PrimitiveSceneProxy, MeshBatch.BatchHitProxyId);
					}
				}
			}
		}

		View.SimpleElementCollector.DrawBatchedElements(RHICmdList, RenderState, View, EBlendModeFilter::OpaqueAndMasked);

		if (!View.Family->EngineShowFlags.CompositeEditorPrimitives)
		{
			bool bDirty = false;
			bDirty = DrawViewElements<TVXGIVoxelizationDrawingPolicyFactory>(RHICmdList, View, RenderState, Context, SDPG_World, true) || bDirty;
			bDirty = DrawViewElements<TVXGIVoxelizationDrawingPolicyFactory>(RHICmdList, View, RenderState, Context, SDPG_Foreground, true) || bDirty;
		}
	}

	VxgiInterface->endVoxelizationDrawCallGroup();

	RHICmdList.VXGICleanupAfterVoxelization();
	RHIPopVoxelizationFlag();
}

VXGI::Box3f FSceneRenderer::GetVxgiWorldSpaceSceneBounds()
{
	VXGI::Box3f SceneExtents(VXGI::float3(-FLT_MAX), VXGI::float3(FLT_MAX));
	return SceneExtents;
}

void FSceneRenderer::RenderVxgiDebug(FRHICommandListImmediate& RHICmdList, const FViewInfo& View, int ViewIndex)
{
	if (!IsVxgiEnabled(View))
	{
		return;
	}

	VXGI::DebugRenderParameters Params;

	if (ViewFamily.EngineShowFlags.VxgiOpacityVoxels)
		Params.debugMode = VXGI::DebugRenderMode::OPACITY_TEXTURE;
	else if (ViewFamily.EngineShowFlags.VxgiEmittanceVoxels)
		Params.debugMode = VXGI::DebugRenderMode::EMITTANCE_TEXTURE;
	else if (ViewFamily.EngineShowFlags.VxgiIrradianceVoxels)
		Params.debugMode = VXGI::DebugRenderMode::INDIRECT_IRRADIANCE_TEXTURE;
	else
		return;


	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

	SCOPED_DRAW_EVENT(RHICmdList, VXGIDebugRendering);

	VXGI::IGlobalIllumination* VxgiInterface = GDynamicRHI->RHIVXGIGetInterface();


	// With reverse infinite projections, the near plane is at Z=1 and the far plane is at Z=0
	// The lib uses these 2 values along with the ViewProjMatrix to compute the ray directions
	const float nearClipZ = 1.0f;
	const float farClipZ = 0.0f;

	Params.viewport = NVRHI::Viewport(View.ViewRect.Min.X, View.ViewRect.Max.X, View.ViewRect.Min.Y, View.ViewRect.Max.Y, 0.f, 1.f);

	Params.destinationTexture = GDynamicRHI->GetVXGITextureFromRHI((FRHITexture*)SceneContext.GetSceneColorSurface().GetReference());
	
	FRHISetRenderTargetsInfo RTInfo;
	RTInfo.bClearColor = false;
	RTInfo.bClearDepth = true;
	RTInfo.NumColorRenderTargets = 1;
	RTInfo.ColorRenderTarget[0] = FRHIRenderTargetView(SceneContext.GetSceneColorSurface(), ERenderTargetLoadAction::ELoad);
	RTInfo.DepthStencilRenderTarget = FRHIDepthRenderTargetView(SceneContext.GetSceneDepthSurface(), ERenderTargetLoadAction::ELoad, ERenderTargetStoreAction::EStore);
	RHICmdList.SetRenderTargetsAndClear(RTInfo);

	RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);

	Params.destinationDepth = GDynamicRHI->GetVXGITextureFromRHI((FRHITexture*)SceneContext.GetSceneDepthSurface().GetReference());

	VXGI::float4x4 ViewMatrix, ProjMatrix;
	FMemory::Memcpy(&Params.viewMatrix, &View.ViewMatrices.GetViewMatrix(), sizeof(VXGI::float4x4));
	FMemory::Memcpy(&Params.projMatrix, &View.ViewMatrices.GetProjectionMatrix(), sizeof(VXGI::float4x4));

	Params.level = CVarVxgiDebugClipmapLevel.GetValueOnRenderThread();
	Params.bitToDisplay = 0;
	Params.voxelsToSkip = 0;
	Params.nearClipZ = nearClipZ;
	Params.farClipZ = farClipZ;

	Params.depthStencilState.depthEnable = true;
	Params.depthStencilState.depthFunc = NVRHI::DepthStencilState::COMPARISON_GREATER;

	auto Status = VxgiInterface->renderDebug(Params);

	check(VXGI_SUCCEEDED(Status));
}

/** Encapsulates the post processing ambient occlusion pixel shader. */
template<bool bRawDiffuse>
class FAddVxgiDiffusePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FAddVxgiDiffusePS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Params)
	{
		return IsFeatureLevelSupported(Params.Platform, ERHIFeatureLevel::SM5);
	}

	/** Default constructor. */
	FAddVxgiDiffusePS() {}

	FDeferredPixelShaderParameters DeferredParameters;

	FShaderParameter EnableVxgiDiffuse;
	FShaderParameter EnableAreaLightDiffuse;

public:
	/** Initialization constructor. */
	FAddVxgiDiffusePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DeferredParameters.Bind(Initializer.ParameterMap);
		EnableVxgiDiffuse.Bind(Initializer.ParameterMap, TEXT("EnableVxgiDiffuse"));
		EnableAreaLightDiffuse.Bind(Initializer.ParameterMap, TEXT("EnableAreaLightDiffuse"));
	}

	void SetParameters(FRHICommandList& RHICmdList, const FSceneView& View, bool bVxgiAmbientOcclusionMode)
	{
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, GetPixelShader(), View.ViewUniformBuffer);
		DeferredParameters.Set(RHICmdList, GetPixelShader(), View, EMaterialDomain::MD_PostProcess);

		bool bEnableVxgiDiffuse = !bVxgiAmbientOcclusionMode && View.FinalPostProcessSettings.VxgiDiffuseTracingEnabled;
		bool bEnableAreaLightDiffuse = View.FinalPostProcessSettings.VxgiAreaLightsEnabled;

		SetShaderValue(RHICmdList, GetPixelShader(), EnableVxgiDiffuse, bEnableVxgiDiffuse);
		SetShaderValue(RHICmdList, GetPixelShader(), EnableAreaLightDiffuse, bEnableAreaLightDiffuse);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DeferredParameters;
		Ar << EnableVxgiDiffuse;
		Ar << EnableAreaLightDiffuse;
		return bShaderHasOutdatedParameters;
	}
};

typedef FAddVxgiDiffusePS<false> FAddVxgiCompositedDiffusePS;
typedef FAddVxgiDiffusePS<true>  FAddVxgiRawDiffusePS;

IMPLEMENT_SHADER_TYPE(, FAddVxgiCompositedDiffusePS, TEXT("/Engine/VXGICompositing.usf"), TEXT("AddVxgiDiffusePS"), SF_Pixel);
IMPLEMENT_SHADER_TYPE(, FAddVxgiRawDiffusePS, TEXT("/Engine/VXGICompositing.usf"), TEXT("AddVxgiRawDiffusePS"), SF_Pixel);

void FSceneRenderer::CompositeVxgiDiffuseTracing(FRHICommandListImmediate& RHICmdList, const FViewInfo& View)
{
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);
	
	if (IsValidRef(SceneContext.VxgiOutputDiffuse) || IsValidRef(SceneContext.VxgiOutputAreaLightDiffuse))
	{
		SCOPED_DRAW_EVENT(RHICmdList, VXGICompositeDiffuse);

		SceneContext.BeginRenderingSceneColor(RHICmdList);

		const FSceneRenderTargetItem& DestRenderTarget = SceneContext.GetSceneColor()->GetRenderTargetItem();
		SetRenderTarget(RHICmdList, DestRenderTarget.TargetableTexture, FTextureRHIRef());

		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

		TShaderMapRef<FScreenVS> VertexShader(View.ShaderMap);

		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;

		if (CVarVxgiCompositingMode.GetValueOnRenderThread())
		{
			GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA>::GetRHI();

			TShaderMapRef<FAddVxgiRawDiffusePS> PixelShader(View.ShaderMap);
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);

			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

			PixelShader->SetParameters(RHICmdList, View, bVxgiAmbientOcclusionMode);
		}
		else
		{
			GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One>::GetRHI();
			TShaderMapRef<FAddVxgiCompositedDiffusePS> PixelShader(View.ShaderMap);
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
			
			SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

			PixelShader->SetParameters(RHICmdList, View, bVxgiAmbientOcclusionMode);
		}

		VertexShader->SetParameters(RHICmdList, View.ViewUniformBuffer);

		RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);

		// Draw a quad mapping scene color to the view's render target
		DrawRectangle(
			RHICmdList,
			0, 0,
			View.ViewRect.Width(), View.ViewRect.Height(),
			View.ViewRect.Min.X, View.ViewRect.Min.Y,
			View.ViewRect.Width(), View.ViewRect.Height(),
			View.ViewRect.Size(),
			SceneContext.GetBufferSizeXY(),
			*VertexShader);
	}
}

bool FVXGIVoxelizationNoLightMapPolicy::ShouldCompilePermutation(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
{
	return true;
}

void FVXGIVoxelizationNoLightMapPolicy::ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
{
	OutEnvironment.SetDefine(TEXT("SIMPLE_DYNAMIC_LIGHTING"),TEXT("1"));
	FNoLightMapPolicy::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
}

#define IMPLEMENT_VXGI_VOXELIZATION_SHADER_TYPE(LightMapPolicyType,LightMapPolicyName) \
	typedef TVXGIVoxelizationHS< LightMapPolicyType > TVXGIVoxelizationHS##LightMapPolicyName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TVXGIVoxelizationHS##LightMapPolicyName,TEXT("/Engine/Private/MobileBasePassVertexShader.usf"),TEXT("MainHull"),SF_Hull); \
	typedef TVXGIVoxelizationDS< LightMapPolicyType > TVXGIVoxelizationDS##LightMapPolicyName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TVXGIVoxelizationDS##LightMapPolicyName,TEXT("/Engine/Private/MobileBasePassVertexShader.usf"),TEXT("MainDomain"),SF_Domain); \
	typedef TVXGIVoxelizationVS< LightMapPolicyType > TVXGIVoxelizationVS##LightMapPolicyName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TVXGIVoxelizationVS##LightMapPolicyName,TEXT("/Engine/Private/MobileBasePassVertexShader.usf"),TEXT("Main"),SF_Vertex); \
	typedef TVXGIVoxelizationPS< LightMapPolicyType > TVXGIVoxelizationPS##LightMapPolicyName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TVXGIVoxelizationPS##LightMapPolicyName,TEXT("/Engine/VXGIVoxelizationPixelShader.usf"),TEXT("Main"),SF_Pixel); \
	typedef TVXGIVoxelizationShaderPermutationPS< LightMapPolicyType > TVXGIVoxelizationShaderPermutationPS##LightMapPolicyName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TVXGIVoxelizationShaderPermutationPS##LightMapPolicyName,TEXT("/Engine/VXGIVoxelizationPixelShader.usf"),TEXT("Main"),SF_Pixel);

// Implement shader types only for FVXGIVoxelizationNoLightMapPolicy because we control the drawing process
IMPLEMENT_VXGI_VOXELIZATION_SHADER_TYPE(FVXGIVoxelizationNoLightMapPolicy, FVXGIVoxelizationNoLightMapPolicy);

#define IMPLEMENT_VXGI_CONE_TRACING_SHADER_TYPE(LightMapPolicyType,LightMapPolicyName) \
	typedef TVXGIConeTracingPS< LightMapPolicyType > TVXGIConeTracingPS##LightMapPolicyName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TVXGIConeTracingPS##LightMapPolicyName,TEXT("/Engine/Private/BasePassPixelShader.usf"),TEXT("MainPS"),SF_Pixel); \
	typedef TVXGIConeTracingShaderPermutationPS< LightMapPolicyType > TVXGIConeTracingShaderPermutationPS##LightMapPolicyName; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TVXGIConeTracingShaderPermutationPS##LightMapPolicyName,TEXT("/Engine/Private/BasePassPixelShader.usf"),TEXT("MainPS"),SF_Pixel);

// Implement shader types for all light map policies that are used in ProcessBasePassMesh
IMPLEMENT_VXGI_CONE_TRACING_SHADER_TYPE(TUniformLightMapPolicy<LMP_NO_LIGHTMAP>, _LMP_NO_LIGHTMAP);
IMPLEMENT_VXGI_CONE_TRACING_SHADER_TYPE(TUniformLightMapPolicy<LMP_HQ_LIGHTMAP>, _LMP_HQ_LIGHTMAP);
IMPLEMENT_VXGI_CONE_TRACING_SHADER_TYPE(TUniformLightMapPolicy<LMP_LQ_LIGHTMAP>, _LMP_LQ_LIGHTMAP);
IMPLEMENT_VXGI_CONE_TRACING_SHADER_TYPE(TUniformLightMapPolicy<LMP_SIMPLE_NO_LIGHTMAP>, _LMP_SIMPLE_NO_LIGHTMAP);
IMPLEMENT_VXGI_CONE_TRACING_SHADER_TYPE(TUniformLightMapPolicy<LMP_SIMPLE_LIGHTMAP_ONLY_LIGHTING>, _LMP_SIMPLE_LIGHTMAP_ONLY_LIGHTING);
IMPLEMENT_VXGI_CONE_TRACING_SHADER_TYPE(TUniformLightMapPolicy<LMP_SIMPLE_DIRECTIONAL_LIGHT_LIGHTING>, _LMP_SIMPLE_DIRECTIONAL_LIGHT_LIGHTING);
IMPLEMENT_VXGI_CONE_TRACING_SHADER_TYPE(TUniformLightMapPolicy<LMP_SIMPLE_STATIONARY_PRECOMPUTED_SHADOW_LIGHTING>, _LMP_SIMPLE_STATIONARY_PRECOMPUTED_SHADOW_LIGHTING);
IMPLEMENT_VXGI_CONE_TRACING_SHADER_TYPE(TUniformLightMapPolicy<LMP_SIMPLE_STATIONARY_SINGLESAMPLE_SHADOW_LIGHTING>, _LMP_SIMPLE_STATIONARY_SINGLESAMPLE_SHADOW_LIGHTING);
IMPLEMENT_VXGI_CONE_TRACING_SHADER_TYPE(TUniformLightMapPolicy<LMP_CACHED_VOLUME_INDIRECT_LIGHTING>, _LMP_CACHED_VOLUME_INDIRECT_LIGHTING);
IMPLEMENT_VXGI_CONE_TRACING_SHADER_TYPE(TUniformLightMapPolicy<LMP_CACHED_POINT_INDIRECT_LIGHTING>, _LMP_CACHED_POINT_INDIRECT_LIGHTING);
IMPLEMENT_VXGI_CONE_TRACING_SHADER_TYPE(TUniformLightMapPolicy<LMP_DISTANCE_FIELD_SHADOWS_AND_HQ_LIGHTMAP>, _LMP_DISTANCE_FIELD_SHADOWS_AND_HQ_LIGHTMAP);
IMPLEMENT_VXGI_CONE_TRACING_SHADER_TYPE(FSelfShadowedCachedPointIndirectLightingPolicy, FSelfShadowedCachedPointIndirectLightingPolicy);
IMPLEMENT_VXGI_CONE_TRACING_SHADER_TYPE(FSelfShadowedTranslucencyPolicy, FSelfShadowedTranslucencyPolicy);
IMPLEMENT_VXGI_CONE_TRACING_SHADER_TYPE(FSelfShadowedVolumetricLightmapPolicy, FSelfShadowedVolumetricLightmapPolicy);

template <>
void GetConeTracingPixelShader<FUniformLightMapPolicy>(
	const FVertexFactory* InVertexFactory,
	const FMaterial& InMaterialResource,
	FUniformLightMapPolicy* InLightMapPolicy,
	TBasePassPixelShaderPolicyParamType<FUniformLightMapPolicyShaderParametersType>*& PixelShader)
{
	switch (InLightMapPolicy->GetIndirectPolicy())
	{
	case LMP_CACHED_VOLUME_INDIRECT_LIGHTING:
		GetConeTracingPixelShader<TUniformLightMapPolicy<LMP_CACHED_VOLUME_INDIRECT_LIGHTING>>(InVertexFactory, InMaterialResource, nullptr, PixelShader);
		break;
	case LMP_CACHED_POINT_INDIRECT_LIGHTING:
		GetConeTracingPixelShader<TUniformLightMapPolicy<LMP_CACHED_POINT_INDIRECT_LIGHTING>>(InVertexFactory, InMaterialResource, nullptr, PixelShader);
		break;
	case LMP_SIMPLE_NO_LIGHTMAP:
		GetConeTracingPixelShader<TUniformLightMapPolicy<LMP_SIMPLE_NO_LIGHTMAP>>(InVertexFactory, InMaterialResource, nullptr, PixelShader);
	case LMP_SIMPLE_LIGHTMAP_ONLY_LIGHTING:
		GetConeTracingPixelShader<TUniformLightMapPolicy<LMP_SIMPLE_LIGHTMAP_ONLY_LIGHTING>>(InVertexFactory, InMaterialResource, nullptr, PixelShader);
	case LMP_SIMPLE_DIRECTIONAL_LIGHT_LIGHTING:
		GetConeTracingPixelShader<TUniformLightMapPolicy<LMP_SIMPLE_DIRECTIONAL_LIGHT_LIGHTING>>(InVertexFactory, InMaterialResource, nullptr, PixelShader);
	case LMP_SIMPLE_STATIONARY_PRECOMPUTED_SHADOW_LIGHTING:
		GetConeTracingPixelShader<TUniformLightMapPolicy<LMP_SIMPLE_STATIONARY_PRECOMPUTED_SHADOW_LIGHTING>>(InVertexFactory, InMaterialResource, nullptr, PixelShader);
	case LMP_SIMPLE_STATIONARY_SINGLESAMPLE_SHADOW_LIGHTING:
		GetConeTracingPixelShader<TUniformLightMapPolicy<LMP_SIMPLE_STATIONARY_SINGLESAMPLE_SHADOW_LIGHTING>>(InVertexFactory, InMaterialResource, nullptr, PixelShader);
		break;
	case LMP_LQ_LIGHTMAP:
		GetConeTracingPixelShader<TUniformLightMapPolicy<LMP_LQ_LIGHTMAP>>(InVertexFactory, InMaterialResource, nullptr, PixelShader);
		break;
	case LMP_HQ_LIGHTMAP:
		GetConeTracingPixelShader<TUniformLightMapPolicy<LMP_HQ_LIGHTMAP>>(InVertexFactory, InMaterialResource, nullptr, PixelShader);
		break;
	case LMP_DISTANCE_FIELD_SHADOWS_AND_HQ_LIGHTMAP:
		GetConeTracingPixelShader<TUniformLightMapPolicy<LMP_DISTANCE_FIELD_SHADOWS_AND_HQ_LIGHTMAP>>(InVertexFactory, InMaterialResource, nullptr, PixelShader);
		break;
	case LMP_NO_LIGHTMAP:
		GetConeTracingPixelShader<TUniformLightMapPolicy<LMP_NO_LIGHTMAP>>(InVertexFactory, InMaterialResource, nullptr, PixelShader);
		break;
	default:										
		check(false);
	}
}


void TVXGIVoxelizationDrawingPolicyFactory::AddStaticMesh(FRHICommandList& RHICmdList, FScene* Scene, FStaticMesh* StaticMesh)
{
	const FMaterial* Material = StaticMesh->MaterialRenderProxy->GetMaterial(Scene->GetFeatureLevel());
	if (IsMaterialIgnored(StaticMesh->MaterialRenderProxy, Scene->GetFeatureLevel()))
	{
		return;
	}

	Scene->VxgiVoxelizationDrawList.AddMesh(
		StaticMesh,
		typename TVXGIVoxelizationDrawingPolicy<FVXGIVoxelizationNoLightMapPolicy>::ElementDataType(StaticMesh->PrimitiveSceneInfo),
		TVXGIVoxelizationDrawingPolicy<FVXGIVoxelizationNoLightMapPolicy>(
			StaticMesh->VertexFactory,
			StaticMesh->MaterialRenderProxy,
			*Material,
			ComputeMeshOverrideSettings(*StaticMesh),
			FVXGIVoxelizationNoLightMapPolicy()
			),
		Scene->GetFeatureLevel()
	);
}

bool TVXGIVoxelizationDrawingPolicyFactory::DrawDynamicMesh(
	FRHICommandList& RHICmdList,
	const FViewInfo& View,
	ContextType DrawingContext,
	const FMeshBatch& Mesh,
	bool bPreFog,
	FDrawingPolicyRenderState& DrawRenderState,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	FHitProxyId HitProxyId
	)
{
	const FMaterial* Material = Mesh.MaterialRenderProxy->GetMaterial(View.GetFeatureLevel());
	if (IsMaterialIgnored(Mesh.MaterialRenderProxy, View.GetFeatureLevel()))
	{
		return false;
	}

	TVXGIVoxelizationDrawingPolicy<FVXGIVoxelizationNoLightMapPolicy> DrawingPolicy(
		Mesh.VertexFactory,
		Mesh.MaterialRenderProxy,
		*Material,
		ComputeMeshOverrideSettings(Mesh),
		FVXGIVoxelizationNoLightMapPolicy()
	);

	DrawingPolicy.SetSharedState(RHICmdList, DrawRenderState, &View, typename TVXGIVoxelizationDrawingPolicy<FVXGIVoxelizationNoLightMapPolicy>::ContextDataType());

	for (int32 BatchElementIndex = 0; BatchElementIndex < Mesh.Elements.Num(); BatchElementIndex++)
	{
		TDrawEvent<FRHICommandList> MeshEvent;
		BeginMeshDrawEvent(RHICmdList, PrimitiveSceneProxy, Mesh, MeshEvent, EnumHasAnyFlags(EShowMaterialDrawEventTypes(GShowMaterialDrawEventTypes), EShowMaterialDrawEventTypes::VxgiVoxelization));

		DrawingPolicy.SetMeshRenderState(
			RHICmdList,
			View,
			PrimitiveSceneProxy,
			Mesh,
			BatchElementIndex,
			DrawRenderState,
			typename TVXGIVoxelizationDrawingPolicy<FVXGIVoxelizationNoLightMapPolicy>::ElementDataType(nullptr),
			typename TVXGIVoxelizationDrawingPolicy<FVXGIVoxelizationNoLightMapPolicy>::ContextDataType()
		);

		DrawingPolicy.DrawMesh(RHICmdList, View, Mesh, BatchElementIndex, false);
	}

	return true;
}

#endif
// NVCHANGE_END: Add VXGI
