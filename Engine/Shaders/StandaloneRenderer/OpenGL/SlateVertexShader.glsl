// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

// #version 120 at the beginning is added in FSlateOpenGLShader::CompileShader()

uniform mat4 ViewProjectionMatrix;
uniform vec4 VertexShaderParams;

// Per vertex
attribute vec4 InTexCoords;
attribute vec4 InClipCoords;
attribute vec2 InPosition;
attribute vec4 InColor;

// Between vertex and pixel shader
varying vec4 TexCoords;
varying vec2 SSPosition;
varying vec4 ClipCoords;
varying vec4 Color;

/**
 * Rotates a point around another point 
 *
 * @param InPoint	The point to rotate
 * @param AboutPoint	The point to rotate about
 * @param Radians	The angle of rotation in radians
 * @return The rotated point
 */
vec2 RotatePoint( vec2 InPoint, vec2 AboutPoint, float Radians )
{
	if( Radians != 0.0 )
	{
		float CosAngle = cos(Radians);
		float SinAngle = sin(Radians);

		InPoint.x -= AboutPoint.x;
		InPoint.y -= AboutPoint.y;

		float X = (InPoint.x * CosAngle) - (InPoint.y * SinAngle);
		float Y = (InPoint.x * SinAngle) + (InPoint.y * CosAngle);

		return vec2( X + AboutPoint.x, Y + AboutPoint.y );
	}

	return InPoint;
}

void main()
{
	vec2 RotatedPosition = RotatePoint( InPosition, VertexShaderParams.yz, VertexShaderParams.x );

	SSPosition = RotatedPosition;
	ClipCoords = InClipCoords;
	TexCoords = InTexCoords;
	Color = InColor;

	gl_Position = ViewProjectionMatrix * vec4( RotatedPosition.xy, 0, 1 );
}
