/*
* Copyright (c) 2012-2018, NVIDIA CORPORATION. All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto. Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef GFSDK_VXGI_H_
#define GFSDK_VXGI_H_

/*
* Maxwell [*]: in this file, the name Maxwell refers to NVIDIA GPU architecture used in GM20x and later chips.
* Current products using these chips are GeForce GTX 960, 970 and 980, based on GM204 and GM206 GPUs.
* GM10x GPUs also have the architecture name "Maxwell", but they do not have the hardware features required to accelerate VXGI.
*/

// Define this symbol to 1 in order to replace the static DLL binding with function pointers obtained at run time.
#ifndef VXGI_DYNAMIC_LOAD_LIBRARY
#define VXGI_DYNAMIC_LOAD_LIBRARY 0
#endif

#ifndef CONFIGURATION_TYPE_DLL
#define CONFIGURATION_TYPE_DLL 0
#endif

#if VXGI_DYNAMIC_LOAD_LIBRARY
#include <Windows.h>
#endif

#include "GFSDK_NVRHI.h"
#include "GFSDK_VXGI_MathTypes.h"

#define VXGI_FAILED(status) ((status) != ::VXGI::Status::OK)
#define VXGI_SUCCEEDED(status) ((status) == ::VXGI::Status::OK)
#define VXGI_VERSION_STRING "2.0.0.23960390"

GI_BEGIN_PACKING
namespace VXGI
{
    struct Version
	{
		Version() : Major(2), Minor(0), Branch(0), Revision(23960390)
        { }

        uint32_t Major;
        uint32_t Minor;
        uint32_t Branch;
        uint32_t Revision;
    };

    struct Status
    {
        enum Enum
        {
            OK = 0,
            WRONG_INTERFACE_VERSION = 1,

            D3D_COMPILER_UNAVAILABLE,
            INSUFFICIENT_BINDING_SLOTS,
            INTERNAL_ERROR,
            INVALID_ARGUMENT,
            INVALID_CONFIGURATION,
            INVALID_SHADER_BINARY,
            INVALID_SHADER_SOURCE,
            INVALID_STATE,
            NULL_ARGUMENT,
            RESOURCE_CREATION_FAILED,
            SHADER_COMPILATION_ERROR,
            SHADER_MISSING,
            FUNCTION_MISSING,
            BUFFER_TOO_SMALL,
            NOT_SUPPORTED
        };
    };

    struct DebugRenderMode
    {
        enum Enum
        {
            DISABLED = 0,
            ALLOCATION_MAP,
            OPACITY_TEXTURE,
            EMITTANCE_TEXTURE,
            INDIRECT_IRRADIANCE_TEXTURE
        };
    };

    struct VoxelSizeFunction
    {
        enum Enum
        {
            EXACT = 0,
            LINEAR_UNDERESTIMATE = 1,
            LINEAR_OVERESTIMATE = 2
        };
    };

    struct SsaoParamaters
    {
        float       surfaceBias;
        float       radiusWorld;
        float       backgroundViewDepth;
        float       scale;
        float       powerExponent;

        SsaoParamaters()
            : surfaceBias(0.2f)
            , radiusWorld(50.f)
            , backgroundViewDepth(1000.f)
            , scale(1.f)
            , powerExponent(2.f)
        {
        }
    };

    struct TracingResolution
    {
        enum Enum
        {
            FULL = 1,
            HALF = 2,
            THIRD = 3, 
            QUARTER = 4
        };
    };

    struct LightLeakingAmount
    {
        enum Enum
        {
            MINIMAL = 0,
            MODERATE = 1,
            HEAVY = 2
        };
    };
    
    struct AreaLight 
    {
        // Some data that uniquely identifies a particular light and does not change between frames.
        // Required for temporal filters to work properly.
        void*       identifier;

        // Texture to be applied to the light. Can be NULL, which means the light has solid color.
        // The texture has to be a complete MIP chain.
        NVRHI::TextureHandle texture;

        // Center of the light in world space.
        float3      center;

        // One axis of the light rectangle, i.e. a vector parallel to an edge of the rectangle, whose length is equal to half of that edge.
        float3      majorAxis;

        // Another axis of the light rectangle. 
        // minorAxis and majorAxis should be orthogonal.
        float3      minorAxis;

        // Color of the light. Can be any positive value.
        // Color is multiplied by diffuseIntensity or specularIntensity, and by samples from the light texture, to determine the final outgoing radiance of the light.
        float3      color;

        // Distance from the light plane where irradiance becomes zero.
        // Such attenuation is not physically correct, but it improves performance by limiting the lights' volume of influence.
        // 0 means no attenuation.
        float       attenuationRadius;

        // Radial length of the region where irradiance is transitioned from physically correct values to zero.
        float       transitionLength;

        // Color multiplier for diffuse lighting.
        // Set diffuseIntensity to 0 to prevent diffuse lighting and occlusion from being calculated.
        float       diffuseIntensity;

        // Color multiplier for specular lighting.
        // Set specularIntensity to 0 to prevent specular lighting and occlusion from being calculated.
        float       specularIntensity;

        // Overall quality of the result, [0..1]. 
        // Higher quality means lower performance through a larger number of narrower cones being traced.
        float       quality;

        // Multiplier for the number of cones being traced towards a light, [0.5 .. 2]
        // Using temporal reprojection means that lower sampling rates can be used with approximately the same quality.
        float       directionalSamplingRate;

        // Maximum fraction of hemisphere around a surface covered by the area light when the number of cones stops to grow, [0.125 .. 1]
        // If the light covers a larger fraction of the hemisphere, the maximum number of cones is used, but they become wider.
        float       maxProjectedArea;

        // Distance from the area light surface when the occlusion cones stop, [0 .. 1], in multiples of the voxel sample size at the area light.
        // If the light or a surface behind it is voxelized, it will produce self-occlusion. Using a larger offset mitigates or prevents this self-occlusion.
        float       lightSurfaceOffset;

        // Controls whether the light has to be rendered with occlusion or without (the latter is significantly faster).
        bool        enableOcclusion;

        // Weight of occlusion and texture sampling data from the previous frame, [0 .. 1).
        float       temporalReprojectionWeight;

        // Controls the tradeoff between noise in high-detail areas (0) and temporal trailing under dynamic lighting (1).
        // Not really effective when NCC is used.
        float       temporalReprojectionDetailReconstruction;

        // Controls whether neighborhood color clamping (NCC) should be used in the temporal reprojection filter.
        // NCC greatly reduces trailing artifacts, but adds some noise in high-detail areas.
        bool        enableNeighborhoodClamping;

        // Controls the variance in color that's allowed during NCC.
        float       neighborhoodClampingWidth;

        // Enables screen-space occluder search between the surface and the first voxel sample.
        // Requires ViewInfo::matricesAreValid = true and the matrices to be set.
        bool        enableScreenSpaceOcclusion;

        // Quality of the screen space occlusion, [0..1].
        float       screenSpaceOcclusionQuality;

        // Enables generation of shadows that reflect which part of the light's texture is occluded.
        bool        texturedShadows;

        // Enables the use of a bicubic B-spline filter to sample the texture, instead of bilinear.
        // Produces smoother lighting, especially fine specular reflections, at a small performance cost.
        bool        highQualityTextureFilter;

        AreaLight()
            : identifier(nullptr)
            , center(0, 0, 0)
            , majorAxis(1, 0, 0)
            , minorAxis(0, 0, 1)
            , color(1.f)
            , diffuseIntensity(1.f)
            , specularIntensity(1.f)
            , texture(nullptr)
            , quality(0.5f)
            , directionalSamplingRate(1.0f)
            , maxProjectedArea(0.75f)
            , lightSurfaceOffset(0.5f)
            , enableOcclusion(true)
            , temporalReprojectionWeight(0.9f)
            , temporalReprojectionDetailReconstruction(0.5f)
            , enableNeighborhoodClamping(true)
            , neighborhoodClampingWidth(1.f)
            , enableScreenSpaceOcclusion(false)
            , screenSpaceOcclusionQuality(0.25f)
            , attenuationRadius(0.f)
            , transitionLength(1.f)
            , texturedShadows(true)
            , highQualityTextureFilter(true)
        { }
    };

    struct DiffuseTracingParameters
    {
        // Resolution for the primary cone tracing pass.
        // Using lower resolutions (half etc.) greatly improves performance in exchange for some fine detail quality.
        TracingResolution::Enum    tracingResolution;

        // Overall quality of the result, [0..1]. 
        // Higher quality means lower performance through a larger number of narrower cones being traced.
        float       quality;


        // Fraction of hemisphere around each surface to be sampled on one frame, [0.25..1]
        // Lower sampling rates greatly improve performance but produce more noise, which can be reduced using temporal filtering.
        float       directionalSamplingRate;

        // Softness of diffuse highlights, [0..1].
        float       softness;
        
        // Enables per-frame movement of shading points on the screen. Should be used if some kind of temporal filter is applied.
        bool        enableTemporalJitter;

        // Enables reuse of diffuse tracing results from the previous frame.
        // For this mode to work, different G-buffer textures have to be used for consecutive frames,
        // at least the depth and normal channels. Otherwise the tracing calls will fail.
        bool        enableTemporalReprojection;

        // Weight of the reprojected irradiance data relative to newly computed data, (0..1),
        // where 0 means do not use reprojection, and values closer to 1 mean accumulate data over more previous frames.
        // Higher values result in better filtering of random rotation/offset noise, but also introduce a lag of GI from moving objects.
        // 1.0 or higher will produce an unstable result.
        float       temporalReprojectionWeight;

        // Maximum distance between two samples for which they're still considered to be the same surface, expressed in voxels.
        float       temporalReprojectionMaxDistanceInVoxels;

        // The exponent used for the dot product of old and new normals in the temporal reprojection filter
        float       temporalReprojectionNormalWeightExponent;

        // Controls the tradeoff between noise in high-detail areas (0) and temporal trailing under dynamic lighting (1).
        float       temporalReprojectionDetailReconstruction;

        // Controls whether neighborhood color clamping (NCC) should be used in the temporal reprojection filter.
        // NCC greatly reduces trailing artifacts, but adds some noise in high-detail areas.
        bool        enableNeighborhoodClamping;

        // Controls the variance in color that's allowed during NCC.
        float       neighborhoodClampingWidth;

        // Minimum pixel weight for sparse tracing interpolation. When a pixel is below that weight, it is considered to be
        // a hole and submitted for refinement. When refinement is disabled, such pixels will be black with zero confidence.
        // Using a higher threshold improves image quality and reduces temporal noise at the cost of refinement pass performance.
        // Clamped to [0, 1].
        float       interpolationWeightThreshold;

        // Enables view reprojection, i.e. reuse of samples from one view in a different view within the same frame.
        // The values for ViewInfo::reprojectionSourceViewIndex must be set correctly for this feature to work.
        bool        enableViewReprojection;

        // If the weight of the sample reprojected from a different view is greater than this threshold,
        // the reprojected color is used, and tracing is not performed.
        float       viewReprojectionWeightThreshold;

        // Makes the cone tracing code use the accumulated opacity computed N steps back instead of the current value.
        // Three options are supported: Minimal, Moderate, and Heavy.
        // Minimal produces less light leaking, Heavy is the same setting that was used in VXGI 1.0, and Moderate is something in between.
        LightLeakingAmount::Enum lightLeakingAmount;

        DiffuseTracingParameters()
            : tracingResolution(TracingResolution::QUARTER)
            , quality(0.5f)
            , directionalSamplingRate(1.0f)
            , softness(0.5f)
            , enableTemporalJitter(false)
            , enableTemporalReprojection(false)
            , temporalReprojectionWeight(0.9f)
            , temporalReprojectionMaxDistanceInVoxels(0.25f)
            , temporalReprojectionNormalWeightExponent(20.f)
            , temporalReprojectionDetailReconstruction(0.5f)
            , enableNeighborhoodClamping(true)
            , neighborhoodClampingWidth(1.f)
            , interpolationWeightThreshold(1e-4f)
            , enableViewReprojection(false)
            , viewReprojectionWeightThreshold(0.1f)
            , lightLeakingAmount(LightLeakingAmount::MINIMAL)
        {
        }
    };

    struct BasicDiffuseTracingParameters : public DiffuseTracingParameters
    {
        // Multiplier for the incoming light intensity.
        float       irradianceScale;

        // Tracing step.
        // Reasonable values [0.5, 1]
        // Sampling with lower step produces more stable results at Performance cost.
        float       tracingStep;

        // Opacity correction factor.
        // Reasonable values [0.1, 10]
        // Higher values produce more contrast rendering, overall picture looks darker.
        float       opacityCorrectionFactor;

        // World-space distance at which the contribution of geometry to ambient occlusion will be 10x smaller than near the surface.
        float       ambientRange;

        // Other parameters for the ambient term.
        // correctedConeAmbient = pow(saturate(cone.ambient * ambientScale + ambientBias, ambientPower))
        float       ambientScale;
        float       ambientBias;
        float       ambientPower;

        // Parameters that control the distance of the first sample from the surface
        float       initialOffsetBias;
        float       initialOffsetDistanceFactor;

        BasicDiffuseTracingParameters()
            : DiffuseTracingParameters()
            , irradianceScale(1.f)
            , tracingStep(0.5f)
            , opacityCorrectionFactor(1.f)
            , ambientRange(512.f)
            , ambientScale(1.f)
            , ambientBias(0.f)
            , ambientPower(1.f)
            , initialOffsetBias(2.f)
            , initialOffsetDistanceFactor(1.f)
        { 
        }
    };

    struct SpecularTracingParameters
    {
        enum Filter
        {
            FILTER_NONE,
            FILTER_TEMPORAL,
            FILTER_SIMPLE
        };

        // Selects the filter that is used on the specular surface after tracing in order to reduce noise introduced by cone jitter.
        Filter      filter;

        // Controls random per-pixel adjustment of initial tracing offsets for specular tracing, helps reduce banding.
        // 0 or less means no random offset is used, 1.0 is one tracing step at the surface location.
        float       perPixelRandomOffsetScale;

        // Enables per-frame changes of the noise pattern. Should be used if the the filters are enough to hide the noise.
        bool        enableTemporalJitter;

        // Enables jitter of sample positions along a cone. Should be used if the the filters are enough to hide the noise.
        bool        enableConeJitter;

        // Weight of the reprojected irradiance data relative to newly computed data, [0..1).
        // Only applicable if filter == FILTER_TEMPORAL.
        // Also see the comment for DiffuseTracingParameters::enableTemporalReprojection and temporalReprojectionWeight.
        float       temporalReprojectionWeight;

        // Maximum distance between two samples for which they're still considered to be the same surface, expressed in voxels.
        float       temporalReprojectionMaxDistanceInVoxels;

        // The exponent used for the dot product of old and new normals in the temporal reprojection filter
        float       temporalReprojectionNormalWeightExponent;

        // Enables view reprojection, i.e. reuse of samples from one view in a different view within the same frame.
        // The values for ViewInfo::reprojectionSourceViewIndex must be set correctly for this feature to work.
        bool        enableViewReprojection;

        // If the weight of the sample reprojected from a different view is greater than this threshold,
        // the reprojected color is used, and tracing is not performed.
        float       viewReprojectionWeightThreshold;

        // See the comment for lightLeakingAmount in DiffuseTracingParameters.
        LightLeakingAmount::Enum lightLeakingAmount;

        SpecularTracingParameters()
            : filter(FILTER_SIMPLE)
            , perPixelRandomOffsetScale(0.f)
            , enableTemporalJitter(false)
            , enableConeJitter(false)
            , temporalReprojectionWeight(0.8f)
            , temporalReprojectionMaxDistanceInVoxels(0.25f)
            , temporalReprojectionNormalWeightExponent(20.f)
            , enableViewReprojection(false)
            , viewReprojectionWeightThreshold(0.1f)
            , lightLeakingAmount(LightLeakingAmount::MINIMAL)
        { }
    };

    struct BasicSpecularTracingParameters : public SpecularTracingParameters
    {
        // Multiplier for the incoming light intensity.
        float       irradianceScale;

        // Tracing step.
        // Reasonable values [0.5, 1]
        // Sampling with lower step produces more stable results at Performance cost.
        float       tracingStep;

        // Opacity correction factor.
        // Reasonable values [0.1, 10]
        // Higher values produce more contrast rendering, overall picture looks darker.
        float       opacityCorrectionFactor;

        // Parameters that control the distance of the first sample from the surface
        float       initialOffsetBias;
        float       initialOffsetDistanceFactor;

        // Initial offset multiplier for cones which are close to being coplanar with 
        // the surface that they're being traced from. Reduces self-occlusion.
        float       coplanarOffsetFactor;

        BasicSpecularTracingParameters()
            : SpecularTracingParameters()
            , irradianceScale(1.f)
            , tracingStep(1.f)
            , opacityCorrectionFactor(1.f)
            , initialOffsetBias(2.f)
            , initialOffsetDistanceFactor(1.f)
            , coplanarOffsetFactor(5.f)
        {
        }
    };

    struct IndirectIrradianceMapTracingParameters
    {
        float       coneAngle;
        float       tracingStep;
        float       irradianceScale;
        float       opacityCorrectionFactor;
        LightLeakingAmount::Enum lightLeakingAmount;

        // Auto-normalization is a safeguard algorithm that attempts to prevent irradiance from blowing up.
        // In some cases, such as when the value of irradianceScale in this structure is too large, or when multiple 
        // surfaces with the same orientation, one behind another, receive indirect illumination,
        // multi-bounce tracing feedback factor may become greater than 1, which means irradiance will blow up 
        // exponentially, causing numeric overflows in the voxel textures (or saturation in case UNORM8 emittance is used).
        // The normalization algorithm analyzes average irradiance over multiple frames, and adjusts irradianceScale
        // when it detects that an exponential blow-up is happening.
        // Adjustment is done internally, not visible to the user, and the adjustment factor is reset to 1.0 when 
        // the value of irradianceScale in this structure is different from one used on the previous frame.
        bool        useAutoNormalization;

        // Hard limit for indirect irradiance values. Useful in cases when auto-normalization doesn't work properly.
        // When irradianceClampValue <= 0 or >= 65504.0, the internal limit is set to 65504.0 because that's the maximum
        // value representable as float16 that's not infinity.
        float       irradianceClampValue;

        IndirectIrradianceMapTracingParameters()
            : tracingStep(1.f)
            , irradianceScale(1.f)
            , opacityCorrectionFactor(1.f)
            , coneAngle(40.f)
            , useAutoNormalization(true)
            , irradianceClampValue(0.f)
            , lightLeakingAmount(LightLeakingAmount::MODERATE)
        { }
    };

    struct AreaLightTracingParameters
    {
        // Resolution for the diffuse cone tracing pass.
        TracingResolution::Enum    tracingResolution;

        // Enables per-frame movement of shading points on the screen. Should be used if some kind of temporal filter is applied.
        bool        enableTemporalJitter;

        // See DiffuseTracingParameters::enableTemporalReprojection
        bool        enableTemporalReprojection;

        // See DiffuseTracingParameters::temporalReprojectionMaxDistanceInVoxels
        float       temporalReprojectionMaxDistanceInVoxels;

        // See DiffuseTracingParameters::temporalReprojectionNormalWeightExponent
        float       temporalReprojectionNormalWeightExponent;

        // See DiffuseTracingParameters::interpolationWeightThreshold
        float       interpolationWeightThreshold;

        AreaLightTracingParameters()
            : tracingResolution(TracingResolution::QUARTER)
            , enableTemporalJitter(false)
            , enableTemporalReprojection(false)
            , temporalReprojectionMaxDistanceInVoxels(0.25f)
            , temporalReprojectionNormalWeightExponent(20.f)
            , interpolationWeightThreshold(1e-4f)
        {
        }
    };

    struct BasicAreaLightTracingParameters : public AreaLightTracingParameters
    {
        // Tracing step.
        // Reasonable values [0.5, 1]
        // Sampling with lower step produces more stable results at Performance cost.
        float       tracingStep;

        // Opacity correction factor.
        // Reasonable values [0.1, 10]
        // Higher values produce more contrast rendering, overall picture looks darker.
        float       opacityCorrectionFactor;

        // Parameters that control the distance of the first sample from the surface
        float       initialOffsetBias;
        float       initialOffsetDistanceFactor;

        // Initial offset multiplier for cones which are close to being coplanar with 
        // the surface that they're being traced from. Reduces self-occlusion.
        float       coplanarOffsetFactor;

        BasicAreaLightTracingParameters()
            : AreaLightTracingParameters()
            , tracingStep(1.f)
            , opacityCorrectionFactor(1.f)
            , initialOffsetBias(2.f)
            , initialOffsetDistanceFactor(1.f)
            , coplanarOffsetFactor(5.f)
        {
        }
    };

    struct HardwareFeatures
    {
        enum Enum
        {
            NONE = 0x00,

            NVAPI_FP16_ATOMICS = 0x01,
            NVAPI_FAST_GS = 0x02,
            NVAPI_TYPED_UAV_LOAD = 0x04,
            NVAPI_CONSERVATIVE_RASTER = 0x08,
            NVAPI_WAVE_MATCH = 0x10,
            NVAPI_QUAD_FILL = 0x20,
            TYPED_UAV_LOAD = 0x40,
            CONSERVATIVE_RASTER = 0x80,

            ALL_NVAPI_HLSL_EXTENSIONS = NVAPI_FP16_ATOMICS | NVAPI_TYPED_UAV_LOAD | NVAPI_WAVE_MATCH,
            ALL = 0xFF
        };
    };

    static inline HardwareFeatures::Enum operator |(HardwareFeatures::Enum a, HardwareFeatures::Enum b) { return HardwareFeatures::Enum(int(a) | int(b)); }
    static inline HardwareFeatures::Enum operator &(HardwareFeatures::Enum a, HardwareFeatures::Enum b) { return HardwareFeatures::Enum(int(a) & int(b)); }

    static inline void operator |=(HardwareFeatures::Enum& a, HardwareFeatures::Enum b) { a = a | b; }
    static inline void operator &=(HardwareFeatures::Enum& a, HardwareFeatures::Enum b) { a = a & b; }

    struct VoxelizationParameters
    {
        // Controls voxelization density, must be a power of 2 and within range [16, 256]
        uint3 mapSize;

        // Controls allocation granularity, bigger values result in coarser allocation map being used
        uint32_t allocationMapLodBias;

        // Number of levels in a clipmap stack used for scene representation.
        // Reasonable values [2, log2(MapSize) - 1]
        uint32_t stackLevels;

        // Number of levels in a mipmap stack used for scene representation.
        // These levels do not include stack levels, and there can be 0 of them.
        uint32_t mipLevels;

        // Controls whether opacity and emittance voxel data can be preserved between frames. 
        // If not, the whole clipmap will be automatically invalidated during the prepareForOpacityVoxelization call,
        // and some passes of the VXGI algorithm will become faster.
        bool persistentVoxelData;

        // Enables a mode where VXGI does not optimize the invalidation regions before processing them on the GPU.
        // Updates to voxel data on the GPU will still happen with one page granularity, but the application will
        // receive just one "optimized" region which is a union of all submitted regions, rounded to page size.
        // The value of simplifiedInvalidate does not matter if persistentVoxelData == false.
        bool simplifiedInvalidate;

        // Controls whether VXGI will store and process light information (false) or just occlusion (true).
        bool ambientOcclusionMode;

        // Global multiplier for emittance voxels - adjust it according to your light intensities to avoid clamping or quantization.
        // As long as neither of these effects takes place, changing this parameter has no effect on rendered images.
        float emittanceStorageScale;

        // Enables a mode wherein transitions from downsampled to directly voxelized emittance are smoothed
        // by blending the two with a factor that depends on the exact clipmap anchor, not the quantized clipmap center.
        // This mode requires persistentVoxelData = false to operate properly.
        bool useEmittanceInterpolation;

        // Enables a higher-order filter to be used during emittance downsampling.
        // Using this filter makes moving objects produce much smoother indirect illumination, at a significant performance cost.
        bool useHighQualityEmittanceDownsampling;

        // Controls whether a separate indirect irradiance 3D map is computed for use on the next frame.
        // In order to see multi-bounce lighting, call this function in the emittance voxelization pixel shader:
        //
        //    float3 VxgiGetIndirectIrradiance(float3 worldPos, float3 normal)
        //
        // Then shade the material with the returned indirect irradiance (don't forget to divide it by PI) and add the result to output color.
        bool enableMultiBounce;

        // Controls the size of the indirect irradiance map.
        // - indirectIrradianceMapLodBias == 0 means that the indirect irradiance map will have the same size and resolution 
        //    as the coarsest clipmap level.
        // - indirectIrradianceMapLodBias > 0 means that there will be fewer voxels, but not fewer than in the allocation map, 
        //    i.e. indirectIrradianceMapLodBias <= allocationMapLodBias.
        // - indirectIrradianceMapLodBias < 0 means that there will be more voxels, but not more than 256^3, and the resolution 
        //    will not be finer than the finest clipmap level, i.e. indirectIrradianceMapLodBias > -stackLevels.
        int32_t indirectIrradianceMapLodBias;

        // Informs VXGI about the features that may be used if the GPU supports them. 
        // By default, all features are enabled. Disable some if the NVRHI backend doesn't implement them.
        HardwareFeatures::Enum enabledHardwareFeatures;

        // Default constructor sets default parameters
        VoxelizationParameters()
            : mapSize(64)
            , allocationMapLodBias(0)
            , stackLevels(5)
            , mipLevels(5)
            , persistentVoxelData(true)
            , simplifiedInvalidate(true)
            , ambientOcclusionMode(false)
            , emittanceStorageScale(1.0f)
            , useEmittanceInterpolation(false)
            , useHighQualityEmittanceDownsampling(false)
            , enableMultiBounce(false)
            , indirectIrradianceMapLodBias(0)
            , enabledHardwareFeatures(HardwareFeatures::ALL)
        { }

        bool operator!=(const VoxelizationParameters& parameters) const
        {
            if (parameters.mapSize != mapSize ||
                parameters.allocationMapLodBias != allocationMapLodBias ||
                parameters.stackLevels != stackLevels ||
                parameters.mipLevels != mipLevels ||
                parameters.persistentVoxelData != persistentVoxelData ||
                parameters.simplifiedInvalidate != simplifiedInvalidate ||
                parameters.ambientOcclusionMode != ambientOcclusionMode ||
                parameters.emittanceStorageScale != emittanceStorageScale ||
                parameters.useEmittanceInterpolation != useEmittanceInterpolation ||
                parameters.useHighQualityEmittanceDownsampling != useHighQualityEmittanceDownsampling ||
                parameters.enableMultiBounce != enableMultiBounce ||
                parameters.indirectIrradianceMapLodBias != indirectIrradianceMapLodBias || 
                parameters.enabledHardwareFeatures != enabledHardwareFeatures
                ) return true;

            return false;
        }

        bool operator==(const VoxelizationParameters& parameters) const
        {
            return !(*this != parameters);
        }
    };

    struct TracedSamplesParameters
    {
        enum ColorMode
        {
            COLOR_MIP_LEVEL = 0,
            COLOR_EMITTANCE = 1,
            COLOR_OCCLUSION = 2,
            COLOR_TEXELS_LOWER_MIP = 3,
            COLOR_TEXELS_UPPER_MIP = 4,
        };

        ColorMode   colorMode;
        bool        onlyContributingSamples;
        int32_t     coneIndexFilter;
        int32_t     sampleIndexFilter;
        bool        showConeDirections;

        TracedSamplesParameters()
            : onlyContributingSamples(false)
            , coneIndexFilter(0)
            , sampleIndexFilter(0)
            , showConeDirections(false)
            , colorMode(COLOR_MIP_LEVEL)
        { }
    };

    class IUserDefinedShaderSet;

    struct MaterialInfo
    {
        IUserDefinedShaderSet*   pixelShader;
        IUserDefinedShaderSet*   geometryShader;

        // This flag controls the material sampling rate during emittance voxelization, basically the rasterization resolution.
        // Materials that have binary masks and other non-filterable elements should be voxelized with this flag enabled
        // in order to make emittance voxelized into coarse LODs consistent with emittance downsampled from finer LODs.
        bool adaptiveMaterialSamplingRate;

        MaterialInfo()
            : pixelShader(NULL)
            , geometryShader(NULL)
            , adaptiveMaterialSamplingRate(false)
        { }

        bool operator == (const MaterialInfo& b) const
        {
            return !(*this != b);
        }

        bool operator != (const MaterialInfo& b) const
        {
            return
                pixelShader != b.pixelShader ||
                geometryShader != b.geometryShader ||
                adaptiveMaterialSamplingRate != b.adaptiveMaterialSamplingRate;
        }
    };

    // Should be implemented by the application. 
    // Not essential to VXGI operation and is useful for performance measurements only.
    class IPerformanceMonitor
    {
        IPerformanceMonitor& operator=(const IPerformanceMonitor& other); //undefined
    protected:
        virtual ~IPerformanceMonitor() {};
    public:
        virtual void beginSection(const char* pSectionName) = 0;
        virtual void endSection() = 0;
    };

    struct PerformanceSectionsMode
    {
        enum Enum
        {
            NONE = 0,
            OVERVIEW = 1,
            DETAILED = 2
        };
    };

    // Should be implemented by the application.
    class IAllocator
    {
        IAllocator& operator=(const IAllocator& other); //undefined
    protected:
        virtual ~IAllocator() {};
    public:
        virtual void* allocateMemory(size_t size) = 0;
        virtual void freeMemory(void* ptr) = 0;
    };

    class IViewTracer
    {
        IViewTracer& operator=(const IViewTracer& other); //undefined
    protected:
        virtual ~IViewTracer() {};
    public:
        struct ViewInfo
        {
            NVRHI::Rect extents;

            // Projection parameters. Necessary for SSAO and screen-space shadows for area lights. Can be omitted otherwise.
            bool matricesAreValid;
            bool reverseProjection;
            float3 preViewTranslation;
            float4x4 viewMatrix;
            float4x4 projMatrix;

            // Index into the 'views' array for the view which should be used for view reprojection.
            // For view reprojection to work, this value should be non-negative and less than the current view index.
            // Otherwise, reprojection will be disabled for this view.
            int reprojectionSourceViewIndex;

            ViewInfo()
                : matricesAreValid(false)
                , reverseProjection(false)
                , reprojectionSourceViewIndex(-1)
            { }
        };
        
        // Computes the indirect diffuse illumination and ambient occlusion and returns the surfaces with it.
        // In VXAO mode, outDiffuseAndAmbient is a single-channel surface with ambient occlusion, [0..1];
        // In VXGI mode, outDiffuseAndAmbient is a 4-channel surface where RGB channels store indirect illumination 
        // and A channel stores ambient occlusion.
        // outConfidence is a single-channel surface that tells how much VXGI knows about a given pixel, [0..1].
        virtual Status::Enum computeDiffuseChannel(
            const DiffuseTracingParameters& params,
            const NVRHI::PipelineStageBindings& gbufferBindings,
            const int2 gbufferTextureSize,
            const ViewInfo* views,
            uint32_t numViews,
            NVRHI::TextureHandle& outDiffuseAndAmbient,
            NVRHI::TextureHandle& outConfidence) = 0;

        // Computes the indirect specular illumination and returns the surface containing it.
        virtual Status::Enum computeSpecularChannel(
            const SpecularTracingParameters& params,
            const NVRHI::PipelineStageBindings& gbufferBindings,
            const int2 gbufferSize,
            const ViewInfo* views,
            uint32_t numViews,
            NVRHI::TextureHandle& outSpecular) = 0;

        // Computes a screen-space ambient occlusion channel.
        virtual Status::Enum computeSsaoChannel(
            const SsaoParamaters& params,
            const NVRHI::PipelineStageBindings& gbufferBindings,
            const int2 gbufferSize,
            const ViewInfo* views,
            uint32_t numViews,
            NVRHI::TextureHandle& outSSAO) = 0;

        // Computes the diffuse and specular illumination from area lights and returns the surfaces containing those.
        virtual Status::Enum computeAreaLightChannels(
            const AreaLightTracingParameters& params,
            const NVRHI::PipelineStageBindings& gbufferBindings,
            const int2 gbufferTextureSize,
            const ViewInfo* views,
            uint32_t numViews,
            const AreaLight* pAreaLights,
            uint32_t numAreaLights,
            NVRHI::TextureHandle& outAreaLightDiffuse,
            NVRHI::TextureHandle& outAreaLightSpecular) = 0;

        // Initializes internal rendering state for a new frame. 
        // This function needs to be called once per frame, but not per view.
        virtual void beginFrame() = 0;

        // Render the "debug samples" visualization for the samples that were previously saved
        // using a saveSamplesFromNextCall call followed by compute[Something]Channel
        virtual Status::Enum renderSamplesDebug(
            NVRHI::TextureHandle destinationTexture,
            NVRHI::TextureHandle destinationDepth,
            const TracedSamplesParameters &params,
            const NVRHI::Viewport& viewport,
            const float3& preViewTranslation,
            const float4x4& viewMatrix,
            const float4x4& projMatrix) = 0;

        // Enables sample capture mode 
        virtual void saveSamplesFromNextCall(int x, int y) = 0;
    };

    class IBasicViewTracer : public virtual IViewTracer
    {
    public:
        struct InputBuffers
        {
            // Handles for G-buffer textures
            NVRHI::TextureHandle gbufferDepth;     // Depth buffer
            NVRHI::TextureHandle gbufferNormal;    // Normals (.xyz) and roughness (.w)

            // Parameters of the camera that was used to render the G-buffer
            float3 preViewTranslation;
            float4x4 viewMatrix;
            float4x4 projMatrix;

            // Viewport within the G-buffer textures
            NVRHI::Viewport gbufferViewport;

            // Scale and bias for decoding the contents of gbufferNormal texture.
            // The effective normal N is computed like this: N = normalize(gbufferNormal.xyz * gbufferNormalScale + gbufferNormalBias)
            float gbufferNormalScale;
            float gbufferNormalBias;

            InputBuffers()
                : gbufferNormalScale(1.0f)
                , gbufferNormalBias(0.0f)
                , gbufferViewport()
                , gbufferDepth(0)
                , gbufferNormal(0)
            {}
        };
        
        // Computes the indirect diffuse illumination and ambient occlusion and returns the surfaces with it.
        virtual Status::Enum computeDiffuseChannelBasic(
            const BasicDiffuseTracingParameters& params,
            const InputBuffers& inputBuffers,
            const InputBuffers* inputBuffersPreviousFrame,
            NVRHI::TextureHandle& outDiffuseAndAmbient,
            NVRHI::TextureHandle& outConfidence) = 0;

        // Computes the indirect specular illumination and returns the surface containing it.
        virtual Status::Enum computeSpecularChannelBasic(
            const BasicSpecularTracingParameters& params,
            const InputBuffers& inputBuffers,
            const InputBuffers* inputBuffersPreviousFrame,
            NVRHI::TextureHandle& outSpecular) = 0;

        // Computes a screen-space ambient occlusion channel.
        virtual Status::Enum computeSsaoChannelBasic(
            const SsaoParamaters& params,
            const InputBuffers& inputBuffers,
            NVRHI::TextureHandle& outSSAO) = 0;

        // Computes the diffuse and specular illumination from area lights and returns the surfaces containing those.
        virtual Status::Enum computeAreaLightChannelsBasic(
            const BasicAreaLightTracingParameters& params,
            const InputBuffers& inputBuffers,
            const InputBuffers* inputBuffersPreviousFrame,
            const AreaLight* pAreaLights,
            uint32_t numAreaLights,
            NVRHI::TextureHandle& outAreaLightDiffuse,
            NVRHI::TextureHandle& outAreaLightSpecular) = 0;
    };

    struct ShaderResources
    {
        enum { MAX_TEXTURE_BINDINGS = 128, MAX_SAMPLER_BINDINGS = 16, MAX_CB_BINDINGS = 15, MAX_UAV_BINDINGS = 64 };

        uint32_t textureSlots[MAX_TEXTURE_BINDINGS];
        uint32_t textureCount;

        uint32_t samplerSlots[MAX_SAMPLER_BINDINGS];
        uint32_t samplerCount;

        uint32_t constantBufferSlots[MAX_CB_BINDINGS];
        uint32_t constantBufferCount;

        uint32_t unorderedAccessViewSlots[MAX_UAV_BINDINGS];
        uint32_t unorderedAccessViewCount;

        ShaderResources()
            : textureCount(0)
            , samplerCount(0)
            , constantBufferCount(0)
            , unorderedAccessViewCount(0)
        {
            memset(textureSlots, 0, sizeof(textureSlots));
            memset(samplerSlots, 0, sizeof(samplerSlots));
            memset(constantBufferSlots, 0, sizeof(constantBufferSlots));
            memset(unorderedAccessViewSlots, 0, sizeof(unorderedAccessViewSlots));
        }
    };

    // This structure describes the attributes that have to be passed through the voxelization geometry shader.
    // The geometry shader code is generated by VXGI and is not available to the application.
    struct VoxelizationGeometryShaderDesc
    {
        enum { MAX_NAME_LENGTH = 512, MAX_ATTRIBUTE_COUNT = 32 };
        enum AttributeType
        {
            FLOAT_ATTR,
            INT_ATTR,
            UINT_ATTR,
        };

        struct Attribute
        {
            AttributeType type;
            uint32_t width, semanticIndex;
            char name[MAX_NAME_LENGTH];
            char semantic[MAX_NAME_LENGTH];

            Attribute() : type(FLOAT_ATTR), width(4), semanticIndex(0)
            {
                memset(name, 0, sizeof(name));
                memset(semantic, 0, sizeof(semantic));
            }
        };
        uint32_t pixelShaderInputCount;
        Attribute pixelShaderInputs[MAX_ATTRIBUTE_COUNT];

        VoxelizationGeometryShaderDesc() : pixelShaderInputCount(0)
        {
        }
    };

    // This structure describes a user-defined triangle culling function, presented in HLSL source code.
    // Such function is inserted into the VXGI voxelization geometry shader and can discard triangles early in the pipeline,
    // based on vertex positions and/or triangle normal. For example, when voxelizing directly lit geometry,
    // the culling function can test whether the triangle intersects with the light frustum and is facing the light.
    // The function should be defined to match the following prototype: 
    // 
    //    bool CullTriangle(float3 v1, float3 v2, float3 v3, float3 normal) { ... }
    //
    // - v1, v2, v3 are vertex coordinates in the same order as they're received by the GS;
    // - normal is the triangle normal computed as normalize(cross(v1 - v0, v2 - v0));
    // - returns true if the triangle has to be discarded, and false if it has to be voxelized.
    // 
    struct VoxelizationGeometryShaderCullFunctionDesc
    {
        // Pointer to the culling function source code in HLSL, not necessarily ending with a zero
        const char* sourceCode;

        // Size of the source code, in bytes
        size_t sourceCodeSize;

        // Resources such as constant buffers or textures that are used by the culling function.
        // These resources have to be bound by the application after applying the voxelization state
        // that is produced by IGlobalIllumination::getVoxelizationState(...) function.
        ShaderResources resources;
    };

    // A container for binary data
    class IBlob
    {
        IBlob& operator=(const IBlob& other); //undefined
    protected:
        //The user cannot delete this
        virtual ~IBlob() {};
    public:
        virtual const void* getData() const = 0;
        virtual size_t getSize() const = 0;
        //the caller is finished with this object and it can be destroyed
        virtual void dispose() = 0;
    };

    class IUserDefinedShaderSet
    {
        IUserDefinedShaderSet& operator=(const IUserDefinedShaderSet& other); //undefined
    protected:
        virtual ~IUserDefinedShaderSet() {};
    public:
        enum ShaderType
        {
            VOXELIZATION_GEOMETRY_SHADER,
            VOXELIZATION_PIXEL_SHADER,
            CONE_TRACING_PIXEL_SHADER,
            CONE_TRACING_COMPUTE_SHADER,
            VIEW_TRACING_SHADERS
        };

        virtual ShaderType getType() = 0;

        //There could be multiple versions of this shader inside
        virtual uint32_t getPermutationCount() = 0;
        virtual NVRHI::ShaderHandle getApplicationShaderHandle(uint32_t permutation) = 0;
    };

    struct UpdateVoxelizationParameters
    {
        // Anchor is the point around which the clipmap center is located - it is snapped to a grid.
        // For first-person cameras, the anchor should be placed slightly ahead of the camera, e.g. at (eyePosition + eyeDirection * finestVoxelSize * average(mapSize) * 0.25).
        float3 clipmapAnchor;

        // Scene bounding box in world space - used to stop cone tracing when cones exit the scene
        Box3f sceneExtents;

        // Size of a voxel in the finest clipmap level, in world units
        float finestVoxelSize;

        // A set of world-space boxes that contain some geometry that changed since the previous frame
        const Box3f* invalidatedRegions;

        // The number of boxes in 'invalidatedRegions'
        uint32_t invalidatedRegionCount;

		// A set of world-space frusta for lights which have been moved or otherwise changed.
		// All emittance data in pages intersecting the frusta will be cleared.
		const Frustum* invalidatedLightFrusta;

		// The number of frusta in 'invalidatedLightFrusta'
		uint32_t invalidatedFrustumCount;

        // Parameters that control the cone tracing process used for the indirect irradiance map.
        // The nearClipZ, farClipZ and debugParameters members of CommonTracingParameters are ignored here, all others are effective.
        IndirectIrradianceMapTracingParameters indirectIrradianceMapTracingParameters;

        UpdateVoxelizationParameters()
            : clipmapAnchor(0.f)
            , sceneExtents(float3(FLT_MIN), float3(FLT_MAX))
            , finestVoxelSize(1.0f)
            , invalidatedRegions(NULL)
            , invalidatedRegionCount(0)
            , invalidatedLightFrusta(NULL)
            , invalidatedFrustumCount(0)
        { }
    };

    struct DebugRenderParameters
    {
        // Which texture?
        DebugRenderMode::Enum debugMode;

        // Camera paramaters
        float3 preViewTranslation;
        float4x4 viewMatrix;
        float4x4 projMatrix;

        NVRHI::Viewport viewport;

        // Required
        NVRHI::TextureHandle destinationTexture;

        // Optional - use it to correctly overlay the voxels over the scene rendering
        NVRHI::TextureHandle destinationDepth;

        NVRHI::BlendState blendState;
        NVRHI::DepthStencilState depthStencilState;

        // Opacity that will be written into the .a channel of destinationTexture for covered pixels
        float targetOpacity;

        // Level of detail to visualize (only relevant for opacity and emittance views)
        // -1 means "all clipmap levels"
        // 0 .. (stackLevels - 1) are clipmap levels
        // stackLevels .. (stackLevels + mipLevels - 1) are mipmap levels
        // Anything else is invalid and will draw nothing.
        int level;

        // Allocation map bit index to visualize (for the allocation map view)
        uint32_t bitToDisplay;

        // Number of voxel faces to look through
        uint32_t voxelsToSkip;

        // Projection parameters
        float nearClipZ;
        float farClipZ;

        // Color that will be used for near-zero opacity voxels
        float3 opacityLowColor;

        // Color that will be used for fully opaque voxels
        float3 opacityHighColor;

        DebugRenderParameters()
            : debugMode(DebugRenderMode::DISABLED)
            , destinationTexture(NULL)
            , destinationDepth(NULL)
            , targetOpacity(1.0f)
            , level(-1)
            , bitToDisplay(0)
            , voxelsToSkip(0)
            , nearClipZ(0.0f)
            , farClipZ(1.0f)
            , opacityLowColor(0.f, 0.f, 1.f)  // blue
            , opacityHighColor(1.f, 0.f, 0.f) // red
        { }
    };

    class IShaderCompiler
    {
    public:
        // Functions that create a voxelization geometry shader based on..
        // - A descriptor listing the attributes to pass through the GS
        virtual Status::Enum compileVoxelizationGeometryShader(IBlob** ppBlob, const VoxelizationGeometryShaderDesc& desc, const VoxelizationGeometryShaderCullFunctionDesc* cullFunction = NULL) = 0;
        // - A vertex shader; shader reflection is used to get the list of attributes
        virtual Status::Enum compileVoxelizationGeometryShaderFromVS(IBlob** ppBlob, const void* binary, size_t binarySize, const VoxelizationGeometryShaderCullFunctionDesc* cullFunction = NULL) = 0;
        // - A domain shader; shader reflection is used to get the list of attributes
        virtual Status::Enum compileVoxelizationGeometryShaderFromDS(IBlob** ppBlob, const void* binary, size_t binarySize, const VoxelizationGeometryShaderCullFunctionDesc* cullFunction = NULL) = 0;

        // Creates a default voxelization PS that is only useful for opacity voxelization of opaque geometry.
        // When used for emittance voxelization, this shader produces flat white emissive geometry.
        virtual Status::Enum compileVoxelizationDefaultPixelShader(IBlob** ppBlob) = 0;

        // Creates a custom voxelization PS that can be used for both opacity and emittance voxelization.
        // The source code provided by the user should call the following function:
        //
        //    void VxgiStoreVoxelizationData(VxgiVoxelizationPSInputData inputData, float3 worldNormal, float opacity, float3 emissiveColorFront, float3 emissiveColorBack)
        //
        // - inputData should be received as PS input attributes.
        // - worldNormal should be normalized; it can be a flat geometry normal or actual normal used for shading.
        // - opacity, emissiveColorFront and emissiveColorBack can be computed as required.
        //
        // The function:
        //     bool VxgiCanWriteEmittance();
        // can be used to determine the parameters of the current voxelization pass, i.e. what has been passed to 
        // the getVoxelizationState(...) function. In VXAO mode, VxgiCanWriteEmittance() always returns false.
        // This function can be used to skip some unnecessary calculations in the user code, like shading.
        //
        // VxgiGetIndirectIrradiance(...) function is also available to the user code to get the indirect irradiance,
        // see the comments for VoxelizationParameters::multiBounceMode for more details.
        // 
        virtual Status::Enum compileVoxelizationPixelShader(IBlob** ppBlob, const char* source, size_t sourceSize, const char* entryFunc, const ShaderResources& userShaderCodeResources) = 0;

        // Functions that create user defined shaders that use VXGI cone tracing functions.
        // Cone tracing is defined this way:
        // 
        //    struct VxgiConeTracingArguments 
        //    {
        //      float3 firstSamplePosition;       // world space position of the first cone sample
        //      float3 direction;                 // direction of the cone
        //      float coneFactor;                 // sampleSize = t * coneFactor
        //      float tracingStep;                // see comment for CommonTracingParameters::tracingStep
        //      float firstSampleT;               // ray parameter (t) value for the first sample, in voxels
        //      float maxTracingDistance;         // maximum distance to trace from surface, in world units, default is 0.0 = no limit
        //      float opacityCorrectionFactor;    // see comment for CommonTracingParameters::opacityCorrectionFactor
        //      float emittanceScale;             // multiplier for the irradiance result, default is 1.0
        //      float initialOpacity;             // opacity accumulated elsewhere before starting the cone, default is 0.0
        //      float ambientAttenuationFactor;   // ambient contribution = exp(-t * ambientAttenuationFactor)
        //      float maxSamples;                 // limits the number of samples to take
        //      bool enableSceneBoundsCheck;      // should the cone be terminated when it leaves the scene bounding box
        //    };
        //  
        //    struct VxgiConeTracingResults
        //    {
        //      float3 irradiance;                // irradiance from the cone
        //      float ambient;                    // amount of ambient lighting from the cone, normalized to [0, 1]
        //      float finalOpacity;               // opacity accumulated by the cone
        //      float sampleCount;                // number of samples taken for the cone
        //    };
        //
        //    VxgiConeTracingResults VxgiTraceCone(VxgiConeTracingArguments args);
        //
        // Use this function to initialize all VxgiConeTracingArguments fields with default values:
        //
        //    VxgiConeTracingArguments VxgiDefaultConeTracingArguments();
        //
        virtual Status::Enum compileConeTracingPixelShader(IBlob** ppBlob, const char* source, size_t sourceSize, const char* entryFunc, const ShaderResources& userShaderCodeResources) = 0;
        virtual Status::Enum compileConeTracingComputeShader(IBlob** ppBlob, const char* source, size_t sourceSize, const char* entryFunc, const ShaderResources& userShaderCodeResources) = 0;

        // Creates a set of shaders for performing diffuse and specular cone tracing, as well as SSAO effect rendering,
        // for a G-buffer with user-defined format and projection type.
        // The user-supplied source code has to define the following structures and functions:
        // 
        //    // A structure that you can use to keep any information in - it's passed from VxgiLoadGBufferSample to all other functions
        //    struct VxgiGBufferSample; 
        //
        //    // Loads a sample from the current or previous G-buffer at given window coordinates.
        //    // If onlyPosition is true, the function should skip loading and processing anything else, for performance.
        //    bool VxgiLoadGBufferSample(float2 windowPos, uint viewIndex, bool previous, bool onlyPosition, out VxgiGBufferSample result);
        //
        //    // Returns the view depth or distance to camera for a given sample
        //    float VxgiGetGBufferViewDepth(VxgiGBufferSample gbufferSample);
        //
        //    // Returns the world position of a given sample
        //    float3 VxgiGetGBufferWorldPos(VxgiGBufferSample gbufferSample);
        //
        //    // Returns the world space normal for a given sample
        //    float3 VxgiGetGBufferNormal(VxgiGBufferSample gbufferSample);
        //
        //    // Returns the world space tangent for a given sample.
        //    // Tangents are used by VXGI to calculate directions for diffuse cones. Using tangents that originate from meshes
        //    // can help avoid seams on diffuse illumination where computed tangents have a discontinuity.
        //    float3 VxgiGetGBufferTangent(VxgiGBufferSample gbufferSample);
        //
        //    // Returns the diffuse/ambient tracing settings for the current sample.
        //    // The return structure is defined as follows:
        //    // 
        //    //    struct VxgiDiffuseShaderParameters
        //    //    {
        //    //        bool        enable;
        //    //        float       irradianceScale;
        //    //        float       tracingStep;
        //    //        float       opacityCorrectionFactor;
        //    //        float       ambientRange;
        //    //        float       ambientScale;
        //    //        float       ambientBias;
        //    //        float       ambientPower;
        //    //        float       initialOffsetBias;
        //    //        float       initialOffsetDistanceFactor;
        //    //    };
        //    // 
        //    // An instance of this structure with all-default fields can be obtained by calling VxgiGetDefaultDiffuseShaderParameters().
        //    // 
        //    VxgiDiffuseShaderParameters VxgiGetGBufferDiffuseShaderParameters(VxgiGBufferSample gbufferSample)
        //
        //    // Returns the specular tracing settings for the current sample.
        //    // The return structure is defined as follows:
        //    // 
        //    //    struct VxgiSpecularShaderParameters
        //    //    {
        //    //        bool        enable;
        //    //        float3      viewDirection;
        //    //        float       irradianceScale;
        //    //        float       roughness;
        //    //        float       tracingStep;
        //    //        float       opacityCorrectionFactor;
        //    //        float       initialOffsetBias;
        //    //        float       initialOffsetDistanceFactor;
        //    //        float       coplanarOffsetFactor;
        //    //    };
        //    // 
        //    VxgiSpecularShaderParameters VxgiGetGBufferSpecularShaderParameters(VxgiGBufferSample gbufferSample)
        //
        //    // Returns the area light settings for the current sample.
        //    // The return structure is defined as follows:
        //    //
        //    //    struct VxgiAreaLightShaderParameters
        //    //    {
        //    //        bool        enableDiffuse;
        //    //        bool        enableSpecular;
        //    //        float3      viewDirection;
        //    //        float       roughness;
        //    //        float       tracingStep;
        //    //        float       opacityCorrectionFactor;
        //    //        float       initialOffsetBias;
        //    //        float       initialOffsetDistanceFactor;
        //    //        float       coplanarOffsetFactor;
        //    //    };
        //    // 
        //    VxgiAreaLightShaderParameters VxgiGetGBufferAreaLightShaderParameters(VxgiGBufferSample gbufferSample)
        //
        //    // Return true if SSAO  should be computed for the sample;
        //    // VXGI assumes that the sample is valid (position, normal etc.) if this function returns true.
        //    bool VxgiGetGBufferEnableSSAO(VxgiGBufferSample gbufferSample);
        //
        //    // Returns the clip-space position for a given sample.
        //    // When normalized == true, VXGI expects that the .w component of position will be 1.0
        //    // This function is only used for SSAO.
        //    float4 VxgiGetGBufferClipPos(VxgiGBufferSample gbufferSample, bool normalized);
        //
        //    // Maps a given clip-space position to window coordinates which can be used to sample the G-buffer.
        //    // This function is only used for SSAO.
        //    float2 VxgiGBufferMapClipToWindow(uint viewIndex, float2 clipPos);
        //
        //    // Maps a given sample to a different view in the same or previous frame and returns the window coordinates.
        //    // Returns true if a matching surface exists in the other view, false otherwise.
        //    bool VxgiGetGBufferPositionInOtherView(VxgiGBufferSample gbufferSample, uint viewIndex, bool previous, out float2 prevWindowPos);
        //
        //    // Returns irradiance from an environment map for a surface at 'surfacePos' coming from a cone 
        //    // looking in 'coneDirection' with the cone factor 'coneFactor'. 
        //    float3 VxgiGetEnvironmentIrradiance(float3 surfacePos, float3 coneDirection, float coneFactor, bool isSpecular);
        //
        virtual Status::Enum compileViewTracingShaders(IBlob** ppBlob, const char* source, size_t sourceSize, const ShaderResources& userShaderCodeResources) = 0;

        // Tests whether the user defined shader binary is compatible with the current version of VXGI library.
        // If it isn't, you should recompile the shader and generate a new binary.
        virtual bool isValidUserDefinedShaderBinary(const void* binary, size_t binarySize) = 0;

        // Extracts the shader type descriptor from the binary.
        virtual IUserDefinedShaderSet::ShaderType getUserDefinedShaderBinaryType(const void* binary, size_t binarySize) = 0;

        // Gets the number of specific shader permutations stored in a binary.
        virtual uint32_t getUserDefinedShaderBinaryPermutationCount(const void* binary, size_t binarySize) = 0;

        // You must free these blobs
        virtual IBlob* getUserDefinedShaderBinaryReflectionData(const void* binary, size_t binarySize, uint32_t permutation) = 0;
        // This may do nothing if the API doesn't support this
        virtual IBlob* stripUserDefinedShaderBinary(const void* binary, size_t binarySize) = 0;
    };

    struct ShaderCompilerParameters
    {
        NVRHI::IErrorCallback* errorCallback;
        IAllocator* allocator;

        // you can use this to override the DLL VXGI will use to match your application
        const char* d3dCompilerDLLName;
        bool multicoreShaderCompilation;

        NVRHI::GraphicsAPI::Enum graphicsAPI;

		// Flags passed to the D3DCompile function
		uint32_t d3dCompileFlags;
		uint32_t d3dCompileFlags2;

        ShaderCompilerParameters()
            : errorCallback(nullptr)
            , allocator(nullptr)
            , d3dCompilerDLLName(nullptr)
            , multicoreShaderCompilation(true)
            , graphicsAPI(NVRHI::GraphicsAPI::D3D11)
			, d3dCompileFlags(0x9002) // D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_SKIP_VALIDATION | D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY
			, d3dCompileFlags2(0)
        { }
    };

    struct VoxelizationViewParameters
    {
        Box3f clipmapExtents;
        float4x4 viewMatrix;
        float4x4 projectionMatrix;
    };

    // The primary interface for interaction with VXGI
    // An instance of object implementing this interface can be created with VFX_VXGI_CreateGIObject
    // and destroyed with VFX_VXGI_DestroyGIObject
    class IGlobalIllumination
    {
        IGlobalIllumination& operator=(const IGlobalIllumination& other); //undefined
    protected:
        virtual ~IGlobalIllumination() {};
    public:

        // Returns the hash of the VXGI shaders that compose the IUserDefinedShaderSets. If this does not match the application must compile new voxelization shaders
        // or else loadUserDefinedShaderSet will reject them
        virtual uint64_t getInternalShaderHash() = 0;

        // Creates a view tracer with user-defined G-buffer access.
        virtual Status::Enum createCustomTracer(IViewTracer** ppTracer, IUserDefinedShaderSet* pShaderSet) = 0;

        // Creates a view tracer with default (planar projection, no AA, depth + normals) G-buffer access.
        virtual Status::Enum createBasicTracer(IBasicViewTracer** ppTracer, IShaderCompiler* pCompiler) = 0;

        // Releases all the previously created resources for a specific tracer
        virtual void destroyTracer(IViewTracer* pTracer) = 0;

        // Get the current renderer interface
        virtual NVRHI::IRendererInterface* getRendererInterface() = 0;

        // Gets the performance monitor
        virtual IPerformanceMonitor* getPerformanceMonitor() = 0;

        virtual void setPerformanceSectionsMode(PerformanceSectionsMode::Enum mode) = 0;

        // Sets or updates the voxelization parameters, such as clipmap resolution or storage formats.
        // All data in the existing clipmap will be lost.
        // If this function fails, the GI object will be in an uninitialized state, and all subsequent rendering calls 
        // will fail and return an INVALID_STATE error.
        virtual Status::Enum setVoxelizationParameters(const VoxelizationParameters& parameters) = 0;

        // Validates the voxelization parameters without affecting the active settings or re-allocating anything.
        // Returns Status::OK for acceptable sets of parameters, signals and returns an error for unacceptable ones.
        virtual Status::Enum validateVoxelizationParameters(const VoxelizationParameters& parameters) = 0;

        // This function performs all steps necessary to begin voxelization for a new frame. It should only be called once per frame.
        // The performOpacityVoxelization and performEmittanceVoxelization reference parameters are returned from this function,
        // indicating whether there is any region of voxel data where opacity or emittance is allowed to be updated on this frame, respectively.
        virtual Status::Enum prepareForVoxelization(
            const UpdateVoxelizationParameters& params,
            bool& performOpacityVoxelization,
            bool& performEmittanceVoxelization) = 0;

        // Marks the beginning of a group of independent draw calls used for voxelization, useful for performance.
        // If this function is not called, a barrier will be inserted by the D3D runtime between these draw calls because they use a UAV.
        // Inside, this methods calls NVRHI::IRendererInterface::setEnableUavBarriers(false, ...) with the right set of textures.
        virtual Status::Enum beginVoxelizationDrawCallGroup() = 0;

        // Marks the end of a group of independent draw calls used for voxelization.
        // Inside, this methods calls NVRHI::IRendererInterface::setEnableUavBarriers(true, ...)
        virtual Status::Enum endVoxelizationDrawCallGroup() = 0;

        // Returns the list of world-space regions that have to be revoxelized on this frame.
        // The list is based on the invalidatedRegions list passed to prepareForVoxelization(...), but is more complete and optimized.
        // Specifically, this list contains extra regions that are created from camera movements,
        // and all regions in the list are snapped to the allocation map page grid.
        // If the buffer passed as pRegions is too small, a BUFFER_TOO_SMALL error is returned, and numRegions contains a valid number. 
        // Call getInvalidatedRegions(0, 0, numRegions) to get the number of regions and allocate the buffer based on that.
        virtual Status::Enum getInvalidatedRegions(Box3f* pRegions, uint32_t maxRegions, uint32_t& numRegions) = 0;

        // Returns the minimum voxel size at a given world position using the last updated clipmap range and anchor. 
        // EXACT function returns the exact voxel size for each point, but it has discontinuities at level boundaries.
        // LINEAR_UNDERESTIMATE function returns a linear ramp that is less than or equal to the exact voxel size for each point.
        // LINEAR_OVERESTIMATE function returns a linear ramp that is greater than or equal to the exact voxel size for each point.
        // When zeroOutOfRange == false, values of this function outside of the clipmap equal to the values at the closest boundary.
        // When zeroOutOfRange == true, the function is zero outside of the clipmap.
        virtual float getMinVoxelSizeAtPoint(float3 position, VoxelSizeFunction::Enum function, bool zeroOutOfRange = false) = 0;

        // Returns the parameters that can to be used to setup the voxelization scene view, using the current configuration and anchor.
        // For preparing view parameters in advance, use the VFX_VXGI_ComputeVoxelizationViewParameters function. 
        virtual Status::Enum getVoxelizationViewParameters(VoxelizationViewParameters& outParams) = 0;

        // Computes the state necessary to perform voxelization for a given material.
        // Has to be called between prepareForVoxelization(...) and finalizeVoxelization(), otherwise an INVALID_STATE error will occur.
        // The state object passed into this function will be completely overwritten.
        // This function does not modify any internal state or make any rendering calls, so it's safe to call it from threads 
        // other than the primary rendering thread as long as it's called between prepareForVoxelization(...) and finalizeVoxelization().
        virtual Status::Enum getVoxelizationState(const MaterialInfo& materialInfo, bool writesEmittance, NVRHI::DrawCallState& state) = 0;

        // Finalizes all voxel representation updates and prepares for cone tracing.
        virtual Status::Enum finalizeVoxelization() = 0;

        // Renders a visualization of one of the voxel textures.
        virtual Status::Enum renderDebug(const DebugRenderParameters& params) = 0;

        // These functions bind the VXGI related resources to the user defined tracing shader.
        // You can add any other resources that your shader needs, but they have to be declared as ShaderResources 
        // in the corresponding createConeTracing{Pixel,Compute}Shader(...) call.
        // After the state is set, you can do any number of draw calls or dispatches with that state, 
        // which stays valid until the next call to updateGlobalIllumination.
        virtual Status::Enum setupUserDefinedConeTracingPixelShaderState(IUserDefinedShaderSet* shaderSet, NVRHI::DrawCallState& state) = 0;
        virtual Status::Enum setupUserDefinedConeTracingComputeShaderState(IUserDefinedShaderSet* shaderSet, NVRHI::DispatchState& state) = 0;

        // These functions return the clipmap parameters computed on the last successful call to prepareForOpacityVoxelization.
        virtual const Box3f& getLastUpdatedWorldRegion() = 0;
        virtual const Box3f& getLastUpdatedSceneExtents() = 0;
        virtual const float3& getLastUpdatedClipmapAnchor() = 0;

        // Loads the previously compiled user defined shader from a binary representation.
        // The binary can be obtained from IShaderCompiler::compileXxxShader(...) methods and stored somewhere to speed up application startup.
        virtual Status::Enum loadUserDefinedShaderSet(IUserDefinedShaderSet** ppShaderSet, const void* binary, size_t binarySize, bool reportNoErrorsOnInvalidBinaryFormat = false) = 0;

        // Destroys a user defined shader set previously created with one of the above functions.
        virtual void destroyUserDefinedShaderSet(IUserDefinedShaderSet* shader) = 0;

        // Renders something simple (currently a cube) into the voxel textures.
        // Call this function after prepareForOpacityVoxelization and/or prepareForEmittanceVoxelization to test 
        // the rendering backend implementation separately from your scene voxelization code.
        virtual Status::Enum voxelizeTestScene(float3 testObjectPosition, float testObjectSize, IShaderCompiler* compiler) = 0;

        // Sets some parameters used for VXGI development. Do not use.
        virtual void setInternalParameters(float4 params) = 0;
        virtual float4 getInternalParameters() = 0;

        // Returns the set of features that are supported by the GPU, regardless of VoxelizationParameters::enabledHardwareFeatures
        virtual HardwareFeatures::Enum getSupportedHardwareFeatures() = 0;
    };

    struct GIParameters
    {
        NVRHI::IRendererInterface* rendererInterface;
        NVRHI::IErrorCallback* errorCallback;
        IPerformanceMonitor* perfMonitor;
        IAllocator* allocator;

        GIParameters()
            : rendererInterface(nullptr)
            , perfMonitor(nullptr)
            , allocator(nullptr)
            , errorCallback(nullptr)
        { }
    };

    // All the members of this structure have the same meaning as the members of VoxelizationParameters and 
    // UpdateVoxelizationParameters structures with the same names.
    struct ComputeVoxelizationViewParametersInput
    {
        uint3 mapSize;
        uint32_t allocationMapLodBias;
        uint32_t stackLevels;
        float3 clipmapAnchor;
        float finestVoxelSize;

        ComputeVoxelizationViewParametersInput()
            : allocationMapLodBias(0)
            , stackLevels(0)
            , finestVoxelSize(0.f)
        { }
    };

#if VXGI_DYNAMIC_LOAD_LIBRARY
#define VXGI_EXPORT_FUNCTION(rtype, name, ...) typedef rtype (*PFN_##name)(__VA_ARGS__); extern PFN_##name name
#define VXGI_VERSION_ARGUMENT Version version
#elif CONFIGURATION_TYPE_DLL
#define VXGI_EXPORT_FUNCTION(rtype, name, ...) __declspec( dllexport ) rtype name(__VA_ARGS__)
#define VXGI_VERSION_ARGUMENT Version version
#else
#define VXGI_EXPORT_FUNCTION(rtype, name, ...) __declspec( dllimport ) rtype name(__VA_ARGS__)
#define VXGI_VERSION_ARGUMENT Version version = Version()
#endif

#if !VXGI_DYNAMIC_LOAD_LIBRARY
    extern "C"
    {
#endif
        // Creates a root interface object for VXGI. 
        // The interfaceVersion parameter is there to make sure that the VXGI DLL is built with the same version of the headers as the user code.
        // If the interface versions do not match, this method will return WRONG_INTERFACE_VERSION.
        VXGI_EXPORT_FUNCTION(Status::Enum, VFX_VXGI_CreateGIObject, const GIParameters& params, IGlobalIllumination** ppGI, VXGI_VERSION_ARGUMENT);

        // Destroys a previously created instance of the VXGI interface object.
        VXGI_EXPORT_FUNCTION(void, VFX_VXGI_DestroyGIObject, IGlobalIllumination* gi);

        // Creates a shader compiler object for VXGI.
        VXGI_EXPORT_FUNCTION(Status::Enum, VFX_VXGI_CreateShaderCompiler, const ShaderCompilerParameters& params, IShaderCompiler** ppCompiler, VXGI_VERSION_ARGUMENT);

        // Destroys a previously created instance of the VXGI interface object.
        VXGI_EXPORT_FUNCTION(void, VFX_VXGI_DestroyShaderCompiler, IShaderCompiler* compiler);

        // Compares the version of the header that the user code was built with and the version that VXGI was built with.
        // Returns OK if the versions match, and WRONG_INTERFACE_VERSION otherwise.
        VXGI_EXPORT_FUNCTION(Status::Enum, VFX_VXGI_VerifyInterfaceVersion, VXGI_VERSION_ARGUMENT);

        // This method returns a hash of VXGI shader fragments that are linked together with user-defined voxelization or cone tracing shaders.
        // User code may store the compiled shaders in binary format and re-use them as long as this shader hash stays the same.
        // The shader hash is also verified in IGlobalIllumination::loadUserDefinedShaderSet, so it is not necessary to store it separately.
        VXGI_EXPORT_FUNCTION(uint64_t, VFX_VXGI_GetInternalShaderHash, VXGI_VERSION_ARGUMENT);

        // This function calculates the parameters, such as clipmap extents and view/projection matrices, needed to set up a scene view for voxelization.
        // The function is stateless and can be called regardless of what the rest of VXGI is doing, for example in a game thread while the rendering thread does something else.
        VXGI_EXPORT_FUNCTION(Status::Enum, VFX_VXGI_ComputeVoxelizationViewParameters, const ComputeVoxelizationViewParametersInput& inputParams, VoxelizationViewParameters& outputParams, VXGI_VERSION_ARGUMENT);

        // Converts a status code to a string containing the name of that status code, e.g. "NULL_ARGUMENT".
        VXGI_EXPORT_FUNCTION(const char*, VFX_VXGI_StatusToString, Status::Enum status);

#if VXGI_DYNAMIC_LOAD_LIBRARY
        Status::Enum GetProcAddresses(HMODULE hDLL);
#else
    }
#endif
    
} // VXGI
GI_END_PACKING

#endif // GFSDK_VXGI_H_
