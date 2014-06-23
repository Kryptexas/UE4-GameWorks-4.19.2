// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "../hlslcc.h"

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
	std::vector<ir_instruction*> Buffers;

	int GetIndex(ir_variable* Var)
	{
		for (int i = 0, n = (int)Buffers.size(); i < n; ++i)
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
		std::vector<ir_instruction*> AllBuffers;
		AllBuffers.resize(Buffers.size(), nullptr);
		TIRVarList CBuffers;
		for (int i = 0, n = (int)Buffers.size(); i < n; ++i)
		{
			auto* Var = Buffers[i]->as_variable();
			check(Var);
			if (Var->semantic && strlen(Var->semantic) == 1)
			{
				int Index = ConvertArrayTypeToIndex((EArrayType)Var->semantic[0]);
				if (AllBuffers.size() <= Index)
				{
					AllBuffers.resize(Index + 1);
				}
				AllBuffers[Index] = Var;
			}
			else
			{
				CBuffers.push_back(Var);
			}
		}

		for (int i = 0; i < AllBuffers.size() && !CBuffers.empty(); ++i)
		{
			if (!AllBuffers[i])
			{
				AllBuffers[i] = CBuffers.front();
				CBuffers.erase(CBuffers.begin());
			}
		}

		Buffers.clear();
		Buffers.swap(AllBuffers);
	}
};

const glsl_type* PromoteHalfToFloatType(_mesa_glsl_parse_state* state, const glsl_type* type);
