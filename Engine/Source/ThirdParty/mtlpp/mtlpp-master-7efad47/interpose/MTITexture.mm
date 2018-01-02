// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#import <Metal/MTLTexture.h>
#include "MTITexture.hpp"


MTLPP_BEGIN

INTERPOSE_PROTOCOL_REGISTER(MTITextureTrace, id<MTLTexture>);

INTERPOSE_DEFINITION_VOID(MTITextureTrace, Rootresource, id <MTLResource>)
{
	return Original(Obj, Cmd );
}

INTERPOSE_DEFINITION_VOID(MTITextureTrace, Parenttexture, id <MTLTexture>)
{
	return Original(Obj, Cmd );
}

INTERPOSE_DEFINITION_VOID(MTITextureTrace, Parentrelativelevel, NSUInteger)
{
	return Original(Obj, Cmd );
}

INTERPOSE_DEFINITION_VOID(MTITextureTrace, Parentrelativeslice, NSUInteger)
{
	return Original(Obj, Cmd );
}

INTERPOSE_DEFINITION_VOID(MTITextureTrace, Buffer, id <MTLBuffer>)
{
	return Original(Obj, Cmd );
}

INTERPOSE_DEFINITION_VOID(MTITextureTrace, Bufferoffset, NSUInteger)
{
	return Original(Obj, Cmd );
}

INTERPOSE_DEFINITION_VOID(MTITextureTrace, Bufferbytesperrow, NSUInteger)
{
	return Original(Obj, Cmd );
}

INTERPOSE_DEFINITION_VOID(MTITextureTrace, Iosurface, IOSurfaceRef)
{
	return Original(Obj, Cmd );
}

INTERPOSE_DEFINITION_VOID(MTITextureTrace, Iosurfaceplane, NSUInteger)
{
	return Original(Obj, Cmd );
}

INTERPOSE_DEFINITION_VOID(MTITextureTrace, Texturetype, MTLTextureType)
{
	return Original(Obj, Cmd );
}

INTERPOSE_DEFINITION_VOID(MTITextureTrace, Pixelformat, MTLPixelFormat)
{
	return Original(Obj, Cmd );
}

INTERPOSE_DEFINITION_VOID(MTITextureTrace, Width, NSUInteger)
{
	return Original(Obj, Cmd );
}

INTERPOSE_DEFINITION_VOID(MTITextureTrace, Height, NSUInteger)
{
	return Original(Obj, Cmd );
}

INTERPOSE_DEFINITION_VOID(MTITextureTrace, Depth, NSUInteger)
{
	return Original(Obj, Cmd );
}

INTERPOSE_DEFINITION_VOID(MTITextureTrace, Mipmaplevelcount, NSUInteger)
{
	return Original(Obj, Cmd );
}

INTERPOSE_DEFINITION_VOID(MTITextureTrace, Samplecount, NSUInteger)
{
	return Original(Obj, Cmd );
}

INTERPOSE_DEFINITION_VOID(MTITextureTrace, Arraylength, NSUInteger)
{
	return Original(Obj, Cmd );
}

INTERPOSE_DEFINITION_VOID(MTITextureTrace, Usage, MTLTextureUsage)
{
	return Original(Obj, Cmd );
}

INTERPOSE_DEFINITION_VOID(MTITextureTrace, Isframebufferonly, BOOL)
{
	return Original(Obj, Cmd );
}

INTERPOSE_DEFINITION(MTITextureTrace, Getbytesbytesperrowbytesperimagefromregionmipmaplevelslice, void, void * pixelBytes , NSUInteger bytesPerRow , NSUInteger bytesPerImage , MTLPPRegion region , NSUInteger level , NSUInteger slice)
{
	Original(Obj, Cmd, pixelBytes, bytesPerRow, bytesPerImage, region, level, slice);
}

INTERPOSE_DEFINITION(MTITextureTrace, Replaceregionmipmaplevelslicewithbytesbytesperrowbytesperimage, void, MTLPPRegion region , NSUInteger level , NSUInteger slice , const void * pixelBytes , NSUInteger bytesPerRow , NSUInteger bytesPerImage)
{
	Original(Obj, Cmd, region, level, slice, pixelBytes, bytesPerRow, bytesPerImage);
}

INTERPOSE_DEFINITION(MTITextureTrace, Getbytesbytesperrowfromregionmipmaplevel, void, void * pixelBytes , NSUInteger bytesPerRow , MTLPPRegion region , NSUInteger level)
{
	Original(Obj, Cmd, pixelBytes, bytesPerRow, region, level);
}

INTERPOSE_DEFINITION(MTITextureTrace, Replaceregionmipmaplevelwithbytesbytesperrow, void, MTLPPRegion region , NSUInteger level , const void * pixelBytes , NSUInteger bytesPerRow)
{
	Original(Obj, Cmd, region, level, pixelBytes, bytesPerRow);
}

INTERPOSE_DEFINITION(MTITextureTrace, Newtextureviewwithpixelformat, id<MTLTexture>, MTLPixelFormat pixelFormat)
{
	return MTITextureTrace::Register(Original(Obj, Cmd, pixelFormat));
}

INTERPOSE_DEFINITION(MTITextureTrace, Newtextureviewwithpixelformattexturetypelevelsslices, id<MTLTexture>, MTLPixelFormat pixelFormat , MTLTextureType textureType , NSRange levelRange , NSRange sliceRange)
{
	return MTITextureTrace::Register(Original(Obj, Cmd, pixelFormat, textureType, levelRange, sliceRange));
}

MTLPP_END

