/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include "library.hpp"
#include "device.hpp"
#include "function_constant_values.hpp"
#include "argument_encoder.hpp"
#include <Metal/MTLLibrary.h>

namespace mtlpp
{
    VertexAttribute::VertexAttribute() :
        ns::Object<MTLVertexAttribute*>([[MTLVertexAttribute alloc] init], false)
    {
    }

    ns::String VertexAttribute::GetName() const
    {
        Validate();
        return [(MTLVertexAttribute*)m_ptr name];
    }

    uint32_t VertexAttribute::GetAttributeIndex() const
    {
        Validate();
        return uint32_t([(MTLVertexAttribute*)m_ptr attributeIndex]);
    }

    DataType VertexAttribute::GetAttributeType() const
    {
        Validate();
        return DataType([(MTLVertexAttribute*)m_ptr attributeType]);
    }

    bool VertexAttribute::IsActive() const
    {
        Validate();
        return [(MTLVertexAttribute*)m_ptr isActive];
    }

    bool VertexAttribute::IsPatchData() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return [(MTLVertexAttribute*)m_ptr isActive];
#else
        return false;
#endif
    }

    bool VertexAttribute::IsPatchControlPointData() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return [(MTLVertexAttribute*)m_ptr isActive];
#else
        return false;
#endif
    }

    Attribute::Attribute() :
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        ns::Object<MTLAttribute*>([[MTLAttribute alloc] init], false)
#else
        ns::Object<MTLAttribute*>(nullptr)
#endif
    {
    }

    ns::String Attribute::GetName() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return [(MTLAttribute*)m_ptr name];
#else
        return nullptr;
#endif
    }

    uint32_t Attribute::GetAttributeIndex() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return uint32_t([(MTLAttribute*)m_ptr attributeIndex]);
#else
        return 0;
#endif
    }

    DataType Attribute::GetAttributeType() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return DataType([(MTLAttribute*)m_ptr attributeType]);
#else
        return DataType(0);
#endif
    }

    bool Attribute::IsActive() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return [(MTLAttribute*)m_ptr isActive];
#else
        return false;
#endif
    }

    bool Attribute::IsPatchData() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return [(MTLAttribute*)m_ptr isActive];
#else
        return false;
#endif
    }

    bool Attribute::IsPatchControlPointData() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return [(MTLAttribute*)m_ptr isActive];
#else
        return false;
#endif
    }

    FunctionConstant::FunctionConstant() :
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        ns::Object<MTLFunctionConstant*>([[MTLFunctionConstant alloc] init], false)
#else
        ns::Object<MTLFunctionConstant*>(nullptr)
#endif
    {
    }

    ns::String FunctionConstant::GetName() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return [(MTLFunctionConstant*)m_ptr name];
#else
        return nullptr;
#endif
    }

    DataType FunctionConstant::GetType() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return DataType([(MTLFunctionConstant*)m_ptr type]);
#else
        return DataType(0);
#endif
    }

    uint32_t FunctionConstant::GetIndex() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return uint32_t([(MTLFunctionConstant*)m_ptr index]);
#else
        return 0;
#endif
    }

    bool FunctionConstant::IsRequired() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return [(MTLFunctionConstant*)m_ptr required];
#else
        return false;
#endif
    }

    ns::String Function::GetLabel() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return [(id<MTLFunction>)m_ptr label];
#else
        return nullptr;
#endif
    }

    Device Function::GetDevice() const
    {
        Validate();
        return [(id<MTLFunction>)m_ptr device];
    }

    FunctionType Function::GetFunctionType() const
    {
        Validate();
        return FunctionType([(id<MTLFunction>)m_ptr functionType]);
    }

    PatchType Function::GetPatchType() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return PatchType([(id<MTLFunction>)m_ptr patchType]);
#else
        return PatchType(0);
#endif
    }

    int32_t Function::GetPatchControlPointCount() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return int32_t([(id<MTLFunction>)m_ptr patchControlPointCount]);
#else
        return 0;
#endif
    }

    const ns::Array<VertexAttribute> Function::GetVertexAttributes() const
    {
        Validate();
        return [(id<MTLFunction>)m_ptr vertexAttributes];
    }

    const ns::Array<Attribute> Function::GetStageInputAttributes() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return [(id<MTLFunction>)m_ptr stageInputAttributes];
#else
        return nullptr;
#endif
    }

    ns::String Function::GetName() const
    {
        Validate();
        return [(id<MTLFunction>)m_ptr name];
    }

    ns::Dictionary<ns::String, FunctionConstant> Function::GetFunctionConstants() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return [(id<MTLFunction>)m_ptr functionConstantsDictionary];
#else
        return nullptr;
#endif
    }
	
	ArgumentEncoder Function::NewArgumentEncoderWithBufferIndex(uint32_t index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return [(id<MTLFunction>)m_ptr newArgumentEncoderWithBufferIndex:index];
#else
		return ArgumentEncoder();
#endif
	}

	ArgumentEncoder Function::NewArgumentEncoderWithBufferIndex(uint32_t index, Argument* reflection)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		MTLArgument* arg = nil;
		ArgumentEncoder encoder( [(id<MTLFunction>)m_ptr newArgumentEncoderWithBufferIndex:index reflection:reflection ? &arg : nil] );
		if (reflection) { *reflection = arg; }
		return encoder;
#else
		return ArgumentEncoder();
#endif
	}

    void Function::SetLabel(const ns::String& label)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        [(id<MTLFunction>)m_ptr setLabel:(NSString*)label.GetPtr()];
#endif
    }

    ns::Dictionary<ns::String, ns::String> CompileOptions::GetPreprocessorMacros() const
    {
        Validate();
        return [(MTLCompileOptions*)m_ptr preprocessorMacros];
    }

    bool CompileOptions::IsFastMathEnabled() const
    {
        Validate();
        return [(MTLCompileOptions*)m_ptr fastMathEnabled];
    }

    LanguageVersion CompileOptions::GetLanguageVersion() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
        return LanguageVersion([(MTLCompileOptions*)m_ptr languageVersion]);
#else
        return LanguageVersion::Version1_0;
#endif
    }

    void CompileOptions::SetFastMathEnabled(bool fastMathEnabled)
    {
        Validate();
        [(MTLCompileOptions*)m_ptr setFastMathEnabled:fastMathEnabled];
    }

    void CompileOptions::SetLanguageVersion(LanguageVersion languageVersion)
    {
        Validate();
        [(MTLCompileOptions*)m_ptr setLanguageVersion:MTLLanguageVersion(languageVersion)];
    }

    ns::String Library::GetLabel() const
    {
        Validate();
        return [(id<MTLLibrary>)m_ptr label];
    }

    void Library::SetLabel(const ns::String& label)
    {
        Validate();
        [(id<MTLLibrary>)m_ptr setLabel:(NSString*)label.GetPtr()];
    }

    ns::Array<ns::String> Library::GetFunctionNames() const
    {
        Validate();
        return [(id<MTLLibrary>)m_ptr functionNames];
    }

    Function Library::NewFunction(const ns::String& functionName) const
    {
        Validate();
        return [(id<MTLLibrary>)m_ptr newFunctionWithName:(NSString*)functionName.GetPtr()];
    }

    Function Library::NewFunction(const ns::String& functionName, const FunctionConstantValues& constantValues, ns::Error* error) const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        NSError** nsError = error ? (NSError**)error->GetInnerPtr() : nullptr;
        return [(id<MTLLibrary>)m_ptr
                                            newFunctionWithName:(NSString*)functionName.GetPtr()
                                            constantValues:(MTLFunctionConstantValues*)constantValues.GetPtr()
                                            error:nsError];
#else
        return nullptr;
#endif
    }

    void Library::NewFunction(const ns::String& functionName, const FunctionConstantValues& constantValues, std::function<void(const Function&, const ns::Error&)> completionHandler) const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        [(id<MTLLibrary>)m_ptr
             newFunctionWithName:(NSString*)functionName.GetPtr()
             constantValues:(MTLFunctionConstantValues*)constantValues.GetPtr()
             completionHandler:^(id <MTLFunction> mtlFunction, NSError* error){
                 completionHandler(mtlFunction, error);
             }];
#endif
    }

}
