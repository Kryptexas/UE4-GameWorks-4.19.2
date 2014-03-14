//-----------------------------------------------------------------------------
// File:		LightPropagationVolumeVisualisation.cpp
//
// Summary:		Light Propagation Volume visualisation support 
//
// Created:		2013-03-01
//
// Author:		mailto:benwood@microsoft.com
//
//				Copyright (C) Microsoft. All rights reserved.
//-----------------------------------------------------------------------------

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "LightPropagationVolume.h"

// ----------------------------------------------------------------------------

static FGlobalBoundShaderState LpvVisBoundShaderState;

// ----------------------------------------------------------------------------

class FLpvVisualiseBase : public FGlobalShader
{
public:
	// Default constructor
	FLpvVisualiseBase()	{	}

	// Initialization constructor 
	explicit FLpvVisualiseBase( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
		: FGlobalShader(Initializer)
	{
	}

	static void ModifyCompilationEnvironment( EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment )				
	{ 
		OutEnvironment.SetDefine( TEXT("LPV_MULTIPLE_BOUNCES"), (uint32)LPV_MULTIPLE_BOUNCES );
		OutEnvironment.SetDefine( TEXT("LPV_GV_VOLUME_TEXTURE"),	(uint32)LPV_GV_VOLUME_TEXTURE );

		FGlobalShader::ModifyCompilationEnvironment( Platform, OutEnvironment ); 
	}
};


class FLpvVisualiseGS : public FLpvVisualiseBase
{
public:
	DECLARE_SHADER_TYPE(FLpvVisualiseGS,Global);

	FLpvVisualiseGS()																												{}
	explicit FLpvVisualiseGS( const ShaderMetaType::CompiledShaderInitializerType& Initializer ) : FLpvVisualiseBase(Initializer)	{}
	virtual bool Serialize( FArchive& Ar ) OVERRIDE																					{ return FLpvVisualiseBase::Serialize( Ar ); }

	//@todo-rco: Remove this when reenabling for OpenGL
	static bool ShouldCache( EShaderPlatform Platform )		{ return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && !IsOpenGLPlatform(Platform); }
	static void ModifyCompilationEnvironment( EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment )				{ FLpvVisualiseBase::ModifyCompilationEnvironment( Platform, OutEnvironment ); }

	void SetParameters(
		const FSceneView& View )
	{
		FGeometryShaderRHIParamRef ShaderRHI = GetGeometryShader();
		FGlobalShader::SetParameters(ShaderRHI, View); 
	}
};

class FLpvVisualiseVS : public FLpvVisualiseBase
{
public:
	DECLARE_SHADER_TYPE(FLpvVisualiseVS,Global);

	FLpvVisualiseVS()	{	}
	explicit FLpvVisualiseVS( const ShaderMetaType::CompiledShaderInitializerType& Initializer ) : FLpvVisualiseBase(Initializer) {}
	virtual bool Serialize( FArchive& Ar ) OVERRIDE																					{ return FLpvVisualiseBase::Serialize( Ar ); }

	//@todo-rco: Remove this when reenabling for OpenGL
	static bool ShouldCache( EShaderPlatform Platform )		{ return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && !IsOpenGLPlatform(Platform); }
	static void ModifyCompilationEnvironment( EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment )				{ FLpvVisualiseBase::ModifyCompilationEnvironment( Platform, OutEnvironment ); }

	void SetParameters(
		const FSceneView& View )
	{
		FVertexShaderRHIParamRef ShaderRHI = GetVertexShader();
		FGlobalShader::SetParameters(ShaderRHI, View); 
	}
};

class FLpvVisualisePS : public FLpvVisualiseBase
{
public:
	DECLARE_SHADER_TYPE(FLpvVisualisePS,Global);

	FLpvVisualisePS()	{	}
	explicit FLpvVisualisePS( const ShaderMetaType::CompiledShaderInitializerType& Initializer ) : FLpvVisualiseBase(Initializer) 
	{
#if LPV_VOLUME_TEXTURE
		for ( int i=0; i<7; i++ )
		{
			LpvBufferSRVParameters[i].Bind( Initializer.ParameterMap, LpvVolumeTextureSRVNames[i] );
		}
#else
		InLpvBuffer.Bind( Initializer.ParameterMap, TEXT("gLpvBuffer") );
#endif

#if LPV_VOLUME_TEXTURE || LPV_GV_VOLUME_TEXTURE
		LpvVolumeTextureSampler.Bind(Initializer.ParameterMap, TEXT("gLpv3DTextureSampler"));
#endif

#if LPV_GV_VOLUME_TEXTURE
		for ( int i=0; i<3; i++ )
		{
			GvBufferSRVParameters[i].Bind( Initializer.ParameterMap, LpvGvVolumeTextureSRVNames[i] );
		}
#else
		InGvBuffer.Bind(			Initializer.ParameterMap, TEXT("gGvBuffer") );
#endif
	}

	//@todo-rco: Remove this when reenabling for OpenGL
	static bool ShouldCache( EShaderPlatform Platform )		{ return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) && !IsOpenGLPlatform(Platform); }
	static void ModifyCompilationEnvironment( EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment )				{ FLpvVisualiseBase::ModifyCompilationEnvironment( Platform, OutEnvironment ); }

	void SetParameters(
		const FLightPropagationVolume* LPV,
		const FSceneView& View )
	{
		FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();
		FGlobalShader::SetParameters(ShaderRHI, View); 

#if LPV_VOLUME_TEXTURE
		for ( int i=0; i<7; i++ )
		{
			FTextureRHIParamRef LpvBufferSrv = LPV->LpvVolumeTextures[ 1-LPV->mWriteBufferIndex ][i]->GetRenderTargetItem().ShaderResourceTexture;
			if ( LpvBufferSRVParameters[i].IsBound() )
			{
				RHISetShaderTexture( ShaderRHI, LpvBufferSRVParameters[i].GetBaseIndex(), LpvBufferSrv );
			}
			SetTextureParameter( ShaderRHI, LpvBufferSRVParameters[i], LpvVolumeTextureSampler, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(), LpvBufferSrv );
		}
#else
		if ( InLpvBuffer.IsBound() ) 
		{
			RHISetShaderResourceViewParameter( ShaderRHI, InLpvBuffer.GetBaseIndex(), LPV->mLpvBuffers[ LPV->mWriteBufferIndex ]->SRV ); 
		}
#endif

#if LPV_GV_VOLUME_TEXTURE
		for ( int i=0; i<3; i++ )
		{
			FTextureRHIParamRef GvBufferSrv = LPV->GvVolumeTextures[i]->GetRenderTargetItem().ShaderResourceTexture;
			if ( GvBufferSRVParameters[i].IsBound() )
			{
				RHISetShaderTexture( ShaderRHI, GvBufferSRVParameters[i].GetBaseIndex(), GvBufferSrv );
			}
			SetTextureParameter( ShaderRHI, GvBufferSRVParameters[i], LpvVolumeTextureSampler, TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(), GvBufferSrv );
		}

#else
		if ( InGvBuffer.IsBound() ) 
		{
			RHISetShaderResourceViewParameter( ShaderRHI, InGvBuffer.GetBaseIndex(), LPV->GvBuffer->SRV ); 
		}
#endif
	}


	// Serialization
	virtual bool Serialize( FArchive& Ar ) OVERRIDE
	{
		bool bShaderHasOutdatedParameters = FLpvVisualiseBase::Serialize( Ar );

#if LPV_VOLUME_TEXTURE
		for ( int i=0; i<7; i++ )
		{
			Ar << LpvBufferSRVParameters[i];
		}
#else
		Ar << InLpvBuffer;
#endif

#if LPV_VOLUME_TEXTURE | LPV_GV_VOLUME_TEXTURE
		Ar << LpvVolumeTextureSampler;
#endif

#if LPV_GV_VOLUME_TEXTURE
		for ( int i=0; i<3; i++ )
		{
			Ar << GvBufferSRVParameters[i];
		}
#else
		Ar << InGvBuffer;
#endif
		return bShaderHasOutdatedParameters;
	}
#if LPV_VOLUME_TEXTURE
	FShaderResourceParameter LpvBufferSRVParameters[7];
#else
	FShaderResourceParameter InLpvBuffer;
#endif 

#if LPV_VOLUME_TEXTURE | LPV_GV_VOLUME_TEXTURE
	FShaderResourceParameter LpvVolumeTextureSampler;
#endif

#if LPV_GV_VOLUME_TEXTURE
	FShaderResourceParameter GvBufferSRVParameters[3];
#else
	FShaderResourceParameter InGvBuffer;
#endif

	void UnbindBuffers()
	{
		// TODO: Is this necessary here?
		FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();
#if LPV_VOLUME_TEXTURE
		for ( int i=0;i<7; i++ )
		{
			if ( LpvBufferSRVParameters[i].IsBound() )
			{
				RHISetShaderTexture( ShaderRHI, LpvBufferSRVParameters[i].GetBaseIndex(), FTextureRHIParamRef() );
			}
		}
#else
		if ( InLpvBuffer.IsBound() ) RHISetShaderResourceViewParameter( ShaderRHI, InLpvBuffer.GetBaseIndex(), FShaderResourceViewRHIParamRef() );
#endif

#if LPV_GV_VOLUME_TEXTURE
		for ( int i=0;i<3; i++ )
		{
			if ( GvBufferSRVParameters[i].IsBound() )
			{
				RHISetShaderTexture( ShaderRHI, GvBufferSRVParameters[i].GetBaseIndex(), FTextureRHIParamRef() );
			}
		}
#else
		if ( InGvBuffer.IsBound() ) RHISetShaderResourceViewParameter( ShaderRHI, InGvBuffer.GetBaseIndex(), FShaderResourceViewRHIParamRef() );
#endif
	}

};


IMPLEMENT_SHADER_TYPE(,FLpvVisualiseGS,TEXT("LPVVisualise"),TEXT("GShader"),SF_Geometry);
IMPLEMENT_SHADER_TYPE(,FLpvVisualiseVS,TEXT("LPVVisualise"),TEXT("VShader"),SF_Vertex);
IMPLEMENT_SHADER_TYPE(,FLpvVisualisePS,TEXT("LPVVisualise"),TEXT("PShader"),SF_Pixel);


void FLightPropagationVolume::Visualise( const FSceneView& View ) const
{
	SCOPED_DRAW_EVENT(LpvVisualise, DEC_LIGHT);
	check(GRHIFeatureLevel == ERHIFeatureLevel::SM5);

	TShaderMapRef<FLpvVisualiseVS> VertexShader(GetGlobalShaderMap());
	TShaderMapRef<FLpvVisualiseGS> GeometryShader(GetGlobalShaderMap());
	TShaderMapRef<FLpvVisualisePS> PixelShader(GetGlobalShaderMap());

	RHISetDepthStencilState( TStaticDepthStencilState<false,CF_Always>::GetRHI() );
	RHISetRasterizerState( TStaticRasterizerState<FM_Solid,CM_None>::GetRHI() );
	RHISetBlendState(TStaticBlendState<CW_RGB,BO_Add,BF_One,BF_One>::GetRHI());

	SetGlobalBoundShaderState(LpvVisBoundShaderState, GSimpleElementVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, *GeometryShader);

	VertexShader->SetParameters( View );
	GeometryShader->SetParameters( View );
	PixelShader->SetParameters( this, View );

	RHISetStreamSource(0, NULL, 0, 0);
	RHIDrawPrimitive( PT_PointList, 0, 1, 32*3 );

	PixelShader->UnbindBuffers();
}
