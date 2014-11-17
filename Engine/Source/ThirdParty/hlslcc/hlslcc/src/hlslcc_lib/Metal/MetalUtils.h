// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "../hlslcc.h"

//@todo-rco: Remove STL!
#include <vector>

inline std::string FixVecPrefix(std::string Type)
{
	if (!strncmp("vec", Type.c_str(), 3))
	{
		std::string Num = Type.substr(3);
		Type = "float";
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
	TArray<ir_instruction*> Buffers;

	int GetIndex(ir_variable* Var)
	{
		for (int i = 0, n = Buffers.Num(); i < n; ++i)
		{
			if (Buffers[i] == Var)
			{
				return i;
			}
		}

		return -1;
	}

	void SortBuffers()
	{
		TArray<ir_instruction*> AllBuffers;
		AllBuffers.Resize(Buffers.Num(), nullptr);
		TIRVarList CBuffers;
		// Put packed UB's into their location (h=0, m=1, etc); leave holes if not using a packed define
		// and group the regular CBuffers in another list
		for (int i = 0, n = Buffers.Num(); i < n; ++i)
		{
			auto* Var = Buffers[i]->as_variable();
			check(Var);
			if (Var->semantic && strlen(Var->semantic) == 1)
			{
				int Index = ConvertArrayTypeToIndex((EArrayType)Var->semantic[0]);
				if (AllBuffers.Num() <= Index)
				{
					AllBuffers.Resize(Index + 1, nullptr);
				}
				AllBuffers[Index] = Var;
			}
			else
			{
				CBuffers.push_back(Var);
			}
		}

		// Fill the holes in the packed array list with real UB's
		for (int i = 0; i < AllBuffers.Num() && !CBuffers.empty(); ++i)
		{
			if (!AllBuffers[i])
			{
				AllBuffers[i] = CBuffers.front();
				CBuffers.erase(CBuffers.begin());
			}
		}

		Buffers = AllBuffers;
	}
};

const glsl_type* PromoteHalfToFloatType(_mesa_glsl_parse_state* state, const glsl_type* type);
