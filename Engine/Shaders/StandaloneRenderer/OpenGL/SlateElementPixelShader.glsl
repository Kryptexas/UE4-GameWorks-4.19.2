// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

// handle differences between ES2 and full GL shaders
#if PLATFORM_USES_ES2
precision highp float;
#else
// #version 120 at the beginning is added in FSlateOpenGLShader::CompileShader()
#extension GL_EXT_gpu_shader4 : enable
#endif

// Shader types
#define ST_Default	0
#define ST_Border	1
#define ST_Font		2
#define ST_Line		3

// Draw effects
uniform bool EffectsDisabled;
uniform bool IgnoreTextureAlpha;

uniform vec4 MarginUVs;
uniform int ShaderType;
uniform sampler2D ElementTexture;

varying vec4 TexCoords;
varying vec2 SSPosition;
varying vec4 ClipCoords;
varying vec4 Color;

vec4 GetFontElementColor()
{
	vec4 OutColor = Color;
	OutColor.a *= texture2D(ElementTexture, TexCoords.xy).a;

	return OutColor;
}

vec4 GetDefaultElementColor()
{
	vec4 OutColor = Color;

    vec4 TextureColor = texture2D(ElementTexture, TexCoords.xy*TexCoords.zw);
	if( IgnoreTextureAlpha )
    {
        TextureColor.a = 1.0;
    }
	OutColor *= TextureColor;

	return OutColor;
}

vec4 GetBorderElementColor()
{
	vec4 OutColor = Color;
	vec4 InTexCoords = TexCoords;
	vec2 NewUV;
	if( InTexCoords.z == 0.0 && InTexCoords.w == 0.0 )
	{
		NewUV = InTexCoords.xy;
	}
	else
	{
		vec2 MinUV;
		vec2 MaxUV;
	
		if( InTexCoords.z > 0.0 )
		{
			MinUV = vec2(MarginUVs.x,0.0);
			MaxUV = vec2(MarginUVs.y,1.0);
			InTexCoords.w = 1.0;
		}
		else
		{
			MinUV = vec2(0.0,MarginUVs.z);
			MaxUV = vec2(1.0,MarginUVs.w);
			InTexCoords.z = 1.0;
		}

		NewUV = InTexCoords.xy*InTexCoords.zw;
		NewUV = fract(NewUV);
		NewUV = mix(MinUV,MaxUV,NewUV);	

	}

	vec4 TextureColor = texture2D(ElementTexture, NewUV);
	if( IgnoreTextureAlpha )
	{
		TextureColor.a = 1.0;
	}
		
	OutColor *= TextureColor;

	return OutColor;
}

vec4 GetSplineElementColor()
{
	vec4 InTexCoords = TexCoords;
	float Width = MarginUVs.x;
	float Radius = MarginUVs.y;
	
	vec2 StartPos = InTexCoords.xy;
	vec2 EndPos = InTexCoords.zw;
	
	vec2 Diff = vec2( StartPos.y - EndPos.y, EndPos.x - StartPos.x ) ;
	
	float K = 2.0/( (2.0*Radius + Width)*sqrt( dot( Diff, Diff) ) );
	
	vec3 E0 = K*vec3( Diff.x, Diff.y, (StartPos.x*EndPos.y - EndPos.x*StartPos.y) );
	E0.z += 1.0;
	
	vec3 E1 = K*vec3( -Diff.x, -Diff.y, (EndPos.x*StartPos.y - StartPos.x*EndPos.y) );
	E1.z += 1.0;
	
	vec3 Pos = vec3(SSPosition.xy,1.0);
	
	vec2 Distance = vec2( dot(E0,Pos), dot(E1,Pos) );
	
	if( any( lessThan(Distance, vec2(0.0)) ) )
	{
		// using discard instead of clip because
		// apparently clipped pixels are written into the stencil buffer but discards are not
		discard;
	}
	
	
	vec4 InColor = Color;
	
	float Index = min(Distance.x,Distance.y);
	
	// Without this, the texture sample sometimes samples the next entry in the table.  Usually occurs when sampling the last entry in the table but instead
	// samples the first and we get white pixels
	const float HalfPixelOffset = 1.0/32.0;
	
	InColor.a *= texture2D(ElementTexture, vec2(Index-HalfPixelOffset,0.0)).x;
	
	if( InColor.a < 0.05 )
	{
		discard;
	}
	
	return InColor;
}

void main()
{
	// Clip pixels which are outside of the clipping rect
	vec4 Clip = vec4( SSPosition.x - ClipCoords.x, ClipCoords.z - SSPosition.x, SSPosition.y - ClipCoords.y, ClipCoords.w - SSPosition.y );
	if( any( lessThan(Clip, vec4(0,0,0,0) ) ) )
	{
		discard;
	}

	vec4 OutColor;
	if( ShaderType == ST_Default )
	{
		OutColor = GetDefaultElementColor();
	}
	else if( ShaderType == ST_Border )
	{
		OutColor = GetBorderElementColor();
	}
	else if( ShaderType == ST_Font )
	{
		OutColor = GetFontElementColor();
	}
	else
	{
		OutColor = GetSplineElementColor();
	}
	
	// gamma correct
	#if PLATFORM_USES_ES2
		OutColor.rgb = sqrt( OutColor.rgb );
	#else
		OutColor.rgb = pow(OutColor.rgb, vec3(1.0/2.2));
	#endif

	if( EffectsDisabled )
	{
		//desaturate
		vec3 LumCoeffs = vec3( 0.3, 0.59, .11 );
		float Lum = dot( LumCoeffs, OutColor.rgb );
		OutColor.rgb = mix( OutColor.rgb, vec3(Lum,Lum,Lum), .8 );
	
		vec3 Grayish = vec3(0.4, 0.4, 0.4);
	
		OutColor.rgb = mix( OutColor.rgb, Grayish, clamp( distance( OutColor.rgb, Grayish ), 0.0, 0.8)  );
	}

	gl_FragColor = OutColor.bgra;
}
