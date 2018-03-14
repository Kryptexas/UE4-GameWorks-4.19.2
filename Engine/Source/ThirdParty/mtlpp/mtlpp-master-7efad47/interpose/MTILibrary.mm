// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#import <Metal/MTLLibrary.h>
#include "MTILibrary.hpp"
#include "MTIArgumentEncoder.hpp"

MTLPP_BEGIN

INTERPOSE_PROTOCOL_REGISTER(MTIFunctionTrace, id<MTLFunction>);

void MTIFunctionTrace::SetLabelImpl(id Obj, SEL Cmd, Super::SetLabelType::DefinedIMP Original, NSString* Label)
{
	Original(Obj, Cmd, Label);
}

id <MTLArgumentEncoder> MTIFunctionTrace::NewArgumentEncoderWithBufferIndexImpl(id Obj, SEL Cmd, Super::NewArgumentEncoderWithBufferIndexType::DefinedIMP Original, NSUInteger idx)
{
	return MTIArgumentEncoderTrace::Register(Original(Obj, Cmd, idx));
}

id <MTLArgumentEncoder> MTIFunctionTrace::NewArgumentEncoderWithBufferIndexreflectionImpl(id Obj, SEL Cmd, Super::NewArgumentEncoderWithBufferIndexreflectionType::DefinedIMP Original, NSUInteger idx, MTLAutoreleasedArgument* reflection)
{
	return MTIArgumentEncoderTrace::Register(Original(Obj, Cmd, idx, reflection));
}


INTERPOSE_PROTOCOL_REGISTER(MTILibraryTrace, id<MTLLibrary>);

void MTILibraryTrace::SetLabelImpl(id Obj, SEL Cmd, Super::SetLabelType::DefinedIMP Original, NSString* Label)
{
	Original(Obj, Cmd, Label);
}

id <MTLFunction> MTILibraryTrace::NewFunctionWithNameImpl(id Obj, SEL Cmd, Super::NewFunctionWithNameType::DefinedIMP Original, NSString * functionName)
{
	return MTIFunctionTrace::Register(Original(Obj, Cmd, functionName));
}

id <MTLFunction> MTILibraryTrace::NewFunctionWithNameconstantValueserrorImpl(id Obj, SEL Cmd, Super::NewFunctionWithNameconstantValueserrorType::DefinedIMP Original, NSString * name, MTLFunctionConstantValues * constantValues, NSError ** error)
{
	return MTIFunctionTrace::Register(Original(Obj, Cmd, name, constantValues, error));
}

void MTILibraryTrace::NewFunctionWithNameconstantValuescompletionHandlerImpl(id Obj, SEL Cmd, Super::NewFunctionWithNameconstantValuescompletionHandlerType::DefinedIMP Original, NSString * name, MTLFunctionConstantValues * constantValues, void (^Handler)(id<MTLFunction> __nullable function, NSError* error))
{
	Original(Obj, Cmd, name, constantValues, ^(id<MTLFunction> __nullable function, NSError* error){ Handler(MTIFunctionTrace::Register(function), error); });
}

MTLPP_END
