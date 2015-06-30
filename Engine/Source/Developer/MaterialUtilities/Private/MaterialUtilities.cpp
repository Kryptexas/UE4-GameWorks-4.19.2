// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#include "MaterialUtilitiesPrivatePCH.h"

#include "Runtime/Engine/Classes/Engine/World.h"
#include "Runtime/Engine/Classes/Materials/MaterialInterface.h"
#include "Runtime/Engine/Classes/Materials/MaterialExpressionConstant.h"
#include "Runtime/Engine/Classes/Engine/TextureRenderTarget2D.h"
#include "Runtime/Engine/Classes/Engine/Texture2D.h"
#include "Runtime/Engine/Classes/Engine/TextureCube.h"
#include "Runtime/Engine/Public/TileRendering.h"
#include "Runtime/Engine/Public/EngineModule.h"
#include "Runtime/Engine/Public/ImageUtils.h"
#include "Runtime/Engine/Public/CanvasTypes.h"
#include "Runtime/Engine/Public/MaterialCompiler.h"
#include "Runtime/Engine/Classes/Engine/TextureLODSettings.h"
#include "Runtime/Engine/Classes/DeviceProfiles/DeviceProfileManager.h"
#include "RendererInterface.h"
#include "LandscapeProxy.h"
#include "LandscapeComponent.h"


IMPLEMENT_MODULE(FDefaultModuleImpl, MaterialUtilities);


DEFINE_LOG_CATEGORY_STATIC(LogMaterialUtilities, Log, All);

/*------------------------------------------------------------------------------
	Helper classes for render material to texture
------------------------------------------------------------------------------*/
struct FExportMaterialCompiler : public FProxyMaterialCompiler
{
	FExportMaterialCompiler(FMaterialCompiler* InCompiler) :
		FProxyMaterialCompiler(InCompiler)
	{}

	// gets value stored by SetMaterialProperty()
	virtual EShaderFrequency GetCurrentShaderFrequency() const override
	{
		// not used by Lightmass
		return SF_Pixel;
	}

	virtual int32 ParticleRelativeTime() override
	{
		return Compiler->Constant(0.0f);
	}

	virtual int32 ParticleMotionBlurFade() override
	{
		return Compiler->Constant(1.0f);
	}

	virtual int32 ParticleDirection() override
	{
		return Compiler->Constant3(0.0f, 0.0f, 0.0f);
	}

	virtual int32 ParticleSpeed() override
	{
		return Compiler->Constant(0.0f);
	}
	
	virtual int32 ParticleSize() override
	{
		return Compiler->Constant2(0.0f,0.0f);
	}

	virtual int32 ObjectRadius() override
	{
		return Compiler->Constant(500);
	}

	virtual int32 ObjectBounds() override
	{
		return Compiler->Constant3(0,0,0);
	}

	virtual int32 CameraVector() override
	{
		return Compiler->Constant3(0.0f,0.0f,1.0f);
	}

	virtual int32 ReflectionAboutCustomWorldNormal(int32 CustomWorldNormal, int32 bNormalizeCustomWorldNormal) override
	{
		return Compiler->Constant3(0.0f,0.0f,-1.0f);
	}

	virtual int32 VertexColor() override
	{
		return Compiler->Constant4(1.0f,1.0f,1.0f,1.0f);
	}
};



class FExportMaterialProxy : public FMaterial, public FMaterialRenderProxy
{
public:
	FExportMaterialProxy()
		: FMaterial()
	{
		SetQualityLevelProperties(EMaterialQualityLevel::High, false, GMaxRHIFeatureLevel);
	}

	FExportMaterialProxy(UMaterialInterface* InMaterialInterface, EMaterialProperty InPropertyToCompile)
		: FMaterial()
		, MaterialInterface(InMaterialInterface)
		, PropertyToCompile(InPropertyToCompile)
	{
		SetQualityLevelProperties(EMaterialQualityLevel::High, false, GMaxRHIFeatureLevel);
		Material = InMaterialInterface->GetMaterial();
		Material->AppendReferencedTextures(ReferencedTextures);
		FPlatformMisc::CreateGuid(Id);

		FMaterialResource* Resource = InMaterialInterface->GetMaterialResource(GMaxRHIFeatureLevel);

		FMaterialShaderMapId ResourceId;
		Resource->GetShaderMapId(GMaxRHIShaderPlatform, ResourceId);

		{
			TArray<FShaderType*> ShaderTypes;
			TArray<FVertexFactoryType*> VFTypes;
			GetDependentShaderAndVFTypes(GMaxRHIShaderPlatform, ShaderTypes, VFTypes);

			// Overwrite the shader map Id's dependencies with ones that came from the FMaterial actually being compiled (this)
			// This is necessary as we change FMaterial attributes like GetShadingModel(), which factor into the ShouldCache functions that determine dependent shader types
			ResourceId.SetShaderDependencies(ShaderTypes, VFTypes);
		}

		// Override with a special usage so we won't re-use the shader map used by the material for rendering
		switch (InPropertyToCompile)
		{
		case MP_BaseColor: ResourceId.Usage = EMaterialShaderMapUsage::MaterialExportBaseColor; break;
		case MP_Specular: ResourceId.Usage = EMaterialShaderMapUsage::MaterialExportSpecular; break;
		case MP_Normal: ResourceId.Usage = EMaterialShaderMapUsage::MaterialExportNormal; break;
		case MP_Metallic: ResourceId.Usage = EMaterialShaderMapUsage::MaterialExportMetallic; break;
		case MP_Roughness: ResourceId.Usage = EMaterialShaderMapUsage::MaterialExportRoughness; break;
		};
		
		CacheShaders(ResourceId, GMaxRHIShaderPlatform, true);
	}

	/** This override is required otherwise the shaders aren't ready for use when the surface is rendered resulting in a blank image */
	virtual bool RequiresSynchronousCompilation() const override { return true; };

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
	virtual bool ShouldCache(EShaderPlatform Platform, const FShaderType* ShaderType, const FVertexFactoryType* VertexFactoryType) const override
	{
		// Always cache - decreases performance but avoids missing shaders during exports.
		return true;
	}

	virtual const TArray<UTexture*>& GetReferencedTextures() const override
	{
		return ReferencedTextures;
	}

	////////////////
	// FMaterialRenderProxy interface.
	virtual const FMaterial* GetMaterial(ERHIFeatureLevel::Type FeatureLevel) const override
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

	virtual bool GetVectorValue(const FName ParameterName, FLinearColor* OutValue, const FMaterialRenderContext& Context) const override
	{
		return MaterialInterface->GetRenderProxy(0)->GetVectorValue(ParameterName, OutValue, Context);
	}

	virtual bool GetScalarValue(const FName ParameterName, float* OutValue, const FMaterialRenderContext& Context) const override
	{
		return MaterialInterface->GetRenderProxy(0)->GetScalarValue(ParameterName, OutValue, Context);
	}

	virtual bool GetTextureValue(const FName ParameterName,const UTexture** OutValue, const FMaterialRenderContext& Context) const override
	{
		return MaterialInterface->GetRenderProxy(0)->GetTextureValue(ParameterName,OutValue,Context);
	}

	// Material properties.
	/** Entry point for compiling a specific material property.  This must call SetMaterialProperty. */
	virtual int32 CompilePropertyAndSetMaterialProperty(EMaterialProperty Property, FMaterialCompiler* Compiler, EShaderFrequency OverrideShaderFrequency, bool bUsePreviousFrameTime) const override
	{
		// needs to be called in this function!!
		Compiler->SetMaterialProperty(Property, OverrideShaderFrequency, bUsePreviousFrameTime);

		int32 Ret = CompilePropertyAndSetMaterialPropertyWithoutCast(Property, Compiler);

		return Compiler->ForceCast(Ret, GetMaterialPropertyType(Property));
	}

	/** helper for CompilePropertyAndSetMaterialProperty() */
	int32 CompilePropertyAndSetMaterialPropertyWithoutCast(EMaterialProperty Property, FMaterialCompiler* Compiler) const
	{
		if (Property == MP_EmissiveColor)
		{
			UMaterial* ProxyMaterial = MaterialInterface->GetMaterial();
			check(ProxyMaterial);
			EBlendMode BlendMode = MaterialInterface->GetBlendMode();
			EMaterialShadingModel ShadingModel = MaterialInterface->GetShadingModel();
			FExportMaterialCompiler ProxyCompiler(Compiler);
									
			switch (PropertyToCompile)
			{
			case MP_EmissiveColor:
				// Emissive is ALWAYS returned...
				return Compiler->ForceCast(MaterialInterface->CompileProperty(&ProxyCompiler,MP_EmissiveColor),MCT_Float3,true,true);
			case MP_BaseColor:
				// Only return for Opaque and Masked...
				if (BlendMode == BLEND_Opaque || BlendMode == BLEND_Masked)
				{
					return Compiler->ForceCast(MaterialInterface->CompileProperty(&ProxyCompiler, MP_BaseColor),MCT_Float3,true,true);
				}
				break;
			case MP_Specular: 
			case MP_Roughness:
			case MP_Metallic:
				// Only return for Opaque and Masked...
				if (BlendMode == BLEND_Opaque || BlendMode == BLEND_Masked)
				{
					return Compiler->ForceCast(MaterialInterface->CompileProperty(&ProxyCompiler, PropertyToCompile),MCT_Float,true,true);
				}
				break;
			case MP_Normal:
				// Only return for Opaque and Masked...
				if (BlendMode == BLEND_Opaque || BlendMode == BLEND_Masked)
				{
					return Compiler->ForceCast( 
						Compiler->Add( 
							Compiler->Mul(MaterialInterface->CompileProperty(&ProxyCompiler, MP_Normal), Compiler->Constant(0.5f)), // [-1,1] * 0.5
							Compiler->Constant(0.5f)), // [-0.5,0.5] + 0.5
						MCT_Float3, true, true );
				}
				break;
			default:
				return Compiler->Constant(1.0f);
			}
	
			return Compiler->Constant(0.0f);
		}
		else if (Property == MP_WorldPositionOffset)
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

	virtual FString GetMaterialUsageDescription() const override
	{
		return FString::Printf(TEXT("FExportMaterialRenderer %s"), MaterialInterface ? *MaterialInterface->GetName() : TEXT("NULL"));
	}
	virtual int32 GetMaterialDomain() const override
	{
		if (Material)
		{
			return Material->MaterialDomain;
		}
		return MD_Surface;
	}
	virtual bool IsTwoSided() const  override
	{ 
		if (MaterialInterface)
		{
			return MaterialInterface->IsTwoSided();
		}
		return false;
	}
	virtual bool IsDitheredLODTransition() const  override
	{ 
		if (MaterialInterface)
		{
			return MaterialInterface->IsDitheredLODTransition();
		}
		return false;
	}
	virtual bool IsLightFunction() const override
	{
		if (Material)
		{
			return (Material->MaterialDomain == MD_LightFunction);
		}
		return false;
	}
	virtual bool IsUsedWithDeferredDecal() const override
	{
		return	Material &&
				Material->MaterialDomain == MD_DeferredDecal;
	}
	virtual bool IsSpecialEngineMaterial() const override
	{
		if (Material)
		{
			return (Material->bUsedAsSpecialEngineMaterial == 1);
		}
		return false;
	}
	virtual bool IsWireframe() const override
	{
		if (Material)
		{
			return (Material->Wireframe == 1);
		}
		return false;
	}
	virtual bool IsMasked() const override								{ return false; }
	virtual enum EBlendMode GetBlendMode() const override				{ return BLEND_Opaque; }
	virtual enum EMaterialShadingModel GetShadingModel() const override	{ return MSM_Unlit; }
	virtual float GetOpacityMaskClipValue() const override				{ return 0.5f; }
	virtual FString GetFriendlyName() const override { return FString::Printf(TEXT("FExportMaterialRenderer %s"), MaterialInterface ? *MaterialInterface->GetName() : TEXT("NULL")); }
	/**
	* Should shaders compiled for this material be saved to disk?
	*/
	virtual bool IsPersistent() const override { return false; }
	virtual FGuid GetMaterialId() const override { return Id; }

	const UMaterialInterface* GetMaterialInterface() const
	{
		return MaterialInterface;
	}

	friend FArchive& operator<< ( FArchive& Ar, FExportMaterialProxy& V )
	{
		return Ar << V.MaterialInterface;
	}

	/**
	* Iterate through all textures used by the material and return the maximum texture resolution used
	* (ideally this could be made dependent of the material property)
	*
	* @param MaterialInterface The material to scan for texture size
	*
	* @return Size (width and height)
	*/
	FIntPoint FindMaxTextureSize(UMaterialInterface* InMaterialInterface, FIntPoint MinimumSize = FIntPoint(1, 1)) const
	{
		// static lod settings so that we only initialize them once
		UTextureLODSettings* GameTextureLODSettings = UDeviceProfileManager::Get().GetActiveProfile()->GetTextureLODSettings();

		TArray<UTexture*> MaterialTextures;

		InMaterialInterface->GetUsedTextures(MaterialTextures, EMaterialQualityLevel::Num, false, GMaxRHIFeatureLevel, false);

		// find the largest texture in the list (applying it's LOD bias)
		FIntPoint MaxSize = MinimumSize;
		for (int32 TexIndex = 0; TexIndex < MaterialTextures.Num(); TexIndex++)
		{
			UTexture* Texture = MaterialTextures[TexIndex];

			if (Texture == NULL)
			{
				continue;
			}

			// get the max size of the texture
			FIntPoint LocalSize(0, 0);
			if (Texture->IsA(UTexture2D::StaticClass()))
			{
				UTexture2D* Tex2D = (UTexture2D*)Texture;
				LocalSize = FIntPoint(Tex2D->GetSizeX(), Tex2D->GetSizeY());
			}
			else if (Texture->IsA(UTextureCube::StaticClass()))
			{
				UTextureCube* TexCube = (UTextureCube*)Texture;
				LocalSize = FIntPoint(TexCube->GetSizeX(), TexCube->GetSizeY());
			}

			int32 LocalBias = GameTextureLODSettings->CalculateLODBias(Texture);

			// bias the texture size based on LOD group
			FIntPoint BiasedLocalSize(LocalSize.X >> LocalBias, LocalSize.Y >> LocalBias);

			MaxSize.X = FMath::Max(BiasedLocalSize.X, MaxSize.X);
			MaxSize.Y = FMath::Max(BiasedLocalSize.Y, MaxSize.Y);
		}

		return MaxSize;
	}

	static bool WillFillData(EBlendMode InBlendMode, EMaterialProperty InMaterialProperty)
	{
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
				case MP_BaseColor:		return true;
				case MP_Specular:		return true;
				case MP_Normal:			return true;
				case MP_Metallic:		return true;
				case MP_Roughness:		return true;
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
	FGuid Id;
};

static void RenderMaterialTile(UWorld* InWorld, FMaterialRenderProxy* InMaterialProxy, UTextureRenderTarget2D* InRenderTarget, bool bFrontView)
{
	float CurrentRealTime = 0.f;
	float CurrentWorldTime = 0.f;
	float DeltaWorldTime = 0.f;
	
	if (InWorld)
	{
		CurrentRealTime = InWorld->GetRealTimeSeconds();
		CurrentWorldTime = InWorld->GetTimeSeconds();
		DeltaWorldTime = InWorld->GetDeltaSeconds();
	}
	else
	{
		CurrentRealTime = FApp::GetCurrentTime() - GStartTime;
		CurrentWorldTime = FApp::GetDeltaTime();
		DeltaWorldTime = FApp::GetCurrentTime() - GStartTime;
	}

	const FRenderTarget* RenderTargetResource = InRenderTarget->GameThread_GetRenderTargetResource();
	FSceneViewFamily* ViewFamily = new FSceneViewFamily(FSceneViewFamily::ConstructionValues(
		RenderTargetResource,
		InWorld ? InWorld->Scene : nullptr,
		FEngineShowFlags(ESFIM_Game))
		.SetWorldTimes(CurrentWorldTime, DeltaWorldTime, CurrentRealTime)
		.SetGammaCorrection(RenderTargetResource->GetDisplayGamma()));

	FIntPoint ViewSize = RenderTargetResource->GetSizeXY();
	FIntRect ViewRect(FIntPoint(0, 0), ViewSize);
	// By default render tile in top view
	FMatrix ViewMatrix = FMatrix(
				FPlane(1, 0, 0, 0),
				FPlane(0, -1, 0, 0),
				FPlane(0, 0, -1, 0),
				FPlane(0, 0, 0, 1));
	FQuat Rotation = FQuat::Identity;

	if (bFrontView)
	{
		ViewMatrix = FMatrix(
				FPlane(1, 0, 0, 0),
				FPlane(0, 0, -1, 0),
				FPlane(0, 1, 0, 0),
				FPlane(0, 0, 0, 1));
		// Tile mesh was created on XY plane
		Rotation = FRotator(0.0f, 0.0f, 90.0f).Quaternion();
	}
						
	// make a temporary view
	FSceneViewInitOptions ViewInitOptions;
	ViewInitOptions.ViewFamily = ViewFamily;
	ViewInitOptions.SetViewRectangle(ViewRect);
	ViewInitOptions.ViewOrigin = FVector(ViewSize.X/2.f, ViewSize.Y/2.f, 0.0f);
	ViewInitOptions.ViewRotationMatrix = ViewMatrix;
	ViewInitOptions.ProjectionMatrix = FReversedZOrthoMatrix(ViewSize.X/2.f, ViewSize.Y/2.f, 0.5f/HALF_WORLD_MAX, HALF_WORLD_MAX);
	ViewInitOptions.BackgroundColor = FLinearColor::Black;
	ViewInitOptions.OverlayColor = FLinearColor::White;
				
	FSceneView* View = new FSceneView(ViewInitOptions);
						
	ENQUEUE_UNIQUE_RENDER_COMMAND_FIVEPARAMETER(
		RenderMaterialTileCommand,
		FSceneView*, InView, View,
		FMaterialRenderProxy*, InProxy, InMaterialProxy,
		const FRenderTarget*, InRenderTarget, RenderTargetResource,
		FIntPoint, InSize, ViewSize,
		FQuat, InRotation, Rotation,
		{
			::SetRenderTarget(RHICmdList, InRenderTarget->GetRenderTargetTexture(), FTextureRHIRef());
			FIntRect ViewportRect = FIntRect(FIntPoint::ZeroValue, InSize);
			RHICmdList.SetViewport(ViewportRect.Min.X, ViewportRect.Min.Y, 0.0f, ViewportRect.Max.X, ViewportRect.Max.Y, 1.0f);
		
			FTileRenderer::DrawRotatedTile(RHICmdList, *InView, InProxy, false, InRotation, 0.f, 0.f, InSize.X, InSize.Y,  0.f, 0.f, 1.f, 1.f);
		});

	FlushRenderingCommands();

	delete View;
}

static void RenderSceneToTexture(
		FSceneInterface* Scene,
		const FName& VisualizationMode, 
		const FVector& ViewOrigin,
		const FMatrix& ViewRotationMatrix, 
		const FMatrix& ProjectionMatrix,  
		const TSet<FPrimitiveComponentId>& HiddenPrimitives, 
		FIntPoint TargetSize,
		float TargetGamma,
		TArray<FColor>& OutSamples)
{
	auto RenderTargetTexture = NewObject<UTextureRenderTarget2D>();
	check(RenderTargetTexture);
	RenderTargetTexture->AddToRoot();
	RenderTargetTexture->ClearColor = FLinearColor::Transparent;
	RenderTargetTexture->TargetGamma = TargetGamma;
	RenderTargetTexture->InitCustomFormat(TargetSize.X, TargetSize.Y, PF_FloatRGBA, false);
	FTextureRenderTargetResource* RenderTargetResource = RenderTargetTexture->GameThread_GetRenderTargetResource();

	FSceneViewFamilyContext ViewFamily(
		FSceneViewFamily::ConstructionValues(RenderTargetResource, Scene, FEngineShowFlags(ESFIM_Game))
			.SetWorldTimes(FApp::GetCurrentTime() - GStartTime, FApp::GetDeltaTime(), FApp::GetCurrentTime() - GStartTime)
		);

	// To enable visualization mode
	ViewFamily.EngineShowFlags.SetPostProcessing(true);
	ViewFamily.EngineShowFlags.SetVisualizeBuffer(true);
	ViewFamily.EngineShowFlags.SetTonemapper(false);

	FSceneViewInitOptions ViewInitOptions;
	ViewInitOptions.SetViewRectangle(FIntRect(0, 0, TargetSize.X, TargetSize.Y));
	ViewInitOptions.ViewFamily = &ViewFamily;
	ViewInitOptions.HiddenPrimitives = HiddenPrimitives;
	ViewInitOptions.ViewOrigin = ViewOrigin;
	ViewInitOptions.ViewRotationMatrix = ViewRotationMatrix;
	ViewInitOptions.ProjectionMatrix = ProjectionMatrix;
		
	FSceneView* NewView = new FSceneView(ViewInitOptions);
	NewView->CurrentBufferVisualizationMode = VisualizationMode;
	ViewFamily.Views.Add(NewView);
					
	FCanvas Canvas(RenderTargetResource, NULL, FApp::GetCurrentTime() - GStartTime, FApp::GetDeltaTime(), FApp::GetCurrentTime() - GStartTime, Scene->GetFeatureLevel());
	Canvas.Clear(FLinearColor::Transparent);
	GetRendererModule().BeginRenderingViewFamily(&Canvas, &ViewFamily);

	// Copy the contents of the remote texture to system memory
	OutSamples.SetNumUninitialized(TargetSize.X*TargetSize.Y);
	FReadSurfaceDataFlags ReadSurfaceDataFlags;
	ReadSurfaceDataFlags.SetLinearToGamma(false);
	RenderTargetResource->ReadPixelsPtr(OutSamples.GetData(), ReadSurfaceDataFlags, FIntRect(0, 0, TargetSize.X, TargetSize.Y));
	FlushRenderingCommands();
					
	RenderTargetTexture->RemoveFromRoot();
	RenderTargetTexture = nullptr;
}

bool FMaterialUtilities::SupportsExport(EBlendMode InBlendMode, EMaterialProperty InMaterialProperty)
{
	return FExportMaterialProxy::WillFillData(InBlendMode, InMaterialProperty);
}

bool FMaterialUtilities::ExportMaterialProperty(UWorld* InWorld, UMaterialInterface* InMaterial, EMaterialProperty InMaterialProperty, UTextureRenderTarget2D* InRenderTarget, TArray<FColor>& OutBMP)
{
	TScopedPointer<FExportMaterialProxy> MaterialProxy(new FExportMaterialProxy(InMaterial, InMaterialProperty));
	if (MaterialProxy == NULL)
	{
		return false;
	}

	bool bNormalmap = (InMaterialProperty == MP_Normal);

	RenderMaterialTile(InWorld, MaterialProxy, InRenderTarget, !bNormalmap);
					
	FReadSurfaceDataFlags ReadPixelFlags(bNormalmap ? RCM_SNorm : RCM_UNorm);
	ReadPixelFlags.SetLinearToGamma(false);

	FTextureRenderTargetResource* RTResource = InRenderTarget->GameThread_GetRenderTargetResource();
	return RTResource->ReadPixels(OutBMP, ReadPixelFlags);
}

bool FMaterialUtilities::ExportMaterialProperty(UWorld* InWorld, UMaterialInterface* InMaterial, EMaterialProperty InMaterialProperty, FIntPoint& OutSize, TArray<FColor>& OutBMP)
{
	TScopedPointer<FExportMaterialProxy> MaterialProxy(new FExportMaterialProxy(InMaterial, InMaterialProperty));
	if (MaterialProxy == NULL)
	{
		return false;
	}

	bool bNormalmap = (InMaterialProperty == MP_Normal);
	EPixelFormat Format = PF_FloatRGB;
	OutSize = MaterialProxy->FindMaxTextureSize(InMaterial);
	OutBMP.Reserve(OutSize.X*OutSize.Y);
	OutBMP.SetNumUninitialized(OutSize.X*OutSize.Y);

	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
	check(RenderTarget);
	RenderTarget->AddToRoot();
	RenderTarget->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
	RenderTarget->InitCustomFormat(OutSize.X, OutSize.Y, Format, bNormalmap);
		
	RenderMaterialTile(InWorld, MaterialProxy, RenderTarget, !bNormalmap);
					
	FReadSurfaceDataFlags ReadPixelFlags(bNormalmap ? RCM_SNorm : RCM_UNorm);
	ReadPixelFlags.SetLinearToGamma(false);

	FTextureRenderTargetResource* RTResource = RenderTarget->GameThread_GetRenderTargetResource();
	bool bResult = RTResource->ReadPixels(OutBMP, ReadPixelFlags);
	RenderTarget->RemoveFromRoot();

	return bResult;
}

bool FMaterialUtilities::ExportMaterial(UWorld* InWorld, UMaterialInterface* InMaterial, FFlattenMaterial& OutFlattenMaterial)
{
	// Render diffuse property
	if (OutFlattenMaterial.DiffuseSize.X > 0 && 
		OutFlattenMaterial.DiffuseSize.Y > 0)
	{
		// Create temporary render target
		auto RenderTargetDiffuse = NewObject<UTextureRenderTarget2D>();
		check(RenderTargetDiffuse);
		RenderTargetDiffuse->AddToRoot();
		RenderTargetDiffuse->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		RenderTargetDiffuse->InitCustomFormat(
			OutFlattenMaterial.DiffuseSize.X, 
			OutFlattenMaterial.DiffuseSize.Y, PF_B8G8R8A8, false);
			
		//
		OutFlattenMaterial.DiffuseSamples.Empty(OutFlattenMaterial.DiffuseSize.X * OutFlattenMaterial.DiffuseSize.Y);
		bool bResult = ExportMaterialProperty(InWorld, InMaterial, MP_BaseColor, RenderTargetDiffuse, OutFlattenMaterial.DiffuseSamples);

		// Uniform value
		if (OutFlattenMaterial.DiffuseSamples.Num() == 1)
		{
			OutFlattenMaterial.DiffuseSize = FIntPoint(1,1);
		}
			
		RenderTargetDiffuse->RemoveFromRoot();
		RenderTargetDiffuse = nullptr;

		if (!bResult)
		{
			return false;
		}
	}

	// Render normal property
	if (OutFlattenMaterial.NormalSize.X > 0 && 
		OutFlattenMaterial.NormalSize.Y > 0)
	{
		// Create temporary render target
		auto RenderTargetNormal = NewObject<UTextureRenderTarget2D>();
		check(RenderTargetNormal);
		RenderTargetNormal->AddToRoot();
		RenderTargetNormal->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		RenderTargetNormal->InitCustomFormat(
			OutFlattenMaterial.NormalSize.X,
			OutFlattenMaterial.NormalSize.Y, PF_FloatRGB, true);

		//
		OutFlattenMaterial.NormalSamples.Empty(OutFlattenMaterial.NormalSize.X * OutFlattenMaterial.NormalSize.Y);
		bool bResult = ExportMaterialProperty(InWorld, InMaterial, MP_Normal, RenderTargetNormal, OutFlattenMaterial.NormalSamples);

		// Uniform value
		if (OutFlattenMaterial.NormalSamples.Num() == 1)
		{
			OutFlattenMaterial.NormalSize = FIntPoint(1,1);
		}
		
		RenderTargetNormal->RemoveFromRoot();
		RenderTargetNormal = nullptr;

		if (!bResult)
		{
			return false;
		}
	}

	// Render metallic property
	if (OutFlattenMaterial.MetallicSize.X > 0 && 
		OutFlattenMaterial.MetallicSize.Y > 0)
	{
		// Create temporary render target
		auto RenderTargetMetallic = NewObject<UTextureRenderTarget2D>();
		check(RenderTargetMetallic);
		RenderTargetMetallic->AddToRoot();
		RenderTargetMetallic->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		RenderTargetMetallic->InitCustomFormat(
			OutFlattenMaterial.MetallicSize.X,
			OutFlattenMaterial.MetallicSize.Y, PF_B8G8R8A8, true);

		//
		OutFlattenMaterial.MetallicSamples.Empty(OutFlattenMaterial.MetallicSize.X * OutFlattenMaterial.MetallicSize.Y);
		bool bResult = ExportMaterialProperty(InWorld, InMaterial, MP_Metallic, RenderTargetMetallic, OutFlattenMaterial.MetallicSamples);

		// Uniform value
		if (OutFlattenMaterial.MetallicSamples.Num() == 1)
		{
			OutFlattenMaterial.MetallicSize = FIntPoint(1,1);
		}
		
		RenderTargetMetallic->RemoveFromRoot();
		RenderTargetMetallic = nullptr;

		if (!bResult)
		{
			return false;
		}
	}

	// Render roughness property
	if (OutFlattenMaterial.RoughnessSize.X > 0 && 
		OutFlattenMaterial.RoughnessSize.Y > 0)
	{
		// Create temporary render target
		auto RenderTargetRoughness = NewObject<UTextureRenderTarget2D>();
		check(RenderTargetRoughness);
		RenderTargetRoughness->AddToRoot();
		RenderTargetRoughness->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		RenderTargetRoughness->InitCustomFormat(
			OutFlattenMaterial.RoughnessSize.X,
			OutFlattenMaterial.RoughnessSize.Y, PF_B8G8R8A8, true);

		//
		OutFlattenMaterial.RoughnessSamples.Empty(OutFlattenMaterial.RoughnessSize.X * OutFlattenMaterial.RoughnessSize.Y);
		bool bResult = ExportMaterialProperty(InWorld, InMaterial, MP_Roughness, RenderTargetRoughness, OutFlattenMaterial.RoughnessSamples);

		// Uniform value
		if (OutFlattenMaterial.RoughnessSamples.Num() == 1)
		{
			OutFlattenMaterial.RoughnessSize = FIntPoint(1,1);
		}
		
		RenderTargetRoughness->RemoveFromRoot();
		RenderTargetRoughness = nullptr;

		if (!bResult)
		{
			return false;
		}
	}

	// Render specular property
	if (OutFlattenMaterial.SpecularSize.X > 0 && 
		OutFlattenMaterial.SpecularSize.Y > 0)
	{
		// Create temporary render target
		auto RenderTargetSpecular = NewObject<UTextureRenderTarget2D>();
		check(RenderTargetSpecular);
		RenderTargetSpecular->AddToRoot();
		RenderTargetSpecular->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
		RenderTargetSpecular->InitCustomFormat(
			OutFlattenMaterial.SpecularSize.X,
			OutFlattenMaterial.SpecularSize.Y, PF_B8G8R8A8, true);

		//
		OutFlattenMaterial.SpecularSamples.Empty(OutFlattenMaterial.SpecularSize.X * OutFlattenMaterial.SpecularSize.Y);
		bool bResult = ExportMaterialProperty(InWorld, InMaterial, MP_Specular, RenderTargetSpecular, OutFlattenMaterial.SpecularSamples);

		// Uniform value
		if (OutFlattenMaterial.SpecularSamples.Num() == 1)
		{
			OutFlattenMaterial.SpecularSize = FIntPoint(1,1);
		}
		
		RenderTargetSpecular->RemoveFromRoot();
		RenderTargetSpecular = nullptr;

		if (!bResult)
		{
			return false;
		}
	}
		
	OutFlattenMaterial.MaterialId = InMaterial->GetLightingGuid();
	return true;
}

bool FMaterialUtilities::ExportLandscapeMaterial(ALandscapeProxy* InLandscape, const TSet<FPrimitiveComponentId>& HiddenPrimitives, FFlattenMaterial& OutFlattenMaterial)
{
	check(InLandscape);

	FIntRect LandscapeRect = InLandscape->GetBoundingRect();
	FVector MidPoint = FVector(LandscapeRect.Min, 0.f) + FVector(LandscapeRect.Size(), 0.f)*0.5f;
		
	FVector LandscapeCenter = InLandscape->GetTransform().TransformPosition(MidPoint);
	FVector LandscapeExtent = FVector(LandscapeRect.Size(), 0.f)*InLandscape->GetActorScale()*0.5f; 

	FVector ViewOrigin = LandscapeCenter;
	FMatrix ViewRotationMatrix = FInverseRotationMatrix(InLandscape->GetActorRotation());
	ViewRotationMatrix*= FMatrix(FPlane(1,	0,	0,	0),
							FPlane(0,	-1,	0,	0),
							FPlane(0,	0,	-1,	0),
							FPlane(0,	0,	0,	1));
				
	const float ZOffset = WORLD_MAX;
	FMatrix ProjectionMatrix =  FReversedZOrthoMatrix(
		LandscapeExtent.X,
		LandscapeExtent.Y,
		0.5f / ZOffset,
		ZOffset);

	FSceneInterface* Scene = InLandscape->GetWorld()->Scene;
						
	// Render diffuse texture using BufferVisualizationMode=BaseColor
	if (OutFlattenMaterial.DiffuseSize.X > 0 && 
		OutFlattenMaterial.DiffuseSize.Y > 0)
	{
		static const FName BaseColorName("BaseColor");
		const float BaseColorGamma = 2.2f; // BaseColor to gamma space
		RenderSceneToTexture(Scene, BaseColorName, ViewOrigin, ViewRotationMatrix, ProjectionMatrix, HiddenPrimitives, 
			OutFlattenMaterial.DiffuseSize, BaseColorGamma, OutFlattenMaterial.DiffuseSamples);
	}

	// Render normal map using BufferVisualizationMode=WorldNormal
	// Final material should use world space instead of tangent space for normals
	if (OutFlattenMaterial.NormalSize.X > 0 && 
		OutFlattenMaterial.NormalSize.Y > 0)
	{
		static const FName WorldNormalName("WorldNormal");
		const float NormalColorGamma = 1.0f; // Dump normal texture in linear space
		RenderSceneToTexture(Scene, WorldNormalName, ViewOrigin, ViewRotationMatrix, ProjectionMatrix, HiddenPrimitives, 
			OutFlattenMaterial.NormalSize, NormalColorGamma, OutFlattenMaterial.NormalSamples);
	}

	// Render metallic map using BufferVisualizationMode=Metallic
	if (OutFlattenMaterial.MetallicSize.X > 0 && 
		OutFlattenMaterial.MetallicSize.Y > 0)
	{
		static const FName MetallicName("Metallic");
		const float MetallicColorGamma = 1.0f; // Dump metallic texture in linear space
		RenderSceneToTexture(Scene, MetallicName, ViewOrigin, ViewRotationMatrix, ProjectionMatrix, HiddenPrimitives, 
			OutFlattenMaterial.MetallicSize, MetallicColorGamma, OutFlattenMaterial.MetallicSamples);
	}

	// Render roughness map using BufferVisualizationMode=Roughness
	if (OutFlattenMaterial.RoughnessSize.X > 0 && 
		OutFlattenMaterial.RoughnessSize.Y > 0)
	{
		static const FName RoughnessName("Roughness");
		const float RoughnessColorGamma = 2.2f; // Roughness material powers color by 2.2, transform it back to linear
		RenderSceneToTexture(Scene, RoughnessName, ViewOrigin, ViewRotationMatrix, ProjectionMatrix, HiddenPrimitives, 
			OutFlattenMaterial.RoughnessSize, RoughnessColorGamma, OutFlattenMaterial.RoughnessSamples);
	}

	// Render specular map using BufferVisualizationMode=Specular
	if (OutFlattenMaterial.SpecularSize.X > 0 && 
		OutFlattenMaterial.SpecularSize.Y > 0)
	{
		static const FName SpecularName("Specular");
		const float SpecularColorGamma = 1.0f; // Dump specular texture in linear space
		RenderSceneToTexture(Scene, SpecularName, ViewOrigin, ViewRotationMatrix, ProjectionMatrix, HiddenPrimitives, 
			OutFlattenMaterial.SpecularSize, SpecularColorGamma, OutFlattenMaterial.SpecularSamples);
	}
				
	OutFlattenMaterial.MaterialId = InLandscape->GetLandscapeGuid();
	return true;
}

UMaterial* FMaterialUtilities::CreateMaterial(const FFlattenMaterial& InFlattenMaterial, UPackage* InOuter, const FString& BaseName, EObjectFlags Flags, TArray<UObject*>& OutGeneratedAssets)
{
	// Base name for a new assets
	// In case outer is null BaseName has to be long package name
	if (InOuter == nullptr && FPackageName::IsShortPackageName(BaseName))
	{
		UE_LOG(LogMaterialUtilities, Warning, TEXT("Invalid long package name: '%s'."), *BaseName);
		return nullptr;
	}

	const FString AssetBaseName = FPackageName::GetShortName(BaseName);
	const FString AssetBasePath = InOuter ? TEXT("") : FPackageName::GetLongPackagePath(BaseName) + TEXT("/");
				
	// Create material
	const FString MaterialAssetName = TEXT("M_") + AssetBaseName;
	UPackage* MaterialOuter = InOuter;
	if (MaterialOuter == NULL)
	{
		MaterialOuter = CreatePackage(NULL, *(AssetBasePath + MaterialAssetName));
		MaterialOuter->FullyLoad();
		MaterialOuter->Modify();
	}

	UMaterial* Material = NewObject<UMaterial>(MaterialOuter, FName(*MaterialAssetName), Flags);
	Material->TwoSided = false;
	Material->SetShadingModel(MSM_DefaultLit);
	OutGeneratedAssets.Add(Material);

	int32 MaterialNodeY = -150;
	int32 MaterialNodeStepY = 180;

	// BaseColor
	if (InFlattenMaterial.DiffuseSamples.Num() > 1)
	{
		const FString AssetName = TEXT("T_") + AssetBaseName + TEXT("_D");
		const FString AssetLongName = AssetBasePath + AssetName;
		const bool bSRGB = true;
		UTexture2D* Texture = CreateTexture(InOuter, AssetLongName, InFlattenMaterial.DiffuseSize, InFlattenMaterial.DiffuseSamples, TC_Default, TEXTUREGROUP_World, Flags, bSRGB);
		OutGeneratedAssets.Add(Texture);
			
		auto BasecolorExpression = NewObject<UMaterialExpressionTextureSample>(Material);
		BasecolorExpression->Texture = Texture;
		BasecolorExpression->SamplerType = EMaterialSamplerType::SAMPLERTYPE_Color;
		BasecolorExpression->MaterialExpressionEditorX = -400;
		BasecolorExpression->MaterialExpressionEditorY = MaterialNodeY;
		Material->Expressions.Add(BasecolorExpression);
		Material->BaseColor.Expression = BasecolorExpression;

		MaterialNodeY+= MaterialNodeStepY;
	}

	// Metallic
	if (InFlattenMaterial.MetallicSamples.Num() > 1)
	{
		const FString AssetName = TEXT("T_") + AssetBaseName + TEXT("_M");
		const bool bSRGB = false;
		UTexture2D* Texture = CreateTexture(InOuter, AssetBasePath + AssetName, InFlattenMaterial.MetallicSize, InFlattenMaterial.MetallicSamples, TC_Grayscale, TEXTUREGROUP_World, Flags, bSRGB);
		OutGeneratedAssets.Add(Texture);
			
		auto MetallicExpression = NewObject<UMaterialExpressionTextureSample>(Material);
		MetallicExpression->Texture = Texture;
		MetallicExpression->SamplerType = EMaterialSamplerType::SAMPLERTYPE_LinearGrayscale;
		MetallicExpression->MaterialExpressionEditorX = -400;
		MetallicExpression->MaterialExpressionEditorY = MaterialNodeY;
		Material->Expressions.Add(MetallicExpression);
		Material->Metallic.Expression = MetallicExpression;

		MaterialNodeY+= MaterialNodeStepY;
	}
	else if (InFlattenMaterial.MetallicSamples.Num() == 1)
	{
		// Set Metallic to constant
		float Metallic = *(float*)(&InFlattenMaterial.MetallicSamples[0].DWColor());
		auto MetallicExpression = NewObject<UMaterialExpressionConstant>(Material);
		MetallicExpression->R = Metallic;
		MetallicExpression->MaterialExpressionEditorX = -400;
		MetallicExpression->MaterialExpressionEditorY = MaterialNodeY;
		Material->Expressions.Add(MetallicExpression);
		Material->Metallic.Expression = MetallicExpression;

		MaterialNodeY+= MaterialNodeStepY;
	}

	// Specular
	if (InFlattenMaterial.SpecularSamples.Num() > 1)
	{
		const FString AssetName = TEXT("T_") + AssetBaseName + TEXT("_S");
		const bool bSRGB = false;
		UTexture2D* Texture = CreateTexture(InOuter, AssetBasePath + AssetName, InFlattenMaterial.SpecularSize, InFlattenMaterial.SpecularSamples, TC_Grayscale, TEXTUREGROUP_World, Flags, bSRGB);
		OutGeneratedAssets.Add(Texture);
			
		auto SpecularExpression = NewObject<UMaterialExpressionTextureSample>(Material);
		SpecularExpression->Texture = Texture;
		SpecularExpression->SamplerType = EMaterialSamplerType::SAMPLERTYPE_LinearGrayscale;
		SpecularExpression->MaterialExpressionEditorX = -400;
		SpecularExpression->MaterialExpressionEditorY = MaterialNodeY;
		Material->Expressions.Add(SpecularExpression);
		Material->Specular.Expression = SpecularExpression;

		MaterialNodeY+= MaterialNodeStepY;
	}
	else if (InFlattenMaterial.SpecularSamples.Num() == 1)
	{
		// Set Specular to constant
		float Specular = *(float*)(&InFlattenMaterial.SpecularSamples[0].DWColor());
		auto SpecularExpression = NewObject<UMaterialExpressionConstant>(Material);
		SpecularExpression->R = Specular;
		SpecularExpression->MaterialExpressionEditorX = -400;
		SpecularExpression->MaterialExpressionEditorY = MaterialNodeY;
		Material->Expressions.Add(SpecularExpression);
		Material->Specular.Expression = SpecularExpression;

		MaterialNodeY+= MaterialNodeStepY;
	}
	
	// Roughness
	if (InFlattenMaterial.RoughnessSamples.Num() > 1)
	{
		const FString AssetName = TEXT("T_") + AssetBaseName + TEXT("_R");
		const bool bSRGB = false;
		UTexture2D* Texture = CreateTexture(InOuter, AssetBasePath + AssetName, InFlattenMaterial.RoughnessSize, InFlattenMaterial.RoughnessSamples, TC_Grayscale, TEXTUREGROUP_World, Flags, bSRGB);
		OutGeneratedAssets.Add(Texture);
			
		auto RoughnessExpression = NewObject<UMaterialExpressionTextureSample>(Material);
		RoughnessExpression->Texture = Texture;
		RoughnessExpression->SamplerType = EMaterialSamplerType::SAMPLERTYPE_LinearGrayscale;
		RoughnessExpression->MaterialExpressionEditorX = -400;
		RoughnessExpression->MaterialExpressionEditorY = MaterialNodeY;
		Material->Expressions.Add(RoughnessExpression);
		Material->Roughness.Expression = RoughnessExpression;

		MaterialNodeY+= MaterialNodeStepY;
	}
	else if (InFlattenMaterial.RoughnessSamples.Num() == 1)
	{
		// Set Roughness to constant
		float Roughness = *(float*)(&InFlattenMaterial.RoughnessSamples[0].DWColor());
		auto RoughnessExpression = NewObject<UMaterialExpressionConstant>(Material);
		RoughnessExpression->R = Roughness;
		RoughnessExpression->MaterialExpressionEditorX = -400;
		RoughnessExpression->MaterialExpressionEditorY = MaterialNodeY;
		Material->Expressions.Add(RoughnessExpression);
		Material->Roughness.Expression = RoughnessExpression;

		MaterialNodeY+= MaterialNodeStepY;
	}

	// Normal
	if (InFlattenMaterial.NormalSamples.Num() > 1)
	{
		const FString AssetName = TEXT("T_") + AssetBaseName + TEXT("_N");
		const bool bSRGB = false;
		UTexture2D* Texture = CreateTexture(InOuter, AssetBasePath + AssetName, InFlattenMaterial.NormalSize, InFlattenMaterial.NormalSamples, TC_Normalmap, TEXTUREGROUP_WorldNormalMap, Flags, bSRGB);
		OutGeneratedAssets.Add(Texture);
			
		auto NormalExpression = NewObject<UMaterialExpressionTextureSample>(Material);
		NormalExpression->Texture = Texture;
		NormalExpression->SamplerType = EMaterialSamplerType::SAMPLERTYPE_Normal;
		NormalExpression->MaterialExpressionEditorX = -400;
		NormalExpression->MaterialExpressionEditorY = MaterialNodeY;
		Material->Expressions.Add(NormalExpression);
		Material->Normal.Expression = NormalExpression;

		MaterialNodeY+= MaterialNodeStepY;
	}
							
	Material->PostEditChange();
	return Material;
}

UTexture2D* FMaterialUtilities::CreateTexture(UPackage* Outer, const FString& AssetLongName, FIntPoint Size, const TArray<FColor>& Samples, TextureCompressionSettings CompressionSettings, TextureGroup LODGroup, EObjectFlags Flags, bool bSRGB, const FGuid& SourceGuidHash)
{
	FCreateTexture2DParameters TexParams;
	TexParams.bUseAlpha = false;
	TexParams.CompressionSettings = CompressionSettings;
	TexParams.bDeferCompression = true;
	TexParams.bSRGB = bSRGB;
	TexParams.SourceGuidHash = SourceGuidHash;

	if (Outer == nullptr)
	{
		Outer = CreatePackage(NULL, *AssetLongName);
		Outer->FullyLoad();
		Outer->Modify();
	}

	UTexture2D* Texture = FImageUtils::CreateTexture2D(Size.X, Size.Y, Samples, Outer, FPackageName::GetShortName(AssetLongName), Flags, TexParams);
	Texture->LODGroup = LODGroup;
	Texture->PostEditChange();
		
	return Texture;
}

bool FMaterialUtilities::ExportBaseColor(ULandscapeComponent* LandscapeComponent, int32 TextureSize, TArray<FColor>& OutSamples)
{
	ALandscapeProxy* LandscapeProxy = LandscapeComponent->GetLandscapeProxy();

	FIntPoint ComponentOrigin = LandscapeComponent->GetSectionBase() - LandscapeProxy->LandscapeSectionOffset;
	FIntPoint ComponentSize(LandscapeComponent->ComponentSizeQuads, LandscapeComponent->ComponentSizeQuads);
	FVector MidPoint = FVector(ComponentOrigin, 0.f) + FVector(ComponentSize, 0.f)*0.5f;

	FVector LandscapeCenter = LandscapeProxy->GetTransform().TransformPosition(MidPoint);
	FVector LandscapeExtent = FVector(ComponentSize, 0.f)*LandscapeProxy->GetActorScale()*0.5f;

	FVector ViewOrigin = LandscapeCenter;
	FMatrix ViewRotationMatrix = FInverseRotationMatrix(LandscapeProxy->GetActorRotation());
	ViewRotationMatrix *= FMatrix(FPlane(1, 0, 0, 0),
		FPlane(0, -1, 0, 0),
		FPlane(0, 0, -1, 0),
		FPlane(0, 0, 0, 1));

	const float ZOffset = WORLD_MAX;
	FMatrix ProjectionMatrix = FReversedZOrthoMatrix(
		LandscapeExtent.X,
		LandscapeExtent.Y,
		0.5f / ZOffset,
		ZOffset);

	FSceneInterface* Scene = LandscapeProxy->GetWorld()->Scene;

	// Hide all but the component
	TSet<FPrimitiveComponentId> HiddenPrimitives;
	for (auto PrimitiveComponentId : Scene->GetScenePrimitiveComponentIds())
	{
		HiddenPrimitives.Add(PrimitiveComponentId);
	}
	HiddenPrimitives.Remove(LandscapeComponent->SceneProxy->GetPrimitiveComponentId());
				
	FIntPoint TargetSize(TextureSize, TextureSize);

	// Render diffuse texture using BufferVisualizationMode=BaseColor
	static const FName BaseColorName("BaseColor");
	const float BaseColorGamma = 2.2f;
	RenderSceneToTexture(Scene, BaseColorName, ViewOrigin, ViewRotationMatrix, ProjectionMatrix, HiddenPrimitives, TargetSize, BaseColorGamma, OutSamples);
	return true;
}