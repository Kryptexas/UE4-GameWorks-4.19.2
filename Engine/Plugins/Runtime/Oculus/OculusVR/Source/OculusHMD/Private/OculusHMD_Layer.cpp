// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "OculusHMD_Layer.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS
//#include "MediaTexture.h"
//#include "ScreenRendering.h"
//#include "ScenePrivate.h"
//#include "PostProcess/SceneFilterRendering.h"
#include "PostProcess/SceneRenderTargets.h"
#include "HeadMountedDisplayTypes.h" // for LogHMD


namespace OculusHMD
{

//-------------------------------------------------------------------------------------------------
// FOvrpLayer
//-------------------------------------------------------------------------------------------------

FOvrpLayer::FOvrpLayer(uint32 InOvrpLayerId) : 
	OvrpLayerId(InOvrpLayerId)
{
}


FOvrpLayer::~FOvrpLayer()
{
	if (InRenderThread())
	{
		ExecuteOnRHIThread_DoNotWait([this]()
		{
			ovrp_DestroyLayer(OvrpLayerId);
		});
	}
	else
	{
		ovrp_DestroyLayer(OvrpLayerId);
	}
}


//-------------------------------------------------------------------------------------------------
// FLayer
//-------------------------------------------------------------------------------------------------

FLayer::FLayer(uint32 InId, const IStereoLayers::FLayerDesc& InDesc) :
	Id(InId),
	Desc(InDesc),
	OvrpLayerId(0),
	bUpdateTexture(false),
	bInvertY(true),
	bHasDepth(false)
{
	FMemory::Memzero(OvrpLayerDesc);
	FMemory::Memzero(OvrpLayerSubmit);
}


FLayer::FLayer(const FLayer& Layer) :
	Id(Layer.Id),
	Desc(Layer.Desc),
	OvrpLayerId(Layer.OvrpLayerId),
	OvrpLayer(Layer.OvrpLayer),
	TextureSetProxy(Layer.TextureSetProxy),
	DepthTextureSetProxy(Layer.DepthTextureSetProxy),
	RightTextureSetProxy(Layer.RightTextureSetProxy),
	RightDepthTextureSetProxy(Layer.RightDepthTextureSetProxy),
	bUpdateTexture(Layer.bUpdateTexture),
	bInvertY(Layer.bInvertY),
	bHasDepth(Layer.bHasDepth)
{
	FMemory::Memcpy(&OvrpLayerDesc, &Layer.OvrpLayerDesc, sizeof(OvrpLayerDesc));
	FMemory::Memcpy(&OvrpLayerSubmit, &Layer.OvrpLayerSubmit, sizeof(OvrpLayerSubmit));
}


FLayer::~FLayer()
{
}


void FLayer::SetDesc(const IStereoLayers::FLayerDesc& InDesc)
{
	if (Desc.Texture != InDesc.Texture || Desc.LeftTexture != InDesc.LeftTexture)
	{
		bUpdateTexture = true;
	}

	Desc = InDesc;
}


void FLayer::SetEyeLayerDesc(const ovrpLayerDesc_EyeFov& InEyeLayerDesc, const ovrpRecti InViewportRect[ovrpEye_Count])
{
	OvrpLayerDesc.EyeFov = InEyeLayerDesc;

	for(int eye = 0; eye < ovrpEye_Count; eye++)
	{
		OvrpLayerSubmit.ViewportRect[eye] = InViewportRect[eye];
	}

	bHasDepth = InEyeLayerDesc.DepthFormat != ovrpTextureFormat_None;
}


TSharedPtr<FLayer, ESPMode::ThreadSafe> FLayer::Clone() const
{
	return MakeShareable(new FLayer(*this));
}


bool FLayer::CanReuseResources(const FLayer* InLayer) const
{
	if (!InLayer || !InLayer->OvrpLayer.IsValid())
	{
		return false;
	}

	if (OvrpLayerDesc.Shape != InLayer->OvrpLayerDesc.Shape ||
		OvrpLayerDesc.Layout != InLayer->OvrpLayerDesc.Layout ||
		OvrpLayerDesc.TextureSize.w != InLayer->OvrpLayerDesc.TextureSize.w ||
		OvrpLayerDesc.TextureSize.h != InLayer->OvrpLayerDesc.TextureSize.h ||
		OvrpLayerDesc.MipLevels != InLayer->OvrpLayerDesc.MipLevels ||
		OvrpLayerDesc.SampleCount != InLayer->OvrpLayerDesc.SampleCount ||
		OvrpLayerDesc.Format != InLayer->OvrpLayerDesc.Format ||
		((OvrpLayerDesc.LayerFlags ^ InLayer->OvrpLayerDesc.LayerFlags) & ovrpLayerFlag_Static))
	{
		return false;
	}

	if (OvrpLayerDesc.Shape == ovrpShape_EyeFov)
	{
		if (OvrpLayerDesc.EyeFov.DepthFormat != InLayer->OvrpLayerDesc.EyeFov.DepthFormat)
		{
			return false;
		}
	}

	return true;
}


void FLayer::Initialize_RenderThread(FCustomPresent* CustomPresent, FRHICommandListImmediate& RHICmdList, const FLayer* InLayer)
{
	CheckInRenderThread();

	if (Id == 0)
	{
		// OvrpLayerDesc and OvrpViewportRects already initialized, as this is the eyeFOV layer. The only necessary modification is to take into account MSAA level, that can only be accurately determined on the RT.
	}
	else if (Desc.Texture.IsValid())
	{
		if (Desc.UVRect.Min.Y == 1.0f)
		{
			bInvertY = false;
			Desc.UVRect.Min.Y = 0.0f;
		}

		FRHITexture2D* Texture2D = Desc.Texture->GetTexture2D();
		FRHITextureCube* TextureCube = Desc.Texture->GetTextureCube();

		uint32 SizeX, SizeY;

		if(Texture2D)
		{
			SizeX = Texture2D->GetSizeX();
			SizeY = Texture2D->GetSizeY();
		}
		else if(TextureCube)
		{
			SizeX = SizeY = TextureCube->GetSize();
		}
		else
		{
			return;
		}

		ovrpShape Shape;
		
		switch (Desc.ShapeType)
		{
		case IStereoLayers::QuadLayer:
			Shape = ovrpShape_Quad;
			break;

		case IStereoLayers::CylinderLayer:
			Shape = ovrpShape_Cylinder;
			break;

		case IStereoLayers::CubemapLayer:
			Shape = ovrpShape_Cubemap;
			break;

		default:
			return;
		}

		EPixelFormat Format = CustomPresent->GetPixelFormat(Desc.Texture->GetFormat());
#if PLATFORM_ANDROID
		uint32 NumMips = 1;
#else
		uint32 NumMips = 0;
#endif
		uint32 NumSamples = 1;
		int LayerFlags = 0;

		if(!(Desc.Flags & IStereoLayers::LAYER_FLAG_TEX_CONTINUOUS_UPDATE))
			LayerFlags |= ovrpLayerFlag_Static;

		// Calculate layer desc
		ovrp_CalculateLayerDesc(
			Shape,
			!Desc.LeftTexture.IsValid() ? ovrpLayout_Mono : ovrpLayout_Stereo,
			ovrpSizei { (int) SizeX, (int) SizeY },
			NumMips,
			NumSamples,
			CustomPresent->GetOvrpTextureFormat(Format),
			LayerFlags,
			&OvrpLayerDesc);

		// Calculate viewport rect
		for (uint32 EyeIndex = 0; EyeIndex < ovrpEye_Count; EyeIndex++)
		{
			ovrpRecti& ViewportRect = OvrpLayerSubmit.ViewportRect[EyeIndex];
			ViewportRect.Pos.x = (int)(Desc.UVRect.Min.X * SizeX + 0.5f);
			ViewportRect.Pos.y = (int)(Desc.UVRect.Min.Y * SizeY + 0.5f);
			ViewportRect.Size.w = (int)(Desc.UVRect.Max.X * SizeX + 0.5f) - ViewportRect.Pos.x;
			ViewportRect.Size.h = (int)(Desc.UVRect.Max.Y * SizeY + 0.5f) - ViewportRect.Pos.y;
		}
	}
	else
	{
		return;
	}
	
	// Reuse/Create texture set
	if (CanReuseResources(InLayer))
	{
		OvrpLayerId = InLayer->OvrpLayerId;
		OvrpLayer = InLayer->OvrpLayer;
		TextureSetProxy = InLayer->TextureSetProxy;
		DepthTextureSetProxy = InLayer->DepthTextureSetProxy;
		RightTextureSetProxy = InLayer->RightTextureSetProxy;
		RightDepthTextureSetProxy = InLayer->RightDepthTextureSetProxy;
		bUpdateTexture = InLayer->bUpdateTexture;
	}
	else
	{
		bool bLayerCreated = false;
		TArray<ovrpTextureHandle> ColorTextures;
		TArray<ovrpTextureHandle> DepthTextures;
		TArray<ovrpTextureHandle> RightColorTextures;
		TArray<ovrpTextureHandle> RightDepthTextures;

		ExecuteOnRHIThread([&]()
		{
			// UNDONE Do this in RenderThread once OVRPlugin allows ovrp_SetupLayer to be called asynchronously
			int32 TextureCount;
			if (OVRP_SUCCESS(ovrp_SetupLayer(CustomPresent->GetOvrpDevice(), OvrpLayerDesc.Base, (int*) &OvrpLayerId)) &&
				OVRP_SUCCESS(ovrp_GetLayerTextureStageCount(OvrpLayerId, &TextureCount)))
			{
				// Left
				{
					ColorTextures.SetNum(TextureCount);
					if (bHasDepth)
					{
						DepthTextures.SetNum(TextureCount);
					}
					

					for (int32 TextureIndex = 0; TextureIndex < TextureCount; TextureIndex++)
					{
						ovrpTextureHandle* DepthTexHdlPtr = bHasDepth ? &DepthTextures[TextureIndex] : nullptr;
						if (!OVRP_SUCCESS(ovrp_GetLayerTexture2(OvrpLayerId, TextureIndex, ovrpEye_Left, &ColorTextures[TextureIndex], DepthTexHdlPtr)))
						{
							UE_LOG(LogHMD, Error, TEXT("Failed to create Oculus layer texture. NOTE: This causes a leak of %d other texture(s), which will go unused."), TextureIndex);
							// skip setting bLayerCreated and allocating any other textures
							return;
						}
					}
				}

				// Right
				if(OvrpLayerDesc.Layout == ovrpLayout_Stereo)
				{
					RightColorTextures.SetNum(TextureCount);
					if (bHasDepth)
					{
						RightDepthTextures.SetNum(TextureCount);
					}

					for (int32 TextureIndex = 0; TextureIndex < TextureCount; TextureIndex++)
					{
						ovrpTextureHandle* DepthTexHdlPtr = bHasDepth ? &RightDepthTextures[TextureIndex] : nullptr;
						if (!OVRP_SUCCESS(ovrp_GetLayerTexture2(OvrpLayerId, TextureIndex, ovrpEye_Right, &RightColorTextures[TextureIndex], DepthTexHdlPtr)))
						{
							UE_LOG(LogHMD, Error, TEXT("Failed to create Oculus layer texture. NOTE: This causes a leak of %d other texture(s), which will go unused."), TextureCount + TextureIndex);
							// skip setting bLayerCreated and allocating any other textures
							return;
						}
					}
				}

				bLayerCreated = true;
			}
		});

		if(bLayerCreated)
		{
			OvrpLayer = MakeShareable<FOvrpLayer>(new FOvrpLayer(OvrpLayerId));

			uint32 SizeX = OvrpLayerDesc.TextureSize.w;
			uint32 SizeY = OvrpLayerDesc.TextureSize.h;
			EPixelFormat ColorFormat = CustomPresent->GetPixelFormat(OvrpLayerDesc.Format);
			EPixelFormat DepthFormat = PF_DepthStencil;
			uint32 NumMips = OvrpLayerDesc.MipLevels;
			uint32 NumSamples = OvrpLayerDesc.SampleCount;
			uint32 NumSamplesTileMem = 1;
			if (OvrpLayerDesc.Shape == ovrpShape_EyeFov)
			{
				ovrp_GetSystemRecommendedMSAALevel2((int*) &NumSamplesTileMem);
			}

			ERHIResourceType ResourceType;			
			if (OvrpLayerDesc.Shape == ovrpShape_Cubemap || OvrpLayerDesc.Shape == ovrpShape_OffcenterCubemap)
			{
				ResourceType = RRT_TextureCube;
			}
			else if (OvrpLayerDesc.Layout == ovrpLayout_Array)
			{
				ResourceType = RRT_Texture2DArray;
			}
			else
			{
				ResourceType = RRT_Texture2D;
			}

			FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(RHICmdList);

			uint32 ColorTexCreateFlags = TexCreate_ShaderResource | TexCreate_RenderTargetable;
			uint32 DepthTexCreateFlags = TexCreate_ShaderResource | TexCreate_DepthStencilTargetable;
#if PLATFORM_ANDROID
			FClearValueBinding ColorTextureBinding = FClearValueBinding();
#else
			FClearValueBinding ColorTextureBinding = FClearValueBinding::Black;
#endif
			FClearValueBinding DepthTextureBinding = SceneContext.GetDefaultDepthClear();

			TextureSetProxy = CustomPresent->CreateTextureSetProxy_RenderThread(SizeX, SizeY, ColorFormat, ColorTextureBinding, NumMips, NumSamples, NumSamplesTileMem, ResourceType, ColorTextures, ColorTexCreateFlags);

			if (bHasDepth)
			{
				DepthTextureSetProxy = CustomPresent->CreateTextureSetProxy_RenderThread(SizeX, SizeY, DepthFormat, DepthTextureBinding, 1, NumSamples, NumSamplesTileMem, ResourceType, DepthTextures, DepthTexCreateFlags);
			}

			if (OvrpLayerDesc.Layout == ovrpLayout_Stereo)
			{
				RightTextureSetProxy = CustomPresent->CreateTextureSetProxy_RenderThread(SizeX, SizeY, ColorFormat, ColorTextureBinding, NumMips, NumSamples, NumSamplesTileMem, ResourceType, RightColorTextures, ColorTexCreateFlags);

				if (bHasDepth)
				{
					RightDepthTextureSetProxy = CustomPresent->CreateTextureSetProxy_RenderThread(SizeX, SizeY, DepthFormat, DepthTextureBinding, 1, NumSamples, NumSamplesTileMem, ResourceType, RightDepthTextures, DepthTexCreateFlags);
				}
			}
		}

		bUpdateTexture = true;
	}

	if (Desc.Flags & IStereoLayers::LAYER_FLAG_TEX_CONTINUOUS_UPDATE)
	{
		bUpdateTexture = true;
	}
}

void FLayer::UpdateTexture_RenderThread(FCustomPresent* CustomPresent, FRHICommandListImmediate& RHICmdList)
{
	CheckInRenderThread();

	if (bUpdateTexture && TextureSetProxy.IsValid())
	{
		// Copy textures
		if (Desc.Texture.IsValid())
		{
			bool bAlphaPremultiply = true;
			bool bNoAlphaWrite = (Desc.Flags & IStereoLayers::LAYER_FLAG_TEX_NO_ALPHA_CHANNEL) != 0;
			bool bIsCubemap = (Desc.ShapeType == IStereoLayers::ELayerShape::CubemapLayer);

			// Left
			{
				FRHITexture* SrcTexture = Desc.LeftTexture.IsValid() ? Desc.LeftTexture : Desc.Texture;
				FRHITexture* DstTexture = TextureSetProxy->GetTexture();

				const ovrpRecti& OvrpViewportRect = OvrpLayerSubmit.ViewportRect[ovrpEye_Left];
				FIntRect DstRect(OvrpViewportRect.Pos.x, OvrpViewportRect.Pos.y, OvrpViewportRect.Pos.x + OvrpViewportRect.Size.w, OvrpViewportRect.Pos.y + OvrpViewportRect.Size.h);

				CustomPresent->CopyTexture_RenderThread(RHICmdList, DstTexture, SrcTexture, DstRect, FIntRect(), bAlphaPremultiply, bNoAlphaWrite, bInvertY);
			}

			// Right
			if(OvrpLayerDesc.Layout != ovrpLayout_Mono)
			{
				FRHITexture* SrcTexture = Desc.Texture;
				FRHITexture* DstTexture = RightTextureSetProxy.IsValid() ? RightTextureSetProxy->GetTexture() : TextureSetProxy->GetTexture();

				const ovrpRecti& OvrpViewportRect = OvrpLayerSubmit.ViewportRect[ovrpEye_Right];
				FIntRect DstRect(OvrpViewportRect.Pos.x, OvrpViewportRect.Pos.y, OvrpViewportRect.Pos.x + OvrpViewportRect.Size.w, OvrpViewportRect.Pos.y + OvrpViewportRect.Size.h);

				CustomPresent->CopyTexture_RenderThread(RHICmdList, DstTexture, SrcTexture, DstRect, FIntRect(), bAlphaPremultiply, bNoAlphaWrite, bInvertY);
			}

			bUpdateTexture = false;
		}

		// Generate mips
		TextureSetProxy->GenerateMips_RenderThread(RHICmdList);

		if (RightTextureSetProxy.IsValid())
		{
			RightTextureSetProxy->GenerateMips_RenderThread(RHICmdList);
		}
	}
}


const ovrpLayerSubmit* FLayer::UpdateLayer_RHIThread(const FSettings* Settings, const FGameFrame* Frame)
{
	OvrpLayerSubmit.LayerId = OvrpLayerId;
	OvrpLayerSubmit.TextureStage = TextureSetProxy.IsValid() ? TextureSetProxy->GetSwapChainIndex_RHIThread() : 0;

	if (Id != 0)
	{
		int SizeX = OvrpLayerDesc.TextureSize.w;
		int SizeY = OvrpLayerDesc.TextureSize.h;

		float AspectRatio = SizeX ? (float)SizeY / (float)SizeX : 3.0f / 4.0f;
		FVector LocationScaleInv(Frame->WorldToMetersScale);
		FVector LocationScale = LocationScaleInv.Reciprocal();
		ovrpVector3f Scale = ToOvrpVector3f(Desc.Transform.GetScale3D() * LocationScale);

		switch (OvrpLayerDesc.Shape)
		{
		case ovrpShape_Quad:
			{
				float QuadSizeY = (Desc.Flags & IStereoLayers::LAYER_FLAG_QUAD_PRESERVE_TEX_RATIO) ? Desc.QuadSize.X * AspectRatio : Desc.QuadSize.Y;
				OvrpLayerSubmit.Quad.Size = ovrpSizef { Desc.QuadSize.X * Scale.x, QuadSizeY * Scale.y };
			}
			break;
		case ovrpShape_Cylinder:
			{
				float CylinderHeight = (Desc.Flags & IStereoLayers::LAYER_FLAG_QUAD_PRESERVE_TEX_RATIO) ? Desc.CylinderOverlayArc * AspectRatio : Desc.CylinderHeight;
				OvrpLayerSubmit.Cylinder.ArcWidth = Desc.CylinderOverlayArc * Scale.x;
				OvrpLayerSubmit.Cylinder.Height = CylinderHeight * Scale.x;
				OvrpLayerSubmit.Cylinder.Radius = Desc.CylinderRadius * Scale.x;
			}
			break;
		}

		FQuat BaseOrientation;
		FVector BaseLocation;

		switch (Desc.PositionType)
		{
		case IStereoLayers::WorldLocked:
			BaseOrientation = Frame->PlayerOrientation;
			BaseLocation = Frame->PlayerLocation;
			break;

		case IStereoLayers::TrackerLocked:
			BaseOrientation = FQuat::Identity;
			BaseLocation = FVector::ZeroVector;
			break;

		case IStereoLayers::FaceLocked:
			BaseOrientation = Settings->BaseOrientation;
			BaseLocation = Settings->BaseOffset * LocationScaleInv;
			break;
		}

		FTransform playerTransform(BaseOrientation, BaseLocation);

		FQuat Orientation = Desc.Transform.Rotator().Quaternion();
		FVector Location = Desc.Transform.GetLocation();

		OvrpLayerSubmit.Pose.Orientation = ToOvrpQuatf(BaseOrientation.Inverse() * Orientation);
		OvrpLayerSubmit.Pose.Position = ToOvrpVector3f((playerTransform.InverseTransformPosition(Location)) * LocationScale);
		OvrpLayerSubmit.LayerSubmitFlags = 0;

		if (Desc.PositionType == IStereoLayers::FaceLocked)
		{
			OvrpLayerSubmit.LayerSubmitFlags |= ovrpLayerSubmitFlag_HeadLocked;
		}
	}
	else
	{
		OvrpLayerSubmit.EyeFov.DepthFar = 0;
		OvrpLayerSubmit.EyeFov.DepthNear = Frame->NearClippingPlane / 100.f; //physical scale is 100UU/meter
		OvrpLayerSubmit.LayerSubmitFlags |= ovrpLayerSubmitFlag_ReverseZ;

		if (Frame->Flags.bPixelDensityAdaptive)
		{
			OvrpLayerSubmit.ViewportRect[0] = ToOvrpRecti(Frame->FinalViewRect[0]);
			OvrpLayerSubmit.ViewportRect[1] = ToOvrpRecti(Frame->FinalViewRect[1]);
		}
	}

	return &OvrpLayerSubmit.Base;
}


void FLayer::IncrementSwapChainIndex_RHIThread(FCustomPresent* CustomPresent)
{
	CheckInRHIThread();

	if (TextureSetProxy.IsValid())
	{
		TextureSetProxy->IncrementSwapChainIndex_RHIThread(CustomPresent);
	}

	if (DepthTextureSetProxy.IsValid())
	{
		DepthTextureSetProxy->IncrementSwapChainIndex_RHIThread(CustomPresent);
	}

	if (RightTextureSetProxy.IsValid())
	{
		RightTextureSetProxy->IncrementSwapChainIndex_RHIThread(CustomPresent);
	}

	if (RightDepthTextureSetProxy.IsValid())
	{
		RightDepthTextureSetProxy->IncrementSwapChainIndex_RHIThread(CustomPresent);
	}
}


void FLayer::ReleaseResources_RHIThread()
{
	CheckInRHIThread();

	OvrpLayerId = 0;
	OvrpLayer.Reset();
	TextureSetProxy.Reset();
	DepthTextureSetProxy.Reset();
	RightTextureSetProxy.Reset();
	RightDepthTextureSetProxy.Reset();
	bUpdateTexture = false;
}

// PokeAHole layer drawing implementation


static void DrawPokeAHoleQuadMesh(FRHICommandList& RHICmdList, const FMatrix& PosTransform, float X, float Y, float Z, float SizeX, float SizeY, float SizeZ, bool InvertCoords)
{
	float ClipSpaceQuadZ = 0.0f;

	FFilterVertex Vertices[4];
	Vertices[0].Position = PosTransform.TransformFVector4(FVector4(X, Y, Z, 1));
	Vertices[1].Position = PosTransform.TransformFVector4(FVector4(X + SizeX, Y, Z + SizeZ, 1));
	Vertices[2].Position = PosTransform.TransformFVector4(FVector4(X, Y + SizeY, Z, 1));
	Vertices[3].Position = PosTransform.TransformFVector4(FVector4(X + SizeX, Y + SizeY, Z + SizeZ, 1));

	if (InvertCoords)
	{
		Vertices[0].UV = FVector2D(1, 0);
		Vertices[1].UV = FVector2D(1, 1);
		Vertices[2].UV = FVector2D(0, 0);
		Vertices[3].UV = FVector2D(0, 1);
	}
	else
	{
		Vertices[0].UV = FVector2D(0, 1);
		Vertices[1].UV = FVector2D(0, 0);
		Vertices[2].UV = FVector2D(1, 1);
		Vertices[3].UV = FVector2D(1, 0);
	}

	static const uint16 Indices[] = { 0, 1, 3, 0, 3, 2 };

	DrawIndexedPrimitiveUP(RHICmdList, PT_TriangleList, 0, 4, 2, Indices, sizeof(Indices[0]), Vertices, sizeof(Vertices[0]));
}

static void DrawPokeAHoleCylinderMesh(FRHICommandList& RHICmdList, FVector Base, FVector X, FVector Y, const FMatrix& PosTransform, float ArcAngle, float CylinderHeight, float CylinderRadius, bool InvertCoords)
{
	float ClipSpaceQuadZ = 0.0f;
	const int Sides = 40;

	FFilterVertex Vertices[2 * (Sides + 1)];
	static uint16 Indices[6 * Sides]; //	 = { 0, 1, 3, 0, 3, 2 };

	float currentAngle = -ArcAngle / 2;
	float angleStep = ArcAngle / Sides;

	FVector LastVertex = Base + CylinderRadius * (FMath::Cos(currentAngle) * X + FMath::Sin(currentAngle) * Y);
	FVector HalfHeight = FVector(0, 0, CylinderHeight / 2);

	Vertices[0].Position = PosTransform.TransformFVector4(LastVertex - HalfHeight);
	Vertices[1].Position = PosTransform.TransformFVector4(LastVertex + HalfHeight);
	Vertices[0].UV = FVector2D(1, 0);
	Vertices[1].UV = FVector2D(1, 1);

	currentAngle += angleStep;

	for (int side = 0; side < Sides; side++)
	{
		FVector ThisVertex = Base + CylinderRadius * (FMath::Cos(currentAngle) * X + FMath::Sin(currentAngle) * Y);
		currentAngle += angleStep;

		Vertices[2 * (side + 1)].Position = PosTransform.TransformFVector4(ThisVertex - HalfHeight);
		Vertices[2 * (side + 1) + 1].Position = PosTransform.TransformFVector4(ThisVertex + HalfHeight);
		Vertices[2 * (side + 1)].UV = FVector2D(1 - (side + 1) / (float)Sides, 0);
		Vertices[2 * (side + 1) + 1].UV = FVector2D(1 - (side + 1) / (float)Sides, 1);

		Indices[6 * side + 0] = 2 * side;
		Indices[6 * side + 1] = 2 * side + 1;
		Indices[6 * side + 2] = 2 * (side + 1) + 1;
		Indices[6 * side + 3] = 2 * side;
		Indices[6 * side + 4] = 2 * (side + 1) + 1;
		Indices[6 * side + 5] = 2 * (side + 1);

		LastVertex = ThisVertex;
	}

	DrawIndexedPrimitiveUP(RHICmdList, PT_TriangleList, 0, 2 * (Sides + 1), 2 * Sides, Indices, sizeof(Indices[0]), Vertices, sizeof(Vertices[0]));
}

void FLayer::DrawPokeAHoleMesh(FRHICommandList& RHICmdList, const FMatrix& matrix, float scale, bool invertCoords)
{
	int SizeX = OvrpLayerDesc.TextureSize.w;
	int SizeY = OvrpLayerDesc.TextureSize.h;
	float AspectRatio = SizeX ? (float)SizeY / (float)SizeX : 3.0f / 4.0f;

	FMatrix MultipliedMatrix = matrix;

	if (invertCoords)
	{
		FMatrix multiplierMatrix;
		multiplierMatrix.SetIdentity();
		multiplierMatrix.M[1][1] = -1;
		MultipliedMatrix *= multiplierMatrix;
	}

	if (OvrpLayerDesc.Shape == ovrpShape_Quad)
	{
		float QuadSizeY = (Desc.Flags & IStereoLayers::LAYER_FLAG_QUAD_PRESERVE_TEX_RATIO) ? Desc.QuadSize.X * AspectRatio : Desc.QuadSize.Y;

		FVector2D quadsize = FVector2D(Desc.QuadSize.X, QuadSizeY);

		DrawPokeAHoleQuadMesh(RHICmdList, MultipliedMatrix, 0, -quadsize.X*scale / 2, -quadsize.Y*scale / 2, 0, quadsize.X*scale, quadsize.Y*scale, invertCoords);
	}
	else if (OvrpLayerDesc.Shape == ovrpShape_Cylinder)
	{
		float CylinderHeight = (Desc.Flags & IStereoLayers::LAYER_FLAG_QUAD_PRESERVE_TEX_RATIO) ? Desc.CylinderOverlayArc * AspectRatio : Desc.CylinderHeight;

		FVector XAxis = FVector(1, 0, 0);
		FVector YAxis = FVector(0, 1, 0);
		FVector Base = FVector::ZeroVector;

		float CylinderRadius = Desc.CylinderRadius;
		float ArcAngle = Desc.CylinderOverlayArc / Desc.CylinderRadius;

		DrawPokeAHoleCylinderMesh(RHICmdList, Base, XAxis, YAxis, MultipliedMatrix, ArcAngle*scale, CylinderHeight*scale, CylinderRadius, invertCoords);
	}
	else if (OvrpLayerDesc.Shape == ovrpShape_Cubemap)
	{
		DrawPokeAHoleQuadMesh(RHICmdList, FMatrix::Identity, -1, -1, 0, 2, 2, 0, false);
	}
}

} // namespace OculusHMD

#endif //OCULUS_HMD_SUPPORTED_PLATFORMS
