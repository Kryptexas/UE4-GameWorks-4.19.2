// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// .

#pragma once

#include "CoreMinimal.h"

#include "hlslcc.h"
THIRD_PARTY_INCLUDES_START
	#include "ir.h"
THIRD_PARTY_INCLUDES_END
#include "PackUniformBuffers.h"

inline std::string FixVecPrefix(std::string Type)
{
	if (!strncmp("vec", Type.c_str(), 3))
	{
		std::string Num = Type.substr(3);
		Type = "float";
		Type += Num;
	}
	else if (!strncmp("bvec", Type.c_str(), 4))
	{
		std::string Num = Type.substr(4);
		Type = "bool";
		Type += Num;
	}
	else if (!strncmp("ivec", Type.c_str(), 4))
	{
		std::string Num = Type.substr(4);
		Type = "int";
		Type += Num;
	}
	else if (!strncmp("uvec", Type.c_str(), 4))
	{
		std::string Num = Type.substr(4);
		Type = "uint";
		Type += Num;
	}
	else if (!strncmp("mat", Type.c_str(), 3))
	{
		std::string Num = Type.substr(3);
		Type = "float";
		Type += Num;
		Type += "x";
		Type += Num;
	}

	return Type;
}

/** Track external variables. */
struct extern_var : public exec_node
{
	ir_variable* var;
	explicit extern_var(ir_variable* in_var) : var(in_var) {}
};


struct FBuffers
{
	TIRVarSet AtomicVariables;

	TArray<ir_variable*> Buffers;
    
    TArray<ir_variable*> Textures;

	// Information about textures & samplers; we need to have unique samplerstate indices, as one they can be used independent of each other
	TArray<std::string> UniqueSamplerStates;

	void AddBuffer(ir_variable* Var)
	{
		check(Var);
		check(Var->mode == ir_var_uniform || Var->mode == ir_var_out || Var->mode == ir_var_in || Var->mode == ir_var_shared);
		
		Buffers.Add(Var);
	}
	
	void AddTexture(ir_variable* Var)
	{
		check(Var);
		check(Var->mode == ir_var_uniform || Var->mode == ir_var_out || Var->mode == ir_var_in || Var->mode == ir_var_shared);
		
		Textures.Add(Var);
	}
	
	int32 GetUniqueSamplerStateIndex(const std::string& Name, bool bAddIfNotFound, bool& bOutAdded)
	{
		int32 Found = INDEX_NONE;
		bOutAdded = false;
		if (UniqueSamplerStates.Find(Name, Found))
		{
			return Found;
		}
		
		if (bAddIfNotFound)
		{
			UniqueSamplerStates.Add(Name);
			bOutAdded = true;
			return UniqueSamplerStates.Num() - 1;
		}

		return Found;
	}

	int GetIndex(ir_variable* Var)
	{
		for (int i = 0, n = Buffers.Num(); i < n; ++i)
		{
			if (Buffers[i] == Var)
			{
				return i;
			}
        }
        
        for (int i = 0, n = Textures.Num(); i < n; ++i)
        {
            if (Textures[i] == Var)
            {
                return i;
            }
        }

		return -1;
	}

	int GetIndex(const std::string& Name)
	{
		for (int i = 0, n = Buffers.Num(); i < n; ++i)
		{
			auto* Var = Buffers[i] ? Buffers[i]->as_variable() : nullptr;
			if (Var && Var->name && Var->name == Name)
			{
				return i;
			}
		}
        
        for (int i = 0, n = Textures.Num(); i < n; ++i)
        {
            auto* Var = Textures[i] ? Textures[i]->as_variable() : nullptr;
            if (Var && Var->name && Var->name == Name)
            {
                return i;
            }
        }

		return -1;
	}

	void SortBuffers(_mesa_glsl_parse_state* state)
	{
		TArray<ir_variable*> AllBuffers;
		AllBuffers.AddZeroed(Buffers.Num());
        TArray<ir_variable*> AllTextures;
        AllTextures.AddZeroed(Textures.Num());
        TIRVarList CBuffers;
        TIRVarList IBuffers;
        TIRVarList TBuffers;
        TIRVarList ITextures;
        TIRVarList RTextures;
        TIRVarList TTextures;
        
		// Put packed UB's into their location (h=0, m=1, etc); leave holes if not using a packed define
		// and group the regular CBuffers in another list
		for (int i = 0, n = Buffers.Num(); i < n; ++i)
		{
			auto* Var = Buffers[i]->as_variable();
			check(Var);
            if(Var->type->sampler_buffer && Var->type->inner_type->components() != 3 && !Var->invariant)
            {
                TBuffers.push_back(Var);
            }
            else if (Var->type->is_image())
            {
                IBuffers.push_back(Var);
            }
			else if (Var->semantic && strlen(Var->semantic) == 1)
			{
				int Index = ConvertArrayTypeToIndex((EArrayType)Var->semantic[0]);
				if (AllBuffers.Num() <= Index)
				{
					int32 Count = Index + 1 - AllBuffers.Num();
					AllBuffers.AddZeroed(Count);
					//AllBuffers.Resize(Index + 1, nullptr);
				}
				AllBuffers[Index] = Var;
			}
			else
			{
				CBuffers.push_back(Var);
			}
        }
        
        for (int i = 0, n = Textures.Num(); i < n; ++i)
        {
            auto* Var = Textures[i]->as_variable();
            check(Var);
            
            if(Var->type->sampler_buffer && Var->type->inner_type->components() != 3 && !Var->invariant)
            {
                TTextures.push_back(Var);
            }
            else if (Var->type->is_image())
            {
                ITextures.push_back(Var);
            }
            else
            {
                RTextures.push_back(Var);
            }
        }
        
        uint64 UAVIndices = 0;
        
        // Fill the holes in the packed array list with IB's first, recording which UAV indices are bound...
        for (int i = 0; i < AllBuffers.Num() && !IBuffers.empty(); ++i)
        {
            if (!AllBuffers[i])
            {
                // i *must* be less than 8 as we only support indices 0-7 for UAVs and beyond that we can't bind it.
                if (i >= 8)
                {
                    _mesa_glsl_warning(state, "Image buffer '%s' at index '%d' cannot be bound as part of the render-target array.",
                                   IBuffers.front()->name, i);
                }
                
                AllBuffers[i] = IBuffers.front();
                IBuffers.erase(IBuffers.begin());
                
                UAVIndices |= (uint64(1) << i);
            }
        }
        
        for(uint64 i = 0; i < 64ull && !ITextures.empty(); i++)
        {
            check(i < 64);
            if (!(UAVIndices & (1ull << i)))
            {
                // i *must* be less than 8 as we only support indices 0-7 for UAVs and beyond that we can't bind it.
                if (i >= 8)
                {
                    _mesa_glsl_warning(state, "Image texture '%s' at index '%d' cannot be bound as part of the render-target array.",
                                       ITextures.front()->name, i);
                }
                if (AllTextures.Num() <= i)
                {
                    int32 Count = i + 1 - AllTextures.Num();
                    AllTextures.AddZeroed(Count);
                    //AllTextures(Index + 1, nullptr);
                }
                AllTextures[i] = ITextures.front();
                ITextures.erase(ITextures.begin());
                
                UAVIndices |= (uint64(1) << i);
            }
        }
        
        // Below we insert typed buffers and typed textures first: these are the Buffer<*> emulations and consume both a buffer and texture slot.
        // This allows us to use the BufferSize meta-table rather than texture2d.get_width()/.get_height() which aren't very efficient.
        // It also means that our binding points are consistent whether we use textures or buffers so makes switching between a shader variant using textures vs. buffers easy.
        // This means you will get holes in the slots where we see a typed-resource on the other array.
        // We always leave a hole for BufferSizes at index 31.
        
        uint32 TypedBuffers = 0;
        static const uint32 MaxBuffers = 30;
        static const uint32 MaxTextures = 128;
        
        for (int i = 0; i < MaxBuffers && !TBuffers.empty(); ++i)
        {
            if ((i >= AllTextures.Num() || !AllTextures[i]) && (i >= AllBuffers.Num() || !AllBuffers[i]))
            {
                if (AllBuffers.Num() <= i)
                {
                    int32 Count = i + 1 - AllBuffers.Num();
                    AllBuffers.AddZeroed(Count);
                }
                AllBuffers[i] = TBuffers.front();
                TBuffers.erase(TBuffers.begin());
                TypedBuffers |= (1 << i);
            }
        }
        
        for(int i = 0; i < MaxBuffers && !TTextures.empty(); i++)
        {
            if ((i >= AllTextures.Num() || !AllTextures[i]) && (i >= AllBuffers.Num() || !AllBuffers[i]))
            {
                if (AllTextures.Num() <= i)
                {
                    int32 Count = i + 1 - AllTextures.Num();
                    AllTextures.AddZeroed(Count);
                }
                AllTextures[i] = TTextures.front();
                TTextures.erase(TTextures.begin());
                TypedBuffers |= (1 << i);
            }
        }

		// Fill the holes in the packed array list with real UB's
		for (int i = 0; i < MaxBuffers && !CBuffers.empty(); ++i)
		{
            // Can't override a typed-buffer/texture
			if (!(TypedBuffers & (1 << i)) && (i >= AllBuffers.Num() || !AllBuffers[i]))
			{
                if (AllBuffers.Num() <= i)
                {
                    int32 Count = i + 1 - AllBuffers.Num();
                    AllBuffers.AddZeroed(Count);
                }
				AllBuffers[i] = CBuffers.front();
				CBuffers.erase(CBuffers.begin());
			}
		}
        
        for(int i = 0; i < MaxTextures && !RTextures.empty(); i++)
        {
            // Can't override a typed-buffer/texture
            if ((i >= MaxBuffers || !(TypedBuffers & (1 << i))) && (i >= AllTextures.Num() || !AllTextures[i]))
            {
                if (AllTextures.Num() <= i)
                {
                    int32 Count = i + 1 - AllTextures.Num();
                    AllTextures.AddZeroed(Count);
                }
                AllTextures[i] = RTextures.front();
                RTextures.erase(RTextures.begin());
            }
        }
        
        if (!CBuffers.empty() || !IBuffers.empty() || !TBuffers.empty() || !ITextures.empty() || !RTextures.empty() || !TTextures.empty())
        {
            TIRVarList BufferList;
            BufferList.merge(CBuffers);
            BufferList.merge(IBuffers);
            BufferList.merge(TBuffers);
            
            for (auto it : BufferList)
            {
                _mesa_glsl_error(state, "Buffer '%s' cannot be allocated an appropriate index due to resource overflow.",
                           *it->name);
            }
            
            TIRVarList TextureList;
            TextureList.merge(ITextures);
            TextureList.merge(RTextures);
            TextureList.merge(TTextures);
            
            for (auto it : TextureList)
            {
                _mesa_glsl_error(state, "Texture '%s' cannot be allocated an appropriate index due to resource overflow.",
                                 *it->name);
            }
        }

		Buffers = AllBuffers;
		Textures = AllTextures;
	}
};

const glsl_type* PromoteHalfToFloatType(_mesa_glsl_parse_state* state, const glsl_type* type);

struct FSemanticQualifier
{
	struct
	{
		bool bIsPatchConstant = false;
		bool bIsTessellationVSHS = false; // TODO FIXME UGLY UGLY HACK (this is not the right place to put this flag)
	} Fields;
};

namespace MetalUtils
{
	ir_dereference_variable* GenerateInput(EHlslShaderFrequency Frequency, uint32 bIsDesktop, _mesa_glsl_parse_state* ParseState, const char* InputSemantic,
		const glsl_type* InputType, exec_list* DeclInstructions, exec_list* PreCallInstructions);

	ir_dereference_variable* GenerateOutput(EHlslShaderFrequency Frequency, uint32 bIsDesktop, _mesa_glsl_parse_state* ParseState, const char* OutputSemantic,
		FSemanticQualifier Qualifier,
		const glsl_type* OutputType, exec_list* DeclInstructions, exec_list* PreCallInstructions, exec_list* PostCallInstructions);
}

const int MAX_SIMULTANEOUS_RENDER_TARGETS = 8;

/*static unsigned int glslTypeSize(const glsl_type *const t)
{
	// consider t->component_slots()
	// TODO this does nothing to handle alignment
	unsigned int size = 0;
	check(!(t->is_void() || t->is_error() || t->is_sampler() || t->is_image() || t->IsSamplerState() || t->is_outputstream()));
	if(t->is_array())
	{
		return t->length * glslTypeSize(t->element_type());
	}
	else if(t->is_patch())
	{
		return t->patch_length * glslTypeSize(t->inner_type);
	}
	else if(t->is_record())
	{
		for (unsigned i = 0; i < t->length; i++)
		{
			size += glslTypeSize(t->fields.structure[i].type);
		}
	}
	else if(t->is_matrix() || t->is_vector())
	{
		return t->components() * glslTypeSize(t->get_base_type());
	}
	else
	{
		check(t->is_scalar());
		switch(t->base_type)
		{
		case GLSL_TYPE_UINT:
		case GLSL_TYPE_INT:
		case GLSL_TYPE_FLOAT:
			size = 4;
			break;
		case GLSL_TYPE_HALF:
			size = 2;
			break;
		case GLSL_TYPE_BOOL:
			size = 1;
			break;
		default:
			check(0);
		}
	}
	return size;
}*/
