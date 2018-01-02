/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLLibrary.h>
#include "library.hpp"
#include "device.hpp"
#include "function_constant_values.hpp"
#include "argument_encoder.hpp"

MTLPP_BEGIN

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

    NSUInteger VertexAttribute::GetAttributeIndex() const
    {
        Validate();
        return NSUInteger([(MTLVertexAttribute*)m_ptr attributeIndex]);
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

    NSUInteger Attribute::GetAttributeIndex() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return NSUInteger([(MTLAttribute*)m_ptr attributeIndex]);
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

    NSUInteger FunctionConstant::GetIndex() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return NSUInteger([(MTLFunctionConstant*)m_ptr index]);
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
        return m_table->Label(m_ptr);
#else
        return nullptr;
#endif
    }

    Device Function::GetDevice() const
    {
        Validate();
		return m_table->Device(m_ptr);
    }

    FunctionType Function::GetFunctionType() const
    {
        Validate();
		return FunctionType(m_table->FunctionType(m_ptr));
    }

    PatchType Function::GetPatchType() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		return PatchType(m_table->PatchType(m_ptr));
#else
        return PatchType(0);
#endif
    }

    NSInteger Function::GetPatchControlPointCount() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		return m_table->PatchControlPointCount(m_ptr);
#else
        return 0;
#endif
    }

    const ns::Array<VertexAttribute> Function::GetVertexAttributes() const
    {
        Validate();
		return m_table->VertexAttributes(m_ptr);
    }

    const ns::Array<Attribute> Function::GetStageInputAttributes() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		return m_table->StageInputAttributes(m_ptr);
#else
        return nullptr;
#endif
    }

    ns::String Function::GetName() const
    {
        Validate();
		return m_table->Name(m_ptr);
    }

    ns::Dictionary<ns::String, FunctionConstant> Function::GetFunctionConstants() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		return m_table->FunctionConstantsDictionary(m_ptr);
#else
        return nullptr;
#endif
    }
	
	ArgumentEncoder Function::NewArgumentEncoderWithBufferIndex(NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return m_table->NewArgumentEncoderWithBufferIndex(m_ptr, index);
#else
		return ArgumentEncoder();
#endif
	}

	ArgumentEncoder Function::NewArgumentEncoderWithBufferIndex(NSUInteger index, Argument* reflection)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		
		MTLArgument* arg = nil;
		ArgumentEncoder encoder(m_table->NewArgumentEncoderWithBufferIndexreflection(m_ptr, index, reflection ? &arg : nil));
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
		m_table->SetLabel(m_ptr, label.GetPtr());
#endif
    }

    ns::Dictionary<ns::String, ns::Object<NSObject*>> CompileOptions::GetPreprocessorMacros() const
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
		return m_table->Label(m_ptr);
    }

    void Library::SetLabel(const ns::String& label)
    {
        Validate();
		m_table->SetLabel(m_ptr, label.GetPtr());
    }

    ns::Array<ns::String> Library::GetFunctionNames() const
    {
        Validate();
		return m_table->FunctionNames(m_ptr);
    }

    Function Library::NewFunction(const ns::String& functionName) const
    {
        Validate();
		return m_table->NewFunctionWithName(m_ptr, functionName.GetPtr());
    }

    Function Library::NewFunction(const ns::String& functionName, const FunctionConstantValues& constantValues, ns::AutoReleasedError* error) const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		return m_table->NewFunctionWithNameconstantValueserror(m_ptr, functionName.GetPtr(), constantValues.GetPtr(), error ? (NSError**)error->GetInnerPtr() : nullptr);
#else
        return nullptr;
#endif
    }

    void Library::NewFunction(const ns::String& functionName, const FunctionConstantValues& constantValues, FunctionHandler completionHandler) const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
		m_table->NewFunctionWithNameconstantValuescompletionHandler(m_ptr, functionName.GetPtr(), constantValues.GetPtr(), ^(id <MTLFunction> mtlFunction, NSError* error)
		{
			completionHandler(mtlFunction, error);
		});
#endif
    }

}

MTLPP_END
