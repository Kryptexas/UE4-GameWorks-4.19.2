// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.



cbuffer PerElementVSConstants
{
	matrix WorldViewProjection;
	float4 VertexShaderParams;
}

struct VertexOut
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR0;
	float4 TextureCoordinates : TEXCOORD0;
	float4 ClipCoords : TEXCOORD1;
	float2 WorldPosition : TEXCOORD2;
};



/**
 * Rotates a point around another point 
 *
 * @param InPoint	The point to rotate
 * @param AboutPoint	The point to rotate about
 * @param Radians	The angle of rotation in radians
 * @return The rotated point
 */
float2 RotatePoint( float2 InPoint, float2 AboutPoint, float Radians )
{
	if( Radians != 0.0f )
	{
		float CosAngle = cos(Radians);
		float SinAngle = sin(Radians);

		InPoint.x -= AboutPoint.x;
		InPoint.y -= AboutPoint.y;

		float X = (InPoint.x * CosAngle) - (InPoint.y * SinAngle);
		float Y = (InPoint.x * SinAngle) + (InPoint.y * CosAngle);

		return float2( X + AboutPoint.x, Y + AboutPoint.y );
	}

	return InPoint;
}


VertexOut Main(
	in int2 InPosition : POSITION,
	in float4 InTextureCoordinates : TEXCOORD0,
	in uint4 InClipCoords : TEXCOORD1,
	in float4 InColor : COLOR0
	) 
{
	VertexOut Out;
	float2 RotatedPoint = RotatePoint( InPosition.xy, VertexShaderParams.yz, VertexShaderParams.x );
	Out.WorldPosition = RotatedPoint;

	Out.Position = mul( WorldViewProjection,float4( RotatedPoint, 0, 1 ) );

	Out.ClipCoords = InClipCoords;
	Out.TextureCoordinates = InTextureCoordinates;
	
	// Swap r and b for d3d 11
	Out.Color = InColor;

	return Out;
}
