// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*================================================================================
	MeshPaintRendering.cpp: Mesh texture paint brush rendering
================================================================================*/

#include "UnrealEd.h"
#include "MeshPaintRendering.h"
#include "GlobalShader.h"
#include "ShaderParameters.h"


namespace MeshPaintRendering
{

	/** Mesh paint vertex shader */
	class TMeshPaintVertexShader : public FGlobalShader
	{
		DECLARE_SHADER_TYPE( TMeshPaintVertexShader, Global );

	public:

		static bool ShouldCache( EShaderPlatform Platform )
		{
			return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3);
		}

		/** Default constructor. */
		TMeshPaintVertexShader() {}

		/** Initialization constructor. */
		TMeshPaintVertexShader( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
			: FGlobalShader( Initializer )
		{
			TransformParameter.Bind( Initializer.ParameterMap, TEXT( "c_Transform" ) );
		}

		virtual bool Serialize( FArchive& Ar )
		{
			bool bShaderHasOutdatedParameters = FGlobalShader::Serialize( Ar );
			Ar << TransformParameter;
			return bShaderHasOutdatedParameters;
		}

		void SetParameters( const FMatrix& InTransform )
		{
			SetShaderValue( GetVertexShader(), TransformParameter, InTransform );
		}

	private:

		FShaderParameter TransformParameter;
	};


	IMPLEMENT_SHADER_TYPE( , TMeshPaintVertexShader, TEXT( "MeshPaintVertexShader" ), TEXT( "Main" ), SF_Vertex);



	/** Mesh paint pixel shader */
	class TMeshPaintPixelShader : public FGlobalShader
	{
		DECLARE_SHADER_TYPE( TMeshPaintPixelShader, Global );

	public:

		static bool ShouldCache(EShaderPlatform Platform)
		{
			return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3);
		}

		/** Default constructor. */
		TMeshPaintPixelShader() {}

		/** Initialization constructor. */
		TMeshPaintPixelShader( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
			: FGlobalShader( Initializer )
		{
			CloneTextureParameter.Bind( Initializer.ParameterMap, TEXT( "s_CloneTexture" ) );
			CloneTextureParameterSampler.Bind( Initializer.ParameterMap, TEXT( "s_CloneTextureSampler" ));
			WorldToBrushMatrixParameter.Bind( Initializer.ParameterMap, TEXT( "c_WorldToBrushMatrix" ) );
			BrushMetricsParameter.Bind( Initializer.ParameterMap, TEXT( "c_BrushMetrics" ) );
			BrushStrengthParameter.Bind( Initializer.ParameterMap, TEXT( "c_BrushStrength" ) );
			BrushColorParameter.Bind( Initializer.ParameterMap, TEXT( "c_BrushColor" ) );
			ChannelFlagsParameter.Bind( Initializer.ParameterMap, TEXT( "c_ChannelFlags") );
			GenerateMaskFlagParameter.Bind( Initializer.ParameterMap, TEXT( "c_GenerateMaskFlag") );
			GammaParameter.Bind( Initializer.ParameterMap, TEXT( "c_Gamma" ) );
		}

		virtual bool Serialize(FArchive& Ar)
		{
			bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
			Ar << CloneTextureParameter;
			Ar << CloneTextureParameterSampler;
			Ar << WorldToBrushMatrixParameter;
			Ar << BrushMetricsParameter;
			Ar << BrushStrengthParameter;
			Ar << BrushColorParameter;
			Ar << ChannelFlagsParameter;
			Ar << GenerateMaskFlagParameter;
			Ar << GammaParameter;
			return bShaderHasOutdatedParameters;
		}

		void SetParameters( const float InGamma, const FMeshPaintShaderParameters& InShaderParams )
		{
			const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

			SetTextureParameter(
				ShaderRHI,
				CloneTextureParameter,
				CloneTextureParameterSampler,
				TStaticSamplerState< SF_Point, AM_Clamp, AM_Clamp, AM_Clamp >::GetRHI(),
				InShaderParams.CloneTexture->GetRenderTargetResource()->TextureRHI );

			SetShaderValue( ShaderRHI, WorldToBrushMatrixParameter, InShaderParams.WorldToBrushMatrix );

			FVector4 BrushMetrics;
			BrushMetrics.X = InShaderParams.BrushRadius;
			BrushMetrics.Y = InShaderParams.BrushRadialFalloffRange;
			BrushMetrics.Z = InShaderParams.BrushDepth;
			BrushMetrics.W = InShaderParams.BrushDepthFalloffRange;
			SetShaderValue( ShaderRHI, BrushMetricsParameter, BrushMetrics );

			FVector4 BrushStrength4( InShaderParams.BrushStrength, 0.0f, 0.0f, 0.0f );
			SetShaderValue( ShaderRHI, BrushStrengthParameter, BrushStrength4 );

			SetShaderValue( ShaderRHI, BrushColorParameter, InShaderParams.BrushColor );

			FVector4 ChannelFlags;
			ChannelFlags.X = InShaderParams.RedChannelFlag;
			ChannelFlags.Y = InShaderParams.GreenChannelFlag;
			ChannelFlags.Z = InShaderParams.BlueChannelFlag;
			ChannelFlags.W = InShaderParams.AlphaChannelFlag;
			SetShaderValue( ShaderRHI, ChannelFlagsParameter, ChannelFlags );
			
			float MaskVal = InShaderParams.GenerateMaskFlag ? 1.0f : 0.0f;
			SetShaderValue( ShaderRHI, GenerateMaskFlagParameter, MaskVal );

			// @todo MeshPaint
			SetShaderValue( ShaderRHI, GammaParameter, InGamma );
		}


	private:

		/** Texture that is a clone of the destination render target before we start drawing */
		FShaderResourceParameter CloneTextureParameter;
		FShaderResourceParameter CloneTextureParameterSampler;

		/** Brush -> World matrix */
		FShaderParameter WorldToBrushMatrixParameter;

		/** Brush metrics: x = radius, y = falloff range, z = depth, w = depth falloff range */
		FShaderParameter BrushMetricsParameter;

		/** Brush strength */
		FShaderParameter BrushStrengthParameter;

		/** Brush color */
		FShaderParameter BrushColorParameter;

		/** Flags that control paining individual channels: x = Red, y = Green, z = Blue, w = Alpha */
		FShaderParameter ChannelFlagsParameter;
		
		/** Flag to control brush mask generation or paint blending */
		FShaderParameter GenerateMaskFlagParameter;

		/** Gamma */
		// @todo MeshPaint: Remove this?
		FShaderParameter GammaParameter;
	};


	IMPLEMENT_SHADER_TYPE( , TMeshPaintPixelShader, TEXT( "MeshPaintPixelShader" ), TEXT( "Main" ), SF_Pixel );


	/** Mesh paint dilate vertex shader */
	class TMeshPaintDilateVertexShader : public FGlobalShader
	{
		DECLARE_SHADER_TYPE( TMeshPaintDilateVertexShader, Global );

	public:

		static bool ShouldCache( EShaderPlatform Platform )
		{
			return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3);
		}

		/** Default constructor. */
		TMeshPaintDilateVertexShader() {}

		/** Initialization constructor. */
		TMeshPaintDilateVertexShader( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
			: FGlobalShader( Initializer )
		{
			TransformParameter.Bind( Initializer.ParameterMap, TEXT( "c_Transform" ) );
		}

		virtual bool Serialize( FArchive& Ar )
		{
			bool bShaderHasOutdatedParameters = FShader::Serialize( Ar );
			Ar << TransformParameter;
			return bShaderHasOutdatedParameters;
		}

		void SetParameters( const FMatrix& InTransform )
		{
			SetShaderValue( GetVertexShader(), TransformParameter, InTransform );
		}

	private:

		FShaderParameter TransformParameter;
	};


	IMPLEMENT_SHADER_TYPE( , TMeshPaintDilateVertexShader, TEXT( "MeshPaintDilateVertexShader" ), TEXT( "Main" ), SF_Vertex );



	/** Mesh paint pixel shader */
	class TMeshPaintDilatePixelShader : public FGlobalShader
	{
		DECLARE_SHADER_TYPE( TMeshPaintDilatePixelShader, Global );

	public:

		static bool ShouldCache(EShaderPlatform Platform)
		{
			return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3);
		}

		/** Default constructor. */
		TMeshPaintDilatePixelShader() {}

		/** Initialization constructor. */
		TMeshPaintDilatePixelShader( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
			: FGlobalShader( Initializer )
		{
			Texture0Parameter.Bind( Initializer.ParameterMap, TEXT( "Texture0" ) );
			Texture0ParameterSampler.Bind( Initializer.ParameterMap, TEXT( "Texture0Sampler" ) );
			Texture1Parameter.Bind( Initializer.ParameterMap, TEXT( "Texture1") );
			Texture1ParameterSampler.Bind( Initializer.ParameterMap, TEXT( "Texture1Sampler") );
			Texture2Parameter.Bind( Initializer.ParameterMap, TEXT( "Texture2") );
			Texture2ParameterSampler.Bind( Initializer.ParameterMap, TEXT( "Texture2Sampler") );
			WidthPixelOffsetParameter.Bind( Initializer.ParameterMap, TEXT( "WidthPixelOffset") );
			HeightPixelOffsetParameter.Bind( Initializer.ParameterMap, TEXT( "HeightPixelOffset") );
			GammaParameter.Bind( Initializer.ParameterMap, TEXT( "Gamma" ) );
		}

		virtual bool Serialize(FArchive& Ar)
		{
			bool bShaderHasOutdatedParameters = FShader::Serialize(Ar);
			Ar << Texture0Parameter;
			Ar << Texture0ParameterSampler;
			Ar << Texture1Parameter;
			Ar << Texture1ParameterSampler;
			Ar << Texture2Parameter;
			Ar << Texture2ParameterSampler;
			Ar << WidthPixelOffsetParameter;
			Ar << HeightPixelOffsetParameter;
			Ar << GammaParameter;
			return bShaderHasOutdatedParameters;
		}

		void SetParameters( const float InGamma, const FMeshPaintDilateShaderParameters& InShaderParams )
		{
			const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

			SetTextureParameter(
				ShaderRHI,
				Texture0Parameter,
				Texture0ParameterSampler,
				TStaticSamplerState< SF_Point, AM_Clamp, AM_Clamp, AM_Clamp >::GetRHI(),
				InShaderParams.Texture0->GetRenderTargetResource()->TextureRHI );

			SetTextureParameter(
				ShaderRHI,
				Texture1Parameter,
				Texture1ParameterSampler,
				TStaticSamplerState< SF_Point, AM_Clamp, AM_Clamp, AM_Clamp >::GetRHI(),
				InShaderParams.Texture1->GetRenderTargetResource()->TextureRHI );

			SetTextureParameter(
				ShaderRHI,
				Texture2Parameter,
				Texture2ParameterSampler,
				TStaticSamplerState< SF_Point, AM_Clamp, AM_Clamp, AM_Clamp >::GetRHI(),
				InShaderParams.Texture2->GetRenderTargetResource()->TextureRHI );

			SetShaderValue(ShaderRHI, WidthPixelOffsetParameter, InShaderParams.WidthPixelOffset );
			SetShaderValue(ShaderRHI, HeightPixelOffsetParameter, InShaderParams.HeightPixelOffset );
			SetShaderValue(ShaderRHI, GammaParameter, InGamma );
		}


	private:

		/** Texture0 */
		FShaderResourceParameter Texture0Parameter;
		FShaderResourceParameter Texture0ParameterSampler;

		/** Texture1 */
		FShaderResourceParameter Texture1Parameter;
		FShaderResourceParameter Texture1ParameterSampler;

		/** Texture2 */
		FShaderResourceParameter Texture2Parameter;
		FShaderResourceParameter Texture2ParameterSampler;

		/** Pixel size width */
		FShaderParameter WidthPixelOffsetParameter;
		
		/** Pixel size height */
		FShaderParameter HeightPixelOffsetParameter;

		/** Gamma */
		// @todo MeshPaint: Remove this?
		FShaderParameter GammaParameter;
	};


	IMPLEMENT_SHADER_TYPE( , TMeshPaintDilatePixelShader, TEXT( "MeshPaintDilatePixelShader" ), TEXT( "Main" ), SF_Pixel );


	/** Mesh paint vertex format */
	typedef FSimpleElementVertex FMeshPaintVertex;


	/** Mesh paint vertex declaration resource */
	typedef FSimpleElementVertexDeclaration FMeshPaintVertexDeclaration;


	/** Global mesh paint vertex declaration resource */
	TGlobalResource< FMeshPaintVertexDeclaration > GMeshPaintVertexDeclaration;



	typedef FSimpleElementVertex FMeshPaintDilateVertex;
	typedef FSimpleElementVertexDeclaration FMeshPaintDilateVertexDeclaration;
	TGlobalResource< FMeshPaintDilateVertexDeclaration > GMeshPaintDilateVertexDeclaration;


	/** Binds the mesh paint vertex and pixel shaders to the graphics device */
	void SetMeshPaintShaders_RenderThread( const FMatrix& InTransform,
										   const float InGamma,
										   const FMeshPaintShaderParameters& InShaderParams )
	{
		TShaderMapRef< TMeshPaintVertexShader > VertexShader( GetGlobalShaderMap() );
		TShaderMapRef< TMeshPaintPixelShader > PixelShader( GetGlobalShaderMap() );

		static FGlobalBoundShaderState BoundShaderState;
		SetGlobalBoundShaderState( BoundShaderState, GMeshPaintVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader );

		// Set vertex shader parameters
		VertexShader->SetParameters( InTransform );
		
		// Set pixel shader parameters
		PixelShader->SetParameters( InGamma, InShaderParams );

		// @todo MeshPaint: Make sure blending/color writes are setup so we can write to ALPHA if needed!
	}

	/** Binds the mesh paint vertex and pixel shaders to the graphics device */
	void SetMeshPaintDilateShaders_RenderThread( const FMatrix& InTransform,
												 const float InGamma,
												 const FMeshPaintDilateShaderParameters& InShaderParams )
	{
		TShaderMapRef< TMeshPaintDilateVertexShader > VertexShader( GetGlobalShaderMap() );
		TShaderMapRef< TMeshPaintDilatePixelShader > PixelShader( GetGlobalShaderMap() );

		static FGlobalBoundShaderState BoundShaderState;
		SetGlobalBoundShaderState( BoundShaderState, GMeshPaintDilateVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader );

		// Set vertex shader parameters
		VertexShader->SetParameters( InTransform );

		// Set pixel shader parameters
		PixelShader->SetParameters( InGamma, InShaderParams );
	}

}
