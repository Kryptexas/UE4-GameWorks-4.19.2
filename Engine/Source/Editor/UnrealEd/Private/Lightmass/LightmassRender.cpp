// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LightmassRender.cpp: lightmass rendering-related implementation.
=============================================================================*/

#include "UnrealEd.h"

#include "Programs/UnrealLightmass/Public/LightmassPublic.h"
#include "Lightmass.h"
#include "LightmassRender.h"
#include "MaterialCompiler.h"

DEFINE_LOG_CATEGORY_STATIC(LogLightmassRender, Error, All);

// FLightmassMaterialCompiler - A proxy compiler that overrides various compiler functions for potential problem expressions.
struct FLightmassMaterialCompiler : public FProxyMaterialCompiler
{
	FLightmassMaterialCompiler(FMaterialCompiler* InCompiler) :
		FProxyMaterialCompiler(InCompiler)
	{}

	virtual int32 ParticleMacroUV()
	{
		return Compiler->ParticleMacroUV();
	}

	virtual int32 ParticleRelativeTime()
	{
		return Compiler->Constant(0.0f);
	}

	virtual int32 ParticleMotionBlurFade()
	{
		return Compiler->Constant(1.0f);
	}

	virtual int32 ParticleDirection()
	{
		return Compiler->Constant3(0.0f, 0.0f, 0.0f);
	}

	virtual int32 ParticleSpeed()
	{
		return Compiler->Constant(0.0f);
	}
	
	virtual int32 ParticleSize()
	{
		return Compiler->Constant2(0.0f,0.0f);
	}

	virtual int32 WorldPosition(EWorldPositionIncludedOffsets WorldPositionIncludedOffsets)
	{
		//UE_LOG(LogLightmassRender, Log, TEXT("Lightmass material compiler has encountered WorldPosition... Forcing constant (0.0f,0.0f,0.0f)."));
		return Compiler->Constant3(0.0f,0.0f,0.0f);
	}

	virtual int32 ObjectWorldPosition() 
	{
		//UE_LOG(LogLightmassRender, Log, TEXT("Lightmass material compiler has encountered ObjectWorldPosition... Forcing constant (0.0f,0.0f,0.0f)."));
		return Compiler->Constant3(0.0f,0.0f,0.0f);
	}

	virtual int32 ObjectRadius() 
	{
		//UE_LOG(LogLightmassRender, Log, TEXT("Lightmass material compiler has encountered ObjectRadius... Forcing constant 500.0f."));
		return Compiler->Constant(500);
	}

	virtual int32 ObjectBounds() 
	{
		//UE_LOG(LogLightmassRender, Log, TEXT("Lightmass material compiler has encountered ObjectBounds... Forcing constant (0,0,0)."));
		return Compiler->Constant3(0,0,0);
	}

	virtual int32 DistanceCullFade() 
	{
		return Compiler->Constant(1.0f);
	}

	virtual int32 ActorWorldPosition() 
	{
		return Compiler->Constant3(0.0f,0.0f,0.0f);
	}

	virtual int32 CameraWorldPosition()
	{
		//UE_LOG(LogLightmassRender, Log, TEXT("Lightmass material compiler has encountered CameraWorldPosition... Forcing constant (0.0f,0.0f,0.0f)."));
		return Compiler->Constant3(0.0f,0.0f,0.0f);
	}

	virtual int32 CameraVector()
	{
		//UE_LOG(LogLightmassRender, Log, TEXT("Lightmass material compiler has encountered CameraVector... Forcing constant (0.0f,0.0f,1.0f)."));
		return Compiler->Constant3(0.0f,0.0f,1.0f);
	}

	virtual int32 LightVector()
	{
		//UE_LOG(LogLightmassRender, Log, TEXT("Lightmass material compiler has encountered LightVector... Forcing constant (1.0f,0.0f,0.0f)."));
		return Compiler->Constant3(1.0f,0.0f,0.0f);
	}

	virtual int32 ReflectionVector()
	{
		//UE_LOG(LogLightmassRender, Log, TEXT("Lightmass material compiler has encountered ReflectionVector... Forcing constant (0.0f,0.0f,-1.0f)."));
		return Compiler->Constant3(0.0f,0.0f,-1.0f);
	}

	virtual int32 ReflectionAboutCustomWorldNormal(int32 CustomWorldNormal, int32 bNormalizeCustomWorldNormal)
	{
		//UE_LOG(LogLightmassRender, Log, TEXT("Lightmass material compiler has encountered ReflectionAboutCustomNormalVector... Forcing constant (0.0f,0.0f,-1.0f)."));
		return Compiler->Constant3(0.0f,0.0f,-1.0f);
	}

	/**
	 *	Generate shader code for transforming a vector
	 */
	virtual int32 TransformVector(uint8 SourceCoordType,uint8 DestCoordType,int32 A)
	{
		//UE_LOG(LogLightmassRender, Log, TEXT("Lightmass material compiler has encountered TransformVector... Passing thru source vector untouched."));
		return A;
	}

	/**
	 *	Generate shader code for transforming a position
	 *
	 *	@param	CoordType - type of transform to apply. see EMaterialExpressionTransformPosition 
	 *	@param	A - index for input vector parameter's code
	 */
	virtual int32 TransformPosition(uint8 SourceCoordType,uint8 DestCoordType,int32 A)
	{
		//UE_LOG(LogLightmassRender, Log, TEXT("Lightmass material compiler has encountered TransformPosition... Passing thru source vector untouched."));
		return A;
	}

	virtual int32 VertexColor()
	{
		//UE_LOG(LogLightmassRender, Log, TEXT("Lightmass material compiler has encountered VertexColor... Forcing constant (1.0f,1.0f,1.0f,1.0f)."));
		return Compiler->Constant4(1.0f,1.0f,1.0f,1.0f);
	}

	virtual int32 RealTime()
	{
		//UE_LOG(LogLightmassRender, Log, TEXT("Lightmass material compiler has encountered RealTime... Forcing constant 0.0f."));
		return Compiler->Constant(0.0f);
	}

	virtual int32 GameTime()
	{
		//UE_LOG(LogLightmassRender, Log, TEXT("Lightmass material compiler has encountered GameTime... Forcing constant 0.0f."));
		return Compiler->Constant(0.0f);
	}

	virtual int32 LightmassReplace(int32 Realtime, int32 Lightmass) { return Lightmass; }

	virtual int32 GIReplace(int32 Direct, int32 StaticIndirect, int32 DynamicIndirect) { return StaticIndirect; }
};

/**
 * Class for rendering previews of material expressions in the material editor's linked object viewport.
 */
class FLightmassMaterialProxy : public FMaterial, public FMaterialRenderProxy
{
public:
	FLightmassMaterialProxy(): FMaterial()
	{
		SetQualityLevelProperties(EMaterialQualityLevel::High,false,GRHIFeatureLevel);
	}

	/** Initializes the material proxy and kicks off async shader compiling. */
	void BeginCompiling(UMaterialInterface* InMaterialInterface, EMaterialProperty InPropertyToCompile, EMaterialShaderMapUsage::Type InUsage)
	{
		MaterialInterface = InMaterialInterface;
		Material = MaterialInterface ? MaterialInterface->GetMaterial() : NULL;
		PropertyToCompile = InPropertyToCompile;
		Usage = InUsage;

		Material->AppendReferencedTextures(ReferencedTextures);

		// Have to properly handle compilation of static switches in MaterialInstance* cases...
		UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(InMaterialInterface);

		// Walk up the parent chain until we find the first MI with static parameters
		while (MaterialInstance 
			&& !MaterialInstance->bHasStaticPermutationResource
			&& MaterialInstance->Parent 
			&& MaterialInstance->Parent->IsA<UMaterialInstance>())
		{
			MaterialInstance = Cast<UMaterialInstance>(MaterialInstance->Parent);
		}

		// Special path for a MI with static parameters
		if (MaterialInstance && MaterialInstance->bHasStaticPermutationResource && MaterialInstance->Parent)
		{
			FMaterialResource* MIResource = MaterialInstance->GetMaterialResource(GRHIFeatureLevel);

			// Use the shader map Id from the static permutation
			// This allows us to create a deterministic yet unique Id for the shader map that will be compiled for this FLightmassMaterialProxy
			FMaterialShaderMapId ResourceId;
			//@todo - always use highest quality level for static lighting
			MaterialInstance->GetMaterialResourceId(GRHIShaderPlatform, EMaterialQualityLevel::Num, ResourceId);

			// Override with a special usage so we won't re-use the shader map used by the MI for rendering
			ResourceId.Usage = GetShaderMapUsage();

			CacheShaders(ResourceId, GRHIShaderPlatform, true);
		}
		else
		{
			FMaterialResource* MaterialResource = Material->GetMaterialResource(GRHIFeatureLevel);

			// Copy the material resource Id
			// The FLightmassMaterialProxy's GetShaderMapUsage will set it apart from the MI's resource when it comes to finding a shader map

			CacheShaders(GRHIShaderPlatform, true);
		}
	}

	virtual const TArray<UTexture*>& GetReferencedTextures() const OVERRIDE
	{
		return ReferencedTextures;
	}

	/**
	 * Should the shader for this material with the given platform, shader type and vertex 
	 * factory type combination be compiled
	 *
	 * @param Platform		The platform currently being compiled for
	 * @param ShaderType	Which shader is being compiled
	 * @param VertexFactory	Which vertex factory is being compiled (can be NULL)
	 *
	 * @return true if the shader should be compiled
	 */
	virtual bool ShouldCache(EShaderPlatform Platform, const FShaderType* ShaderType, const FVertexFactoryType* VertexFactoryType) const
	{
		if (VertexFactoryType == FindVertexFactoryType(FName(TEXT("FLocalVertexFactory"), FNAME_Find)))
		{
			// we only need the non-light-mapped, base pass, local vertex factory shaders for drawing an opaque Material Tile
			// @todo: Added a FindShaderType by fname or something"

			if(FCString::Stristr(ShaderType->GetName(), TEXT("BasePassVSFNoLightMapPolicy")))
			{
				return true;
			}
			else if(FCString::Stristr(ShaderType->GetName(), TEXT("BasePassPSFNoLightMapPolicy")))
			{
				return true;
			}
		}

		return false;
	}

	////////////////
	// FMaterialRenderProxy interface.
	virtual const FMaterial* GetMaterial(ERHIFeatureLevel::Type FeatureLevel) const
	{
		if(GetRenderingThreadShaderMap())
		{
			return this;
		}
		else
		{
			return UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(false)->GetMaterial(FeatureLevel);
		}
	}

	virtual bool GetVectorValue(const FName ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const
	{
		return MaterialInterface->GetRenderProxy(0)->GetVectorValue(ParameterName, OutValue, Context);
	}

	virtual bool GetScalarValue(const FName ParameterName, float* OutValue, const FMaterialRenderContext& Context) const
	{
		return MaterialInterface->GetRenderProxy(0)->GetScalarValue(ParameterName, OutValue, Context);
	}

	virtual bool GetTextureValue(const FName ParameterName,const UTexture** OutValue, const FMaterialRenderContext& Context) const
	{
		return MaterialInterface->GetRenderProxy(0)->GetTextureValue(ParameterName,OutValue,Context);
	}

	// Material properties.
	/** Entry point for compiling a specific material property.  This must call SetMaterialProperty. */
	virtual int32 CompileProperty(EMaterialProperty Property,EShaderFrequency InShaderFrequency,FMaterialCompiler* Compiler) const
	{
		static const auto UseDiffuseSpecularMaterialInputs = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.UseDiffuseSpecularMaterialInputs"));

		Compiler->SetMaterialProperty(Property, InShaderFrequency);
		// MAKE SURE THIS MATCHES THE CHART IN WillFillData
		// 						  RETURNED VALUES (F16 'textures')
		// 	BLEND MODE  | DIFFUSE     | SPECULAR     | EMISSIVE    | NORMAL    | TRANSMISSIVE              |
		// 	------------+-------------+--------------+-------------+-----------+---------------------------|
		// 	Opaque      | Diffuse     | Spec,SpecPwr | Emissive    | Normal    | 0 (EMPTY)                 |
		// 	Masked      | Diffuse     | Spec,SpecPwr | Emissive    | Normal    | Opacity Mask              |
		// 	Translucent | 0 (EMPTY)   | 0 (EMPTY)    | Emissive    | 0 (EMPTY) | (Emsv | Diffuse)*Opacity  |
		// 	Additive    | 0 (EMPTY)   | 0 (EMPTY)    | Emissive    | 0 (EMPTY) | (Emsv | Diffuse)*Opacity  |
		// 	Modulative  | 0 (EMPTY)   | 0 (EMPTY)    | Emissive    | 0 (EMPTY) | Emsv | Diffuse            |
		// 	------------+-------------+--------------+-------------+-----------+---------------------------|
		if( Property == MP_EmissiveColor )
		{
			UMaterial* ProxyMaterial = MaterialInterface->GetMaterial();
			EBlendMode BlendMode = MaterialInterface->GetBlendMode();
			EMaterialLightingModel LightingModel = MaterialInterface->GetLightingModel();
			check(ProxyMaterial);
			FLightmassMaterialCompiler ProxyCompiler(Compiler);
			switch (PropertyToCompile)
			{
			case MP_EmissiveColor:
				// Emissive is ALWAYS returned...
				return Compiler->ForceCast(MaterialInterface->CompileProperty(&ProxyCompiler,MP_EmissiveColor),MCT_Float3,true,true);
			case MP_DiffuseColor:
				// Only return for Opaque and Masked...
				if (BlendMode == BLEND_Opaque || BlendMode == BLEND_Masked)
				{
					EMaterialProperty DiffuseInput = UseDiffuseSpecularMaterialInputs->GetValueOnGameThread() == 1 ? MP_DiffuseColor : MP_BaseColor;
					return Compiler->ForceCast(MaterialInterface->CompileProperty(&ProxyCompiler, DiffuseInput),MCT_Float3,true,true);
				}
				break;
			case MP_SpecularColor: 
				// Only return for Opaque and Masked...
				if (BlendMode == BLEND_Opaque || BlendMode == BLEND_Masked)
				{
					return Compiler->AppendVector(
						Compiler->ForceCast(MaterialInterface->CompileProperty(&ProxyCompiler, MP_SpecularColor),MCT_Float3,true,true), 
						Compiler->ForceCast(MaterialInterface->CompileProperty(&ProxyCompiler,MP_Roughness),MCT_Float1));
				}
				break;
			case MP_Normal:
				// Only return for Opaque and Masked...
				if (BlendMode == BLEND_Opaque || BlendMode == BLEND_Masked)
				{
					return Compiler->ForceCast( MaterialInterface->CompileProperty(&ProxyCompiler, MP_Normal ), MCT_Float3, true, true );
				}
				break;
			
			case MP_Opacity:
				if (BlendMode == BLEND_Masked)
				{
					return MaterialInterface->CompileProperty(&ProxyCompiler, MP_OpacityMask);
				}
				else if (IsTranslucentBlendMode((EBlendMode)BlendMode) && ProxyMaterial->GetCastShadowAsMasked())
				{
					return MaterialInterface->CompileProperty(&ProxyCompiler, MP_Opacity);
				}
				else if (BlendMode == BLEND_Modulate)
				{
					if (LightingModel == MLM_Unlit)
					{
						return Compiler->ForceCast(MaterialInterface->CompileProperty(Compiler, MP_EmissiveColor),MCT_Float3,true,true);
					}
					else
					{
						return Compiler->ForceCast(MaterialInterface->CompileProperty(Compiler, MP_DiffuseColor),MCT_Float3,true,true);
					}
				}
				else if ((BlendMode == BLEND_Translucent) || (BlendMode == BLEND_Additive))
				{
					int32 ColoredOpacity = -1;
					if (LightingModel == MLM_Unlit)
					{
						ColoredOpacity = Compiler->ForceCast(MaterialInterface->CompileProperty(Compiler, MP_EmissiveColor),MCT_Float3,true,true);
					}
					else
					{
						ColoredOpacity = Compiler->ForceCast(MaterialInterface->CompileProperty(Compiler, MP_DiffuseColor),MCT_Float3,true,true);
					}
					return Compiler->Lerp(Compiler->Constant3(1.0f, 1.0f, 1.0f), ColoredOpacity, Compiler->ForceCast( MaterialInterface->CompileProperty(&ProxyCompiler,MP_Opacity), MCT_Float1));
				}
				break;
			default:
				return Compiler->Constant(1.0f);
			}
	
			return Compiler->Constant(0.0f);
		}
		else if( Property == MP_WorldPositionOffset)
		{
			//This property MUST return 0 as a default or during the process of rendering textures out for lightmass to use, pixels will be off by 1.
			return Compiler->Constant(0.0f);
		}
		else if (Property >= MP_CustomizedUVs0 && Property <= MP_CustomizedUVs7)
		{
			// Pass through customized UVs
			return MaterialInterface->CompileProperty(Compiler, Property);
		}
		else
		{
			return Compiler->Constant(1.0f);
		}
	}

	/** 
	 * Gets the shader map usage of the material, which will be included in the DDC key.
	 * This mechanism allows derived material classes to create different DDC keys with the same base material.
	 * For example lightmass exports diffuse and emissive, each of which requires a material resource with the same base material.
	 */
	virtual EMaterialShaderMapUsage::Type GetShaderMapUsage() const OVERRIDE { return Usage; }

	virtual FString GetMaterialUsageDescription() const OVERRIDE { return FString::Printf(TEXT("%s FLightmassMaterialRenderer"), MaterialInterface ? *MaterialInterface->GetName() : TEXT("NULL")); }
	
	virtual int32 GetMaterialDomain() const OVERRIDE
	{
		if (Material)
		{
			return Material->MaterialDomain;
		}
		return MD_Surface;
	}
	virtual bool IsTwoSided() const 
	{
		if (MaterialInterface)
		{
			return MaterialInterface->IsTwoSided();
		}
		return false;
	}
	virtual bool IsLightFunction() const
	{
		if (Material)
		{
			return (Material->MaterialDomain == MD_LightFunction);
		}
		return false;
	}
	virtual bool IsUsedWithDeferredDecal() const
	{
		return Material && Material->MaterialDomain == MD_DeferredDecal;
	}
	virtual bool IsSpecialEngineMaterial() const
	{
		if (Material)
		{
			return (Material->bUsedAsSpecialEngineMaterial == 1);
		}
		return false;
	}
	virtual bool IsWireframe() const
	{
		if (Material)
		{
			return (Material->Wireframe == 1);
		}
		return false;
	}
	virtual bool IsMasked() const									{ return false; }
	virtual enum EBlendMode GetBlendMode() const					{ return BLEND_Opaque; }
	virtual enum EMaterialLightingModel GetLightingModel() const	{ return MLM_Unlit; }
	virtual float GetOpacityMaskClipValue() const					{ return 0.5f; }
	virtual FString GetFriendlyName() const { return FString::Printf(TEXT("FLightmassMaterialRenderer %s"), MaterialInterface ? *MaterialInterface->GetName() : TEXT("NULL")); }

	/**
	 * Should shaders compiled for this material be saved to disk?
	 */
	virtual bool IsPersistent() const { return true; }

	virtual FGuid GetMaterialId() const OVERRIDE 
	{ 
		// Reuse the base material's Id
		// Normally this would cause a bug as the shader map would try to be shared by both, 
		// But FLightmassMaterialProxy::GetShaderMapUsage() allows this to work
		return Material->StateId;
	}

	const UMaterialInterface* GetMaterialInterface() const
	{
		return MaterialInterface;
	}

	friend FArchive& operator<< ( FArchive& Ar, FLightmassMaterialProxy& V )
	{
		return Ar << V.MaterialInterface;
	}

	bool IsMaterialInputConnected(UMaterial* InMaterial, EMaterialProperty MaterialInput)
	{
		bool bConnected = false;
		static const auto UseDiffuseSpecularMaterialInputs = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.UseDiffuseSpecularMaterialInputs"));

		switch (MaterialInput)
		{
		case MP_EmissiveColor:
			bConnected = InMaterial->EmissiveColor.Expression != NULL;
			break;
		case MP_DiffuseColor:
			bConnected = (UseDiffuseSpecularMaterialInputs->GetValueOnGameThread() == 1 ? InMaterial->DiffuseColor.Expression : InMaterial->BaseColor.Expression) != NULL;
			break;
		case MP_SpecularColor:
			bConnected = InMaterial->SpecularColor.Expression != NULL;
			break;
		case MP_Normal:
			bConnected = InMaterial->Normal.Expression != NULL;
			break;
		case MP_Opacity:
			bConnected = InMaterial->Opacity.Expression != NULL;
			break;
		case MP_OpacityMask:
			bConnected = InMaterial->OpacityMask.Expression != NULL;
			break;
		default:
			break;
		}

		// Note: only checking to see whether the entire material attributes connection exists.  
		// This means materials using the material attributes input will export more attributes than is necessary.
		bConnected = InMaterial->bUseMaterialAttributes ? InMaterial->MaterialAttributes.Expression != NULL : bConnected;
		return bConnected;
	}

	/**
	 *	Checks if the configuration of the material proxy will generate a uniform
	 *	value across the sampling... (Ie, nothing is hooked to the property)
	 *
	 *	@param	OutUniformValue		The value that will be returned.
	 *
	 *	@return	bool				true if a single value would be generated.
	 *								false if not.
	 */
	bool WillGenerateUniformData(FFloat16Color& OutUniformValue)
	{
		// Pre-fill the value...
		OutUniformValue.R = 0.0f;
		OutUniformValue.G = 0.0f;
		OutUniformValue.B = 0.0f;
		OutUniformValue.A = 0.0f;

		EBlendMode BlendMode = MaterialInterface->GetBlendMode();
		EMaterialLightingModel LightingModel = MaterialInterface->GetLightingModel();
		
		check(Material);
		bool bExpressionIsNULL = false;
		switch (PropertyToCompile)
		{
		case MP_EmissiveColor:
			// Emissive is ALWAYS returned...
			bExpressionIsNULL = !IsMaterialInputConnected(Material, PropertyToCompile);
			break;
		case MP_DiffuseColor:
			// Only return for Opaque and Masked...
			if (BlendMode == BLEND_Opaque || BlendMode == BLEND_Masked)
			{
				bExpressionIsNULL = !IsMaterialInputConnected(Material, PropertyToCompile);
			}
			break;
		case MP_SpecularColor: 
			// Only return for Opaque and Masked...
			if (BlendMode == BLEND_Opaque || BlendMode == BLEND_Masked)
			{
				bExpressionIsNULL = !IsMaterialInputConnected(Material, PropertyToCompile);
				OutUniformValue.A = 15.0f;
			}
			break;
		case MP_Normal:
			// Only return for Opaque and Masked...
			if (BlendMode == BLEND_Opaque || BlendMode == BLEND_Masked)
			{
				bExpressionIsNULL = !IsMaterialInputConnected(Material, PropertyToCompile);
				OutUniformValue.B = 1.0f;	// Default normal is (0,0,1)
			}
			break;
		case MP_Opacity:
			if (BlendMode == BLEND_Masked)
			{
				bExpressionIsNULL = !IsMaterialInputConnected(Material, MP_OpacityMask);
				OutUniformValue.R = 1.0f;
				OutUniformValue.G = 1.0f;
				OutUniformValue.B = 1.0f;
				OutUniformValue.A = 1.0f;
			}
			else
			if ((BlendMode == BLEND_Modulate) ||
				(BlendMode == BLEND_Translucent) || 
				(BlendMode == BLEND_Additive))
			{
				bool bColorInputIsNULL = false;
				if (LightingModel == MLM_Unlit)
				{
					bColorInputIsNULL = !IsMaterialInputConnected(Material, MP_EmissiveColor);
				}
				else
				{
					bColorInputIsNULL = !IsMaterialInputConnected(Material, MP_DiffuseColor);
				}
				if (BlendMode == BLEND_Translucent
					|| BlendMode == BLEND_Additive)
				{
					bExpressionIsNULL = bColorInputIsNULL && !IsMaterialInputConnected(Material, PropertyToCompile);
				}
				else
				{
					bExpressionIsNULL = bColorInputIsNULL;
				}
			}
			break;
		}

		return bExpressionIsNULL;
	}

	/**
	 *	Retrieves the desired render target format and size for the given property.
	 *	This will allow for overriding the format and/or size based on the material and property of interest.
	 *
	 *	@param	InMaterialProperty	The material property that is going to be captured in the render target.
	 *	@param	OutFormat			The format the render target should use.
	 *	@param	OutSizeX			The width to use for the render target.
	 *	@param	OutSizeY			The height to use for the render target.
	 *
	 *	@return	bool				true if data is good, false if not (do not create render target)
	 */
	bool GetRenderTargetFormatAndSize(EMaterialProperty InMaterialProperty, EPixelFormat& OutFormat, float SizeScale, int32& OutSizeX, int32& OutSizeY)
	{
		OutFormat = PF_FloatRGBA;

		int32 GlobalSize = 0;
		// For now, just look them up in the config file...
		if (InMaterialProperty == MP_DiffuseColor)
		{
			verify(GConfig->GetInt(TEXT("DevOptions.StaticLightingMaterial"), TEXT("DiffuseSampleSize"), GlobalSize, GLightmassIni));
		}
		else
		if (InMaterialProperty == MP_SpecularColor)
		{
			verify(GConfig->GetInt(TEXT("DevOptions.StaticLightingMaterial"), TEXT("SpecularSampleSize"), GlobalSize, GLightmassIni));
		}
		else
		if (InMaterialProperty == MP_EmissiveColor)
		{
			verify(GConfig->GetInt(TEXT("DevOptions.StaticLightingMaterial"), TEXT("EmissiveSampleSize"), GlobalSize, GLightmassIni));
		}
		else
		if (InMaterialProperty == MP_Normal)
		{
			verify(GConfig->GetInt(TEXT("DevOptions.StaticLightingMaterial"), TEXT("NormalSampleSize"), GlobalSize, GLightmassIni));
		}
		else
		if (InMaterialProperty == MP_Opacity)
		{
			verify(GConfig->GetInt(TEXT("DevOptions.StaticLightingMaterial"), TEXT("TransmissionSampleSize"), GlobalSize, GLightmassIni));
		}
		else
		{
			OutSizeX = 0;
			OutSizeY = 0;
			return false;
		}
		OutSizeX = OutSizeY = FMath::Trunc(GlobalSize * SizeScale);
		return true;
	}

	static bool WillFillData(EBlendMode InBlendMode, EMaterialProperty InMaterialProperty)
	{
		// MAKE SURE THIS MATCHES THE CHART IN CompileProperty
		// 						  RETURNED VALUES (F16 'textures')
		// 	BLEND MODE  | DIFFUSE     | SPECULAR     | EMISSIVE    | NORMAL    | TRANSMISSIVE              |
		// 	------------+-------------+--------------+-------------+-----------+---------------------------|
		// 	Opaque      | Diffuse     | Spec,SpecPwr | Emissive    | Normal    | 0 (EMPTY)                 |
		// 	Masked      | Diffuse     | Spec,SpecPwr | Emissive    | Normal    | Opacity Mask              |
		// 	Translucent | 0 (EMPTY)   | 0 (EMPTY)    | Emissive    | 0 (EMPTY) | (Emsv | Diffuse)*Opacity  |
		// 	Additive    | 0 (EMPTY)   | 0 (EMPTY)    | Emissive    | 0 (EMPTY) | (Emsv | Diffuse)*Opacity  |
		// 	Modulative  | 0 (EMPTY)   | 0 (EMPTY)    | Emissive    | 0 (EMPTY) | Emsv | Diffuse            |
		// 	------------+-------------+--------------+-------------+-----------+---------------------------|

		if (InMaterialProperty == MP_EmissiveColor)
		{
			return true;
		}

		switch (InBlendMode)
		{
		case BLEND_Opaque:
			{
				switch (InMaterialProperty)
				{
				case MP_DiffuseColor:	return true;
				case MP_SpecularColor:	return true;
				case MP_Normal:			return true;
				case MP_Opacity:		return false;
				}
			}
			break;
		case BLEND_Masked:
			{
				switch (InMaterialProperty)
				{
				case MP_DiffuseColor:	return true;
				case MP_SpecularColor:	return true;
				case MP_Normal:			return true;
				case MP_Opacity:		return true;
				}
			}
			break;
		case BLEND_Translucent:
		case BLEND_Additive:
			{
				switch (InMaterialProperty)
				{
				case MP_DiffuseColor:	return false;
				case MP_SpecularColor:	return false;
				case MP_Normal:			return false;
				case MP_Opacity:		return true;
				}
			}
			break;
		case BLEND_Modulate:
			{
				switch (InMaterialProperty)
				{
				case MP_DiffuseColor:	return false;
				case MP_SpecularColor:	return false;
				case MP_Normal:			return false;
				case MP_Opacity:		return true;
				}
			}
			break;
		}
		return false;
	}

private:
	/** The material interface for this proxy */
	UMaterialInterface* MaterialInterface;
	UMaterial* Material;
	TArray<UTexture*> ReferencedTextures;
	/** The property to compile for rendering the sample */
	EMaterialProperty PropertyToCompile;
	/** Stores which exported attribute this proxy is compiling for. */
	EMaterialShaderMapUsage::Type Usage;
};

FMaterialExportDataEntry::~FMaterialExportDataEntry()
{
	delete DiffuseMaterialProxy;
	delete EmissiveMaterialProxy;
	delete OpacityMaterialProxy;
	delete NormalMaterialProxy;
}

//
// FLightmassMaterialRenderer
//
FLightmassMaterialRenderer::~FLightmassMaterialRenderer()
{
	if (!GExitPurge && RenderTarget)
	{
		RenderTarget->RemoveFromRoot();
	}
	RenderTarget = NULL;
	delete Canvas;
	Canvas = NULL;
}

void FLightmassMaterialRenderer::BeginGenerateMaterialData(
	UMaterialInterface* InMaterial, 
	bool bInWantNormals, 
	const FString& ChannelName,
	TMap<UMaterialInterface*, FMaterialExportDataEntry>& MaterialExportData)
{
	UMaterial* BaseMaterial = InMaterial->GetMaterial();

	TEnumAsByte<EBlendMode> BlendMode = InMaterial->GetBlendMode();

	if (BaseMaterial)
	{
		check(MaterialExportData.Find(InMaterial) == NULL);

		FMaterialExportDataEntry& MaterialData = MaterialExportData.Add(InMaterial, FMaterialExportDataEntry(ChannelName));

		if (FLightmassMaterialProxy::WillFillData((EBlendMode)(BlendMode), MP_DiffuseColor))
		{
			MaterialData.DiffuseMaterialProxy = new FLightmassMaterialProxy();
			MaterialData.DiffuseMaterialProxy->BeginCompiling(InMaterial, MP_DiffuseColor, EMaterialShaderMapUsage::LightmassExportDiffuse);
		}

		if (FLightmassMaterialProxy::WillFillData((EBlendMode)(BlendMode), MP_EmissiveColor))
		{
			MaterialData.EmissiveMaterialProxy = new FLightmassMaterialProxy();
			MaterialData.EmissiveMaterialProxy->BeginCompiling(InMaterial, MP_EmissiveColor, EMaterialShaderMapUsage::LightmassExportEmissive);
		}

		if (FLightmassMaterialProxy::WillFillData((EBlendMode)(BlendMode), MP_Opacity))
		{
			MaterialData.OpacityMaterialProxy = new FLightmassMaterialProxy();
			MaterialData.OpacityMaterialProxy->BeginCompiling(InMaterial, MP_Opacity, EMaterialShaderMapUsage::LightmassExportOpacity);
		}

		if (bInWantNormals && FLightmassMaterialProxy::WillFillData((EBlendMode)(BlendMode), MP_Normal))
		{
			MaterialData.NormalMaterialProxy = new FLightmassMaterialProxy();
			MaterialData.NormalMaterialProxy->BeginCompiling(InMaterial, MP_Normal, EMaterialShaderMapUsage::LightmassExportNormal);
		}
	}
}

/**
 *	Generate the required material data for the given material.
 *
 *	@param	Material				The material of interest.
 *	@param	bInWantNormals			True if normals should be generated as well
 *	@param	MaterialEmissive		The emissive samples for the material.
 *	@param	MaterialDiffuse			The diffuse samples for the material.
 *	@param	MaterialTransmission	The transmission samples for the material.
 *
 *	@return	bool					true if successful, false if not.
 */
bool FLightmassMaterialRenderer::GenerateMaterialData(
	UMaterialInterface& InMaterial, 
	Lightmass::FMaterialData& OutMaterialData, 
	FMaterialExportDataEntry& MaterialExportEntry,
	TArray<FFloat16Color>& OutMaterialDiffuse, 
	TArray<FFloat16Color>& OutMaterialEmissive, 
	TArray<FFloat16Color>& OutMaterialTransmission, 
	TArray<FFloat16Color>& OutMaterialNormal)
{
	bool bResult = true;
	UMaterial* BaseMaterial = InMaterial.GetMaterial();
	check(BaseMaterial);

	EBlendMode BlendMode = InMaterial.GetBlendMode();
	EMaterialLightingModel LightingModel = InMaterial.GetLightingModel();
 	if ((LightingModel != MLM_DefaultLit) &&
		(LightingModel != MLM_Unlit) &&
		(LightingModel != MLM_Subsurface) &&
		(LightingModel != MLM_PreintegratedSkin))
	{
		UE_LOG(LogLightmassRender, Warning, TEXT("LIGHTMASS: Material has an unsupported lighting model: %d on %s"), 
			(int32)LightingModel,
			*(InMaterial.GetPathName()));
	}

	// Set the blend mode
	checkAtCompileTime(EBlendMode::BLEND_MAX == (EBlendMode)Lightmass::BLEND_MAX, DebugTypeSizesMustMatch);
	OutMaterialData.BlendMode = (Lightmass::EBlendMode)((int32)BlendMode);
	// Set the two-sided flag
	OutMaterialData.bTwoSided = (uint32)InMaterial.IsTwoSided();
	OutMaterialData.bCastShadowAsMasked = BaseMaterial->GetCastShadowAsMasked() && IsTranslucentBlendMode((EBlendMode)BlendMode);
	OutMaterialData.OpacityMaskClipValue = InMaterial.GetOpacityMaskClipValue();

	// Diffuse
	if (MaterialExportEntry.DiffuseMaterialProxy)
	{
		if (!GenerateMaterialPropertyData(InMaterial, *MaterialExportEntry.DiffuseMaterialProxy, MP_DiffuseColor, OutMaterialData.DiffuseSize, OutMaterialData.DiffuseSize, OutMaterialDiffuse))
		{
			UE_LOG(LogLightmassRender, Warning, TEXT("Failed to generate diffuse material samples for %s: %s"),
				*(InMaterial.GetLightingGuid().ToString()), *(InMaterial.GetPathName()));
			bResult = false;
			OutMaterialData.DiffuseSize = 0;
		}
	}

	// Emissive
	if (MaterialExportEntry.EmissiveMaterialProxy)
	{
		if (!GenerateMaterialPropertyData(InMaterial, *MaterialExportEntry.EmissiveMaterialProxy, MP_EmissiveColor, OutMaterialData.EmissiveSize, OutMaterialData.EmissiveSize, OutMaterialEmissive))
		{
			UE_LOG(LogLightmassRender, Warning, TEXT("Failed to generate emissive material samples for %s: %s"),
				*(InMaterial.GetLightingGuid().ToString()), *(InMaterial.GetPathName()));
			bResult = false;
			OutMaterialData.EmissiveSize = 0;
		}
	}

	// Transmission
	if (MaterialExportEntry.OpacityMaterialProxy)
	{
		if (!GenerateMaterialPropertyData(InMaterial, *MaterialExportEntry.OpacityMaterialProxy, MP_Opacity, OutMaterialData.TransmissionSize, OutMaterialData.TransmissionSize, OutMaterialTransmission))
		{
			UE_LOG(LogLightmassRender, Warning, TEXT("Failed to generate transmissive material samples for %s: %s"),
				*(InMaterial.GetLightingGuid().ToString()), *(InMaterial.GetPathName()));
			bResult = false;
			OutMaterialData.TransmissionSize = 0;
		}
	}

	// Normal
	if (MaterialExportEntry.NormalMaterialProxy)
	{
		if (!GenerateMaterialPropertyData(InMaterial, *MaterialExportEntry.NormalMaterialProxy, MP_Normal, OutMaterialData.NormalSize, OutMaterialData.NormalSize, OutMaterialNormal))
		{
			UE_LOG(LogLightmassRender, Warning, TEXT("Failed to generate normal material samples for %s: %s"),
				*(InMaterial.GetLightingGuid().ToString()), *(InMaterial.GetPathName()));
			bResult = false;
			OutMaterialData.NormalSize = 0;
		}
	}

	return bResult;
}

/**
 *	Generate the material data for the given material and it's given property.
 *
 *	@param	InMaterial				The material of interest.
 *	@param	InMaterialProperty		The property to generate the samples for
 *	@param	InOutSizeX				The desired X size of the sample to capture (in), the resulting size (out)
 *	@param	InOutSizeY				The desired Y size of the sample to capture (in), the resulting size (out)
 *	@param	OutMaterialSamples		The samples for the material.
 *
 *	@return	bool					true if successful, false if not.
 */
bool FLightmassMaterialRenderer::GenerateMaterialPropertyData(
	UMaterialInterface& InMaterial, 
	FLightmassMaterialProxy& MaterialProxy,
	EMaterialProperty InMaterialProperty, 
	int32& InOutSizeX, 
	int32& InOutSizeY, 
	TArray<FFloat16Color>& OutMaterialSamples)
{
	bool bResult = true;

	FFloat16Color UniformValue;
	// Verify that async compiling has completed for this material
	// If the ShaderMap is NULL that's because it failed to compile, which is ok as the default material will be used for exporting
	check(!MaterialProxy.GetGameThreadShaderMap() || MaterialProxy.GetGameThreadShaderMap()->IsValidForRendering());

	if (MaterialProxy.WillGenerateUniformData(UniformValue) == false)
	{
		//@todo UE4. The format may be determined by the material property...
		// For example, if Diffuse doesn't need to be F16 it can create a standard RGBA8 target.
		EPixelFormat Format = PF_FloatRGBA;
		if (MaterialProxy.GetRenderTargetFormatAndSize(InMaterialProperty, Format, InMaterial.GetExportResolutionScale(), InOutSizeX, InOutSizeY))
		{
			if (CreateRenderTarget(Format, InOutSizeX, InOutSizeY) == false)
			{
				UE_LOG(LogLightmassRender, Warning, TEXT("Failed to create %4dx%4d render target!"), InOutSizeX, InOutSizeY);
				bResult = false;
			}
			else
			{
				// At this point, we can't just return false at failure since we have some clean-up to do...
				Canvas->SetRenderTarget(RenderTarget->GameThread_GetRenderTargetResource());
				RHIBeginScene();
				// Clear the render target to black
				// This is necessary because the below DrawTile doesn't write to the first column and first row
				//@todo - figure out and fix DrawTile issues when rendering a full render target quad
				Canvas->Clear(FLinearColor(0,0,0,0));
				FCanvasTileItem TileItem( FVector2D( 0.0f, 0.0f ), &MaterialProxy, FVector2D( InOutSizeX, InOutSizeY ) );
				TileItem.bFreezeTime = true;
				Canvas->DrawItem( TileItem );
				Canvas->Flush();
				FlushRenderingCommands();
				Canvas->SetRenderTarget(NULL);
				FlushRenderingCommands();
				RHIEndScene();

				// Read in the data
				//@todo UE4. Check the format! RenderTarget->Format
				// If we are going to allow non-F16 formats, then the storage will have to be aware of it!
				if (RenderTarget->GameThread_GetRenderTargetResource()->ReadFloat16Pixels(OutMaterialSamples) == false)
				{
					UE_LOG(LogLightmassRender, Warning, TEXT("Failed to read Float16Pixels for 0x%08x property of %s: %s"), 
						(uint32)InMaterialProperty, *(InMaterial.GetLightingGuid().ToString()), *(InMaterial.GetPathName()));
					bResult = false;
				}
				else if (GLightmassDebugOptions.bDebugMaterials == true)
				{
					TArray<FColor> OutputBuffer;
					OutputBuffer.Empty(OutMaterialSamples.Num());
					bool bSRGB = InMaterialProperty != MP_Normal;
					for (int32 i = 0; i < OutMaterialSamples.Num(); ++i)
					{
						FLinearColor LinearColor(OutMaterialSamples[i]);
						OutputBuffer.Add(LinearColor.ToFColor(bSRGB));
					}

					// Create screenshot folder if not already present.
					// Save the contents of the array to a bitmap file.
					FString TempPath = FPaths::ScreenShotDir();
					TempPath += TEXT("/Materials");
					IFileManager::Get().MakeDirectory(*TempPath, true);
					FString TempName = InMaterial.GetPathName();
					TempName = TempName.Replace(TEXT("."), TEXT("_"));
					TempName = TempName.Replace(TEXT(":"), TEXT("_"));
					FString OutputName = TempPath / TempName;
					OutputName += TEXT("_");
					switch (InMaterialProperty)
					{
					case MP_DiffuseColor:	OutputName += TEXT("Diffuse");			break;
					case MP_EmissiveColor:	OutputName += TEXT("Emissive");			break;
					case MP_SpecularColor:	OutputName += TEXT("Specular");			break;
					case MP_Normal:			OutputName += TEXT("Normal");			break;
					case MP_Opacity:		OutputName += TEXT("Transmissive");		break;
					}
					OutputName += TEXT(".BMP");
					FFileHelper::CreateBitmap(*OutputName,InOutSizeX,InOutSizeY,OutputBuffer.GetTypedData());
				}
			}
		}
		else
		{
			UE_LOG(LogLightmassRender, Warning, TEXT("Failed to get render target format and size for 0x%08x property of %s: %s"), 
				(uint32)InMaterialProperty, *(InMaterial.GetLightingGuid().ToString()), *(InMaterial.GetPathName()));
			bResult = false;
		}
	}
	else
	{
		// Single value... fill it in.
		InOutSizeX = 1;
		InOutSizeY = 1; 
		OutMaterialSamples.Empty(1);
		OutMaterialSamples.AddZeroed(1);
		OutMaterialSamples[0] = UniformValue;
	}

	return bResult;
}

/**
 *	Create the required render target.
 *
 *	@param	InFormat	The format of the render target
 *	@param	InSizeX		The X resolution of the render target
 *	@param	InSizeY		The Y resolution of the render target
 *
 *	@return	bool		true if it was successful, false if not
 */
bool FLightmassMaterialRenderer::CreateRenderTarget(EPixelFormat InFormat, int32 InSizeX, int32 InSizeY)
{
	if (RenderTarget && 
		((RenderTarget->OverrideFormat != InFormat) || (RenderTarget->SizeX != InSizeX) || (RenderTarget->SizeY != InSizeY))
		)
	{
		RenderTarget->RemoveFromRoot();
		RenderTarget = NULL;
		delete Canvas;
		Canvas = NULL;
	}

	if (RenderTarget == NULL)
	{
		RenderTarget = new UTextureRenderTarget2D(FPostConstructInitializeProperties());
		check(RenderTarget);
		RenderTarget->AddToRoot();
		RenderTarget->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		RenderTarget->InitCustomFormat(InSizeX, InSizeY, InFormat, false);

		Canvas = new FCanvas(RenderTarget->GameThread_GetRenderTargetResource(), NULL, 0, 0, 0);
		check(Canvas);
	}

	return (RenderTarget != NULL);
}
