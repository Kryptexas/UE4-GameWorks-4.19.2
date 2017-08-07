// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "OculusHMD_Layer.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS
//#include "MediaTexture.h"
//#include "ScreenRendering.h"
//#include "ScenePrivate.h"
//#include "PostProcess/SceneFilterRendering.h"


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
	bUpdateTexture(false)
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
	RightTextureSetProxy(Layer.RightTextureSetProxy),
	bUpdateTexture(Layer.bUpdateTexture)
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
}


TSharedPtr<FLayer, ESPMode::ThreadSafe> FLayer::Clone() const
{
	return MakeShareable(new FLayer(*this));
}


void FLayer::Initialize_RenderThread(FCustomPresent* CustomPresent, const FLayer* InLayer)
{
	CheckInRenderThread();

	if (Id == 0)
	{
		// OvrpLayerDesc and OvrpViewportRects already initialized, as this is the eyeFOV layer.
	}
	else if (Desc.Texture.IsValid())
	{
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
		bool bSRGB = true;
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
			CustomPresent->GetOvrpTextureFormat(Format, bSRGB),
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
	if (InLayer && InLayer->OvrpLayer.IsValid() && !FMemory::Memcmp(&OvrpLayerDesc, &InLayer->OvrpLayerDesc, sizeof(OvrpLayerDesc)))
	{
		OvrpLayerId = InLayer->OvrpLayerId;
		OvrpLayer = InLayer->OvrpLayer;
		TextureSetProxy = InLayer->TextureSetProxy;
		RightTextureSetProxy = InLayer->RightTextureSetProxy;
		bUpdateTexture = InLayer->bUpdateTexture;
	}
	else
	{
		bool bLayerCreated = false;
		TArray<ovrpTextureHandle> Textures;
		TArray<ovrpTextureHandle> RightTextures;

		ExecuteOnRHIThread([&]()
		{
			// UNDONE Do this in RenderThread once OVRPlugin allows ovrp_SetupLayer to be called asynchronously
			int32 TextureCount;
			if (OVRP_SUCCESS(ovrp_SetupLayer(CustomPresent->GetOvrpDevice(), OvrpLayerDesc.Base, (int*) &OvrpLayerId)) &&
				OVRP_SUCCESS(ovrp_GetLayerTextureStageCount(OvrpLayerId, &TextureCount)))
			{
				// Left
				{
					Textures.SetNum(TextureCount);

					for (int32 TextureIndex = 0; TextureIndex < TextureCount; TextureIndex++)
					{
						ovrp_GetLayerTexture(OvrpLayerId, TextureIndex, ovrpEye_Left, &Textures[TextureIndex]);
					}
				}

				// Right
				if(OvrpLayerDesc.Layout == ovrpLayout_Stereo)
				{
					RightTextures.SetNum(TextureCount);

					for (int32 TextureIndex = 0; TextureIndex < TextureCount; TextureIndex++)
					{
						ovrp_GetLayerTexture(OvrpLayerId, TextureIndex, ovrpEye_Right, &RightTextures[TextureIndex]);
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
			EPixelFormat Format = CustomPresent->GetPixelFormat(OvrpLayerDesc.Format);
			uint32 NumMips = OvrpLayerDesc.MipLevels;
			uint32 NumSamples = OvrpLayerDesc.SampleCount;
			uint32 NumSamplesTileMem = 1;
			if (OvrpLayerDesc.Shape == ovrpShape_EyeFov)
			{
				int MSAALevel;
				ovrp_GetSystemRecommendedMSAALevel2(&MSAALevel);
				NumSamplesTileMem = MSAALevel;
			}
			uint32 ArraySize = OvrpLayerDesc.Layout == ovrpLayout_Array ? 2 : 1;
			bool bIsCubemap = OvrpLayerDesc.Shape == ovrpShape_Cubemap;

			TextureSetProxy = CustomPresent->CreateTextureSet_RenderThread(SizeX, SizeY, Format, NumMips, NumSamples, NumSamplesTileMem, ArraySize, bIsCubemap, Textures);

			if(OvrpLayerDesc.Layout == ovrpLayout_Stereo)
			{
				RightTextureSetProxy = CustomPresent->CreateTextureSet_RenderThread(SizeX, SizeY, Format, NumMips, NumSamples, NumSamplesTileMem, ArraySize, bIsCubemap, RightTextures);
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

				CustomPresent->CopyTexture_RenderThread(RHICmdList, DstTexture, SrcTexture, DstRect, FIntRect(), bAlphaPremultiply, bNoAlphaWrite);
			}

			// Right
			if(OvrpLayerDesc.Layout != ovrpLayout_Mono)
			{
				FRHITexture* SrcTexture = Desc.Texture;
				FRHITexture* DstTexture = RightTextureSetProxy.IsValid() ? RightTextureSetProxy->GetTexture() : TextureSetProxy->GetTexture();

				const ovrpRecti& OvrpViewportRect = OvrpLayerSubmit.ViewportRect[ovrpEye_Right];
				FIntRect DstRect(OvrpViewportRect.Pos.x, OvrpViewportRect.Pos.y, OvrpViewportRect.Pos.x + OvrpViewportRect.Size.w, OvrpViewportRect.Pos.y + OvrpViewportRect.Size.h);

				CustomPresent->CopyTexture_RenderThread(RHICmdList, DstTexture, SrcTexture, DstRect, FIntRect(), bAlphaPremultiply, bNoAlphaWrite);
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
		FVector LocationScaleInv = Frame->WorldToMetersScale * Frame->PositionScale;
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

	return &OvrpLayerSubmit.Base;
}


void FLayer::IncrementSwapChainIndex_RHIThread()
{
	CheckInRHIThread();

	if (TextureSetProxy.IsValid())
	{
		TextureSetProxy->IncrementSwapChainIndex_RHIThread();
	}

	if (RightTextureSetProxy.IsValid())
	{
		RightTextureSetProxy->IncrementSwapChainIndex_RHIThread();
	}
}


void FLayer::ReleaseResources_RHIThread()
{
	CheckInRHIThread();

	OvrpLayerId = 0;
	OvrpLayer.Reset();
	TextureSetProxy.Reset();
	RightTextureSetProxy.Reset();
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

void FLayer::DrawPokeAHoleMesh(FRHICommandList& RHICmdList, const FMatrix& InMatrix, float scale, bool invertCoords)
{
	FMatrix matrix = InMatrix;

	int SizeX = OvrpLayerDesc.TextureSize.w;
	int SizeY = OvrpLayerDesc.TextureSize.h;
	float AspectRatio = SizeX ? (float)SizeY / (float)SizeX : 3.0f / 4.0f;

	if (invertCoords)
	{
		FMatrix multiplierMatrix;
		multiplierMatrix.SetIdentity();
		multiplierMatrix.M[1][1] = -1;
		matrix *= multiplierMatrix;
	}

	if (OvrpLayerDesc.Shape == ovrpShape_Quad)
	{
		float QuadSizeY = (Desc.Flags & IStereoLayers::LAYER_FLAG_QUAD_PRESERVE_TEX_RATIO) ? Desc.QuadSize.X * AspectRatio : Desc.QuadSize.Y;

		FVector2D quadsize = FVector2D(Desc.QuadSize.X, QuadSizeY);

		DrawPokeAHoleQuadMesh(RHICmdList, matrix, 0, -quadsize.X*scale / 2, -quadsize.Y*scale / 2, 0, quadsize.X*scale, quadsize.Y*scale, invertCoords);
	}
	else if (OvrpLayerDesc.Shape == ovrpShape_Cylinder)
	{
		float CylinderHeight = (Desc.Flags & IStereoLayers::LAYER_FLAG_QUAD_PRESERVE_TEX_RATIO) ? Desc.CylinderOverlayArc * AspectRatio : Desc.CylinderHeight;

		FVector XAxis = FVector(1, 0, 0);
		FVector YAxis = FVector(0, 1, 0);
		FVector Base = FVector::ZeroVector;

		float CylinderRadius = Desc.CylinderRadius;
		float ArcAngle = Desc.CylinderOverlayArc / Desc.CylinderRadius;

		DrawPokeAHoleCylinderMesh(RHICmdList, Base, XAxis, YAxis, matrix, ArcAngle*scale, CylinderHeight*scale, CylinderRadius, invertCoords);
	}
	else if (OvrpLayerDesc.Shape == ovrpShape_Cubemap)
	{
		DrawPokeAHoleQuadMesh(RHICmdList, FMatrix::Identity, -1, -1, 0, 2, 2, 0, false);
	}
}

} // namespace OculusHMD

#endif //OCULUS_HMD_SUPPORTED_PLATFORMS
