// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "SlateRHIRendererPrivatePCH.h"
#include "SlateRHIRenderingPolicy.h"
#include "SlateShaders.h"

/** Flag to determine if we are running with a color vision deficiency shader on */
uint32 GSlateShaderColorVisionDeficiencyType = 0;


IMPLEMENT_SHADER_TYPE(, FSlateElementVS, TEXT("SlateVertexShader"),TEXT("Main"),SF_Vertex);

#define IMPLEMENT_SLATE_PIXELSHADER_TYPE(ShaderType, bDrawDisabledEffect, bUseTextureAlpha) \
	typedef TSlateElementPS<ESlateShader::ShaderType,bDrawDisabledEffect,bUseTextureAlpha> TSlateElementPS##ShaderType##bDrawDisabledEffect##bUseTextureAlpha; \
	IMPLEMENT_SHADER_TYPE(template<>,TSlateElementPS##ShaderType##bDrawDisabledEffect##bUseTextureAlpha,TEXT("SlateElementPixelShader"),TEXT("Main"),SF_Pixel);

/**
 * All the different permutations of shaders used by slate. Uses #defines to avoid dynamic branches                 
 */
IMPLEMENT_SLATE_PIXELSHADER_TYPE(Default,		false, true);
IMPLEMENT_SLATE_PIXELSHADER_TYPE(Border,		false, true);
IMPLEMENT_SLATE_PIXELSHADER_TYPE(Default,		true, true);
IMPLEMENT_SLATE_PIXELSHADER_TYPE(Border,		true, true);
IMPLEMENT_SLATE_PIXELSHADER_TYPE(Default,		false, false);
IMPLEMENT_SLATE_PIXELSHADER_TYPE(Border,		false, false);
IMPLEMENT_SLATE_PIXELSHADER_TYPE(Default,		true, false);
IMPLEMENT_SLATE_PIXELSHADER_TYPE(Border,		true, false);

IMPLEMENT_SLATE_PIXELSHADER_TYPE(Font,			false, true);
IMPLEMENT_SLATE_PIXELSHADER_TYPE(LineSegment,	false, true);
IMPLEMENT_SLATE_PIXELSHADER_TYPE(Font,			true, true);
IMPLEMENT_SLATE_PIXELSHADER_TYPE(LineSegment,	true, true);

/** The Slate vertex declaration. */
TGlobalResource<FSlateVertexDeclaration> GSlateVertexDeclaration;


/************************************************************************/
/* FSlateVertexDeclaration                                              */
/************************************************************************/
void FSlateVertexDeclaration::InitRHI()
{
	FVertexDeclarationElementList Elements;
	Elements.Add(FVertexElement(0,STRUCT_OFFSET(FSlateVertex,TexCoords),VET_Float4,0));
	Elements.Add(FVertexElement(0,STRUCT_OFFSET(FSlateVertex,ClipCoords),VET_Short4,1));
	Elements.Add(FVertexElement(0,STRUCT_OFFSET(FSlateVertex,Position),VET_Short2,2));
	Elements.Add(FVertexElement(0,STRUCT_OFFSET(FSlateVertex,Color),VET_Color,3));

	VertexDeclarationRHI = RHICreateVertexDeclaration(Elements);
}

void FSlateVertexDeclaration::ReleaseRHI()
{
	VertexDeclarationRHI.SafeRelease();
}

/************************************************************************/
/* FSlateDefaultVertexShader                                            */
/************************************************************************/

FSlateElementVS::FSlateElementVS( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
	: FGlobalShader(Initializer)
{
	ViewProjection.Bind(Initializer.ParameterMap, TEXT("ViewProjection"));
	VertexShaderParams.Bind( Initializer.ParameterMap, TEXT("VertexShaderParams"));
}

void FSlateElementVS::SetViewProjection( const FMatrix& InViewProjection )
{
	SetShaderValue( GetVertexShader(), ViewProjection, InViewProjection );
}

void FSlateElementVS::SetShaderParameters( const FVector4& ShaderParams )
{
	SetShaderValue( GetVertexShader(), VertexShaderParams, ShaderParams );
}

/** Serializes the shader data */
bool FSlateElementVS::Serialize( FArchive& Ar )
{
	bool bShaderHasOutdatedParameters = FGlobalShader::Serialize( Ar );

	Ar << ViewProjection;
	Ar << VertexShaderParams;

	return bShaderHasOutdatedParameters;
}



