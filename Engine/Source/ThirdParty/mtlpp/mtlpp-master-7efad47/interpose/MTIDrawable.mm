// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#import <Metal/MTLDrawable.h>
#include "MTIDrawable.hpp"
#include "MTITexture.hpp"

MTLPP_BEGIN

INTERPOSE_PROTOCOL_REGISTER(MTIDrawableTrace, id<MTLDrawable>);

id <MTLTexture> MTIDrawableTrace::TextureImpl(id Obj, SEL Cmd, Super::TextureType::DefinedIMP Original)
{
	id <MTLTexture> Tex = Original(Obj, Cmd);
	return MTITextureTrace::Register(Tex);
}

CAMetalLayer* MTIDrawableTrace::LayerImpl(id Obj, SEL Cmd, Super::LayerType::DefinedIMP Original)
{
	return Original(Obj, Cmd);
}

void MTIDrawableTrace::PresentImpl(id Obj, SEL Cmd, Super::PresentType::DefinedIMP Original)
{
	Original(Obj, Cmd);
}
void MTIDrawableTrace::PresentAtTimeImpl(id Obj, SEL Cmd, Super::PresentAtTimeType::DefinedIMP Original, CFTimeInterval Time)
{
	Original(Obj, Cmd, Time);
}

INTERPOSE_DEFINITION(MTIDrawableTrace, PresentAfterMinimumDuration, void, CFTimeInterval I)
{
	Original(Obj, Cmd, I);
}

INTERPOSE_DEFINITION(MTIDrawableTrace, AddPresentedHandler, void, MTLDrawablePresentedHandler B)
{
	Original(Obj, Cmd, B);
}

INTERPOSE_DEFINITION(MTILayerTrace, Setdevice, void, id <MTLDevice> Val)
{
	Original(Obj, Cmd, Val);
}
INTERPOSE_DEFINITION(MTILayerTrace, SetpixelFormat, void, MTLPixelFormat Val)
{
	Original(Obj, Cmd, Val);
}

INTERPOSE_DEFINITION(MTILayerTrace, SetframebufferOnly, void, BOOL Val)
{
	Original(Obj, Cmd, Val);
}

INTERPOSE_DEFINITION(MTILayerTrace, SetddrawableSize, void, CGSize Val)
{
	Original(Obj, Cmd, Val);
}

INTERPOSE_DEFINITION_VOID(MTILayerTrace, NextDrawable, id <CAMetalDrawable>)
{
	return (id<CAMetalDrawable>)MTIDrawableTrace::Register(Original(Obj, Cmd));
}

INTERPOSE_DEFINITION(MTILayerTrace, SetmaximumDrawableCount, void, NSUInteger Val)
{
	Original(Obj, Cmd, Val);
}

INTERPOSE_DEFINITION(MTILayerTrace, SetpresentsWithTransaction, void, BOOL Val)
{
	Original(Obj, Cmd, Val);
}

INTERPOSE_DEFINITION(MTILayerTrace, Setcolorspace, void, CGColorSpaceRef Val)
{
	Original(Obj, Cmd, Val);
}

INTERPOSE_DEFINITION(MTILayerTrace, SetwantsExtendedDynamicRangeContent, void, BOOL Val)
{
	Original(Obj, Cmd, Val);
}

INTERPOSE_DEFINITION(MTILayerTrace, SetdisplaySyncEnabled, void, BOOL Val)
{
	Original(Obj, Cmd, Val);
}

INTERPOSE_DEFINITION(MTILayerTrace, SetallowsNextDrawableTimeout, void, BOOL Val)
{
	Original(Obj, Cmd, Val);
}

INTERPOSE_CLASS_REGISTER(MTILayerTrace, CAMetalLayer);


MTLPP_END
