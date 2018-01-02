/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "texture.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    class StructType;
    class ArrayType;

    enum class DataType
    {
        None     = 0,

        Struct   = 1,
        Array    = 2,

        Float    = 3,
        Float2   = 4,
        Float3   = 5,
        Float4   = 6,

        Float2x2 = 7,
        Float2x3 = 8,
        Float2x4 = 9,

        Float3x2 = 10,
        Float3x3 = 11,
        Float3x4 = 12,

        Float4x2 = 13,
        Float4x3 = 14,
        Float4x4 = 15,

        Half     = 16,
        Half2    = 17,
        Half3    = 18,
        Half4    = 19,

        Half2x2  = 20,
        Half2x3  = 21,
        Half2x4  = 22,

        Half3x2  = 23,
        Half3x3  = 24,
        Half3x4  = 25,

        Half4x2  = 26,
        Half4x3  = 27,
        Half4x4  = 28,

        Int      = 29,
        Int2     = 30,
        Int3     = 31,
        Int4     = 32,

        UInt     = 33,
        UInt2    = 34,
        UInt3    = 35,
        UInt4    = 36,

        Short    = 37,
        Short2   = 38,
        Short3   = 39,
        Short4   = 40,

        UShort   = 41,
        UShort2  = 42,
        UShort3  = 43,
        UShort4  = 44,

        Char     = 45,
        Char2    = 46,
        Char3    = 47,
        Char4    = 48,

        UChar    = 49,
        UChar2   = 50,
        UChar3   = 51,
        UChar4   = 52,

        Bool     = 53,
        Bool2    = 54,
        Bool3    = 55,
        Bool4    = 56,
		
		Texture MTLPP_AVAILABLE(10_13, 11_0) = 58,
		Sampler MTLPP_AVAILABLE(10_13, 11_0) = 59,
		Pointer MTLPP_AVAILABLE(10_13, 11_0) = 60,
		
		R8Unorm         MTLPP_AVAILABLE_IOS(11_0) = 62,
		R8Snorm         MTLPP_AVAILABLE_IOS(11_0) = 63,
		R16Unorm        MTLPP_AVAILABLE_IOS(11_0) = 64,
		R16Snorm        MTLPP_AVAILABLE_IOS(11_0) = 65,
		RG8Unorm        MTLPP_AVAILABLE_IOS(11_0) = 66,
		RG8Snorm        MTLPP_AVAILABLE_IOS(11_0) = 67,
		RG16Unorm       MTLPP_AVAILABLE_IOS(11_0) = 68,
		RG16Snorm       MTLPP_AVAILABLE_IOS(11_0) = 69,
		RGBA8Unorm      MTLPP_AVAILABLE_IOS(11_0) = 70,
		RGBA8Unorm_sRGB MTLPP_AVAILABLE_IOS(11_0) = 71,
		RGBA8Snorm      MTLPP_AVAILABLE_IOS(11_0) = 72,
		RGBA16Unorm     MTLPP_AVAILABLE_IOS(11_0) = 73,
		RGBA16Snorm     MTLPP_AVAILABLE_IOS(11_0) = 74,
		RGB10A2Unorm    MTLPP_AVAILABLE_IOS(11_0) = 75,
		RG11B10Float    MTLPP_AVAILABLE_IOS(11_0) = 76,
		RGB9E5Float     MTLPP_AVAILABLE_IOS(11_0) = 77,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class ArgumentType
    {
        Buffer            = 0,
        ThreadgroupMemory = 1,
        Texture           = 2,
        Sampler           = 3,
		
		ImageblockData MTLPP_AVAILABLE_IOS(11_0)     = 16,
		Imageblock MTLPP_AVAILABLE_IOS(11_0)         = 17,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class ArgumentAccess
    {
        ReadOnly  = 0,
        ReadWrite = 1,
        WriteOnly = 2,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

	class Type : public ns::Object<MTLType*>
	{
	public:
		Type();
		Type(MTLType* handle) : ns::Object<MTLType*>(handle) { }
		
		DataType   GetDataType() const;
	}
	MTLPP_AVAILABLE(10_13, 11_0);
	
	class TextureReferenceType : public ns::Object<MTLTextureReferenceType*>
	{
	public:
		TextureReferenceType();
		TextureReferenceType(MTLTextureReferenceType* handle) : ns::Object<MTLTextureReferenceType*>(handle) { }
		
		DataType   GetTextureDataType() const;
		TextureType   GetTextureType() const;
		ArgumentAccess GetAccess() const;
		bool IsDepthTexture() const;
	}
	MTLPP_AVAILABLE(10_13, 11_0);
	
	class PointerType : public ns::Object<MTLPointerType*>
	{
	public:
		PointerType();
		PointerType(MTLPointerType* handle) : ns::Object<MTLPointerType*>(handle) { }
		
		DataType   GetElementType() const;
		ArgumentAccess GetAccess() const;
		NSUInteger GetAlignment() const;
		NSUInteger GetDataSize() const;
		bool GetElementIsArgumentBuffer() const;
		
		StructType GetElementStructType();
		ArrayType GetElementArrayType();
	}
	MTLPP_AVAILABLE(10_13, 11_0);
	
    class StructMember : public ns::Object<MTLStructMember*>
    {
    public:
        StructMember();
        StructMember(MTLStructMember* handle) : ns::Object<MTLStructMember*>(handle) { }

        ns::String GetName() const;
        NSUInteger   GetOffset() const;
        DataType   GetDataType() const;

        StructType GetStructType() const;
        ArrayType  GetArrayType() const;
		
		TextureReferenceType GetTextureReferenceType() const MTLPP_AVAILABLE(10_13, 11_0);
		PointerType GetPointerType() const MTLPP_AVAILABLE(10_13, 11_0);
		uint64_t GetArgumentIndex() const MTLPP_AVAILABLE(10_13, 11_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class StructType : public ns::Object<MTLStructType*>
    {
    public:
        StructType();
        StructType(MTLStructType* handle) : ns::Object<MTLStructType*>(handle) { }

        const ns::Array<StructMember> GetMembers() const;
        StructMember                  GetMember(const ns::String& name) const;
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class ArrayType : public ns::Object<MTLArrayType*>
    {
    public:
        ArrayType();
        ArrayType(MTLArrayType* handle) : ns::Object<MTLArrayType*>(handle) { }

        NSUInteger   GetArrayLength() const;
        DataType   GetElementType() const;
        NSUInteger   GetStride() const;
        StructType GetElementStructType() const;
        ArrayType  GetElementArrayType() const;
		
		uint64_t GetArgumentIndexStride() const MTLPP_AVAILABLE(10_13, 11_0);
		TextureReferenceType GetElementTextureReferenceType() const MTLPP_AVAILABLE(10_13, 11_0);
		PointerType GetElementPointerType() const MTLPP_AVAILABLE(10_13, 11_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class Argument : public ns::Object<MTLArgument*>
    {
    public:
        Argument();
        Argument(MTLArgument* handle) : ns::Object<MTLArgument*>(handle) { }

        ns::String     GetName() const;
        ArgumentType   GetType() const;
        ArgumentAccess GetAccess() const;
        NSUInteger       GetIndex() const;

        bool           IsActive() const;

        NSUInteger       GetBufferAlignment() const;
        NSUInteger       GetBufferDataSize() const;
        DataType       GetBufferDataType() const;
        StructType     GetBufferStructType() const;
		PointerType	   GetBufferPointerType() const MTLPP_AVAILABLE(10_13, 11_0);

        NSUInteger       GetThreadgroupMemoryAlignment() const;
        NSUInteger       GetThreadgroupMemoryDataSize() const;

        TextureType    GetTextureType() const;
        DataType       GetTextureDataType() const;

        bool           IsDepthTexture() const MTLPP_AVAILABLE(10_12, 10_0);
        NSUInteger       GetArrayLength() const MTLPP_AVAILABLE(10_13, 11_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
}

MTLPP_END
