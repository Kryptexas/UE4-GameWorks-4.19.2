// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShaderCompilerCommon.h"
#include "hlslcc.h"
#include "hlslcc_private.h"
#include "mesa/ast.h"
#include "mesa/glsl_parser_extras.h"
#include "mesa/ir_optimization.h"
#include "mesa/loop_analysis.h"
#include "mesa/macros.h"
#include "ir_track_image_access.h"
#include "mesa/ir_print_visitor.h"
#include "mesa/ir_rvalue_visitor.h"
#include "PackUniformBuffers.h"
#include "OptValueNumbering.h"
#include "IRDump.h"

extern int _mesa_hlsl_debug;

#if defined(_MSC_VER)
	#define WIN32_LEAN_AND_MEAN 1
	#include <Windows.h>

	/** Debug output. */
	void dprintf(const char* Format, ...)
	{
		const int BufSize = (1 << 20);
		static char* Buf = 0;
		va_list Args;
		int Count;

		if (Buf == 0)
		{
			Buf = (char*)malloc(BufSize);
		}

		va_start(Args, Format);
		Count = vsnprintf_s(Buf, BufSize, _TRUNCATE, Format, Args);
		va_end(Args);

		if (Count < -1)
		{
			// Overflow, add a line feed and null terminate the string.
			Buf[BufSize - 2] = '\n';
			Buf[BufSize - 1] = 0;
		}
		else
		{
			// Make sure the string is null terminated.
			Buf[Count] = 0;
		}

		OutputDebugString(Buf);
	}
#endif // #if defined(_MSC_VER)


static const _mesa_glsl_parser_targets FrequencyTable[] =
{
	vertex_shader,
	fragment_shader,
	geometry_shader,
	tessellation_control_shader,
	tessellation_evaluation_shader,
	compute_shader
};

static const int VersionTable[HCT_InvalidTarget] =
{
	150,
	310,
	430,
	150,
	150,
};

/**
 * Optimize IR.
 * @param ir - The IR to optimize.
 * @param ParseState - Parse state.
 */
static void OptimizeIR(exec_list* ir, _mesa_glsl_parse_state* ParseState);


int32 FCrossCompiler::VersionMajor = HLSLCC_VersionMajor;
int32 FCrossCompiler::VersionMinor = HLSLCC_VersionMinor;

FCrossCompiler::FCrossCompiler(uint32 HLSLCCFlags) :
	Flags(HLSLCCFlags)
{
}

FCrossCompiler::~FCrossCompiler()
{
}

bool FCrossCompiler::ParseAndGenerateAST(_mesa_glsl_parse_state* ParseState, const char* InShaderSource)
{
	const bool bPreprocess = (Flags & HLSLCC_NoPreprocess) == 0;

	if (bPreprocess)
	{
		ParseState->error = preprocess(
			ParseState,
			&InShaderSource,
			&ParseState->info_log
			);

		if (ParseState->error != 0)
		{
			return false;
		}
	}

	//_mesa_hlsl_debug = 1;
	_mesa_hlsl_lexer_ctor(ParseState, InShaderSource);
	_mesa_hlsl_parse(ParseState);
	_mesa_hlsl_lexer_dtor(ParseState);

	/**
	 * Debug only functionality to write out the AST to stdout
	 */
	const bool bPrintAST = (Flags & HLSLCC_PrintAST) != 0;
	if (bPrintAST)
	{
		printf( "###########################################################################\n");
		printf( "## Begin AST dump\n");
		_mesa_ast_print( ParseState);
		printf( "## End AST dump\n");
		printf( "###########################################################################\n");
	}

	return true;
}

/**
 * Cross compile HLSL shader code to GLSL.
 * @param InSourceFilename - The filename of the shader source code. This file
 *                           is referenced when generating compile errors.
 * @param InShaderSource - HLSL shader source code.
 * @param InEntryPoint - The name of the entry point.
 * @param InShaderFrequency - The shader frequency.
 * @param InFlags - Flags, see the EHlslCompileFlag enum.
 * @param InCompileTarget - Cross compilation target.
 * @param OutShaderSource - Upon return contains GLSL shader source.
 * @param OutErrorLog - Upon return contains the error log, if any.
 * @returns 0 if compilation failed, non-zero otherwise.
 */
int FCrossCompiler::Run(
	const char* InSourceFilename,
	const char* InShaderSource,
	const char* InEntryPoint,
	EHlslShaderFrequency InShaderFrequency,
	FCodeBackend* InShaderBackEnd,
	ILanguageSpec* InLanguageSpec,
	EHlslCompileTarget InCompileTarget,
	char** OutShaderSource,
	char** OutErrorLog
	)
{
	const bool bIsES2 = (InCompileTarget == HCT_FeatureLevelES2);
	const bool bIsES3_1 = (InCompileTarget == HCT_FeatureLevelES3_1);
	const bool bIsES = bIsES2 || bIsES3_1;
	if (bIsES2)
	{
		// ES implies some flag modifications
		Flags |= HLSLCC_PackUniforms | HLSLCC_FlattenUniformBuffers | HLSLCC_FlattenUniformBufferStructures;
	}
	else if (bIsES3_1)
	{
		// ES implies some flag modifications
		Flags |= HLSLCC_PackUniforms;// | HLSLCC_FlattenUniformBuffers;
	}

	const bool bValidate = (Flags & HLSLCC_NoValidation) == 0;
	const bool bPackUniforms = (Flags & HLSLCC_PackUniforms) != 0;
	const bool bFlattenUBStructures = ((Flags & HLSLCC_FlattenUniformBufferStructures) == HLSLCC_FlattenUniformBufferStructures);
	const bool bFlattenUniformBuffers = ((Flags & HLSLCC_FlattenUniformBuffers) == HLSLCC_FlattenUniformBuffers);
	const bool bGroupFlattenedUBs = ((Flags & HLSLCC_GroupFlattenedUniformBuffers) == HLSLCC_GroupFlattenedUniformBuffers);
	const bool bDoCSE = (Flags & HLSLCC_ApplyCommonSubexpressionElimination) != 0;
	const bool bDoSubexpressionExpansion = (Flags & HLSLCC_ExpandSubexpressions) != 0;

	if (InShaderSource == 0 || OutShaderSource == 0 || OutErrorLog == 0 ||
		InShaderFrequency < HSF_VertexShader || InShaderFrequency > HSF_ComputeShader ||
		InCompileTarget < HCT_FeatureLevelSM4 || InCompileTarget >= HCT_InvalidTarget)
	{
		return 0;
	}

	if ((InShaderFrequency == HSF_HullShader || InShaderFrequency == HSF_DomainShader) && 
			InCompileTarget <= HCT_FeatureLevelSM4)
	{
		return 0;
	}

	if (InShaderFrequency == HSF_ComputeShader && (InCompileTarget < HCT_FeatureLevelES3_1Ext || InCompileTarget == HCT_FeatureLevelES2))
	{
		return 0;
	}

//@todo-rco: Support compute on ES 3_1
	if (bIsES && InShaderFrequency != HSF_VertexShader && InShaderFrequency != HSF_PixelShader)
	{
		// ES only supports VS & PS currently
		return 0;
	}

	*OutShaderSource = 0;
	*OutErrorLog = 0;
	void* MemContext = ralloc_context(0);
	_mesa_glsl_parse_state* ParseState = new(MemContext)_mesa_glsl_parse_state(
		MemContext,
		FrequencyTable[InShaderFrequency],
		InLanguageSpec,
		VersionTable[InCompileTarget]
		);
	ParseState->base_source_file = InSourceFilename;
	ParseState->error = 0;
	ParseState->adjust_clip_space_dx11_to_opengl = (Flags & HLSLCC_DX11ClipSpace) != 0;
	ParseState->bFlattenUniformBuffers = bFlattenUniformBuffers;
	ParseState->bGenerateES = bIsES;
	ParseState->bGenerateLayoutLocations = (InCompileTarget == HCT_FeatureLevelSM5) || (InCompileTarget == HCT_FeatureLevelES3_1Ext);

	exec_list* ir = new(MemContext) exec_list();

	if (!ParseAndGenerateAST(ParseState, InShaderSource))
	{
		goto done;
	}

	if (ParseState->error != 0 || ParseState->translation_unit.is_empty())
	{
		goto done;
	}

	_mesa_ast_to_hir(ir, ParseState);
//IRDump(ir);
	if (ParseState->error != 0 || ir->is_empty())
	{
		goto done;
	}

	if (bValidate)
	{
		validate_ir_tree(ir, ParseState);
		if (ParseState->error != 0)
		{
			goto done;
		}
	}

	if (bIsES)
	{
		ParseState->es_shader = true;
		ParseState->language_version = 100;
	}

	if (InShaderBackEnd == nullptr)
	{
		_mesa_glsl_error(
			ParseState,
			"No Shader code generation backend specified!"
			);
		goto done;
	}

	if (!InShaderBackEnd->GenerateMain(InShaderFrequency, InEntryPoint, ir, ParseState))
	{
		goto done;
	}

	if (!InShaderBackEnd->OptimizeAndValidate(ir, ParseState))
	{
		goto done;
	}

	{
		// Extract sampler states
		if (!ExtractSamplerStatesNameInformation(ir, ParseState))
		{
			goto done;
		}

		if (!InShaderBackEnd->OptimizeAndValidate(ir, ParseState))
		{
			goto done;
		}
	}

	if (bPackUniforms)
	{
		if (bFlattenUBStructures)
		{
			FlattenUniformBufferStructures(ir, ParseState);
			validate_ir_tree(ir, ParseState);

			if (!InShaderBackEnd->OptimizeAndValidate(ir, ParseState))
			{
				goto done;
			}
		}
	}

	{
		if (!InShaderBackEnd->ApplyAndVerifyPlatformRestrictions(ir, ParseState, InShaderFrequency))
		{
			goto done;
		}

		if (!InShaderBackEnd->OptimizeAndValidate(ir, ParseState))
		{
			goto done;
		}
	}

	if (bPackUniforms)
	{
		TVarVarMap UniformMap;
		PackUniforms(ir, ParseState, bFlattenUBStructures, bGroupFlattenedUBs, UniformMap);

		RemovePackedUniformBufferReferences(ir, ParseState, UniformMap);

		if (!InShaderBackEnd->OptimizeAndValidate(ir, ParseState))
		{
			goto done;
		}
	}

	if (bDoCSE)
	{
		if (LocalValueNumbering(ir, ParseState))
		{
			if (!InShaderBackEnd->OptimizeAndValidate(ir, ParseState))
			{
				goto done;
			}
		}
	}

	if (bDoSubexpressionExpansion)
	{
		ExpandSubexpressions(ir, ParseState);
	}

	// pass over the shader to tag image accesses
	TrackImageAccess(ir, ParseState);

	// Just run validation once at the end to make sure it is OK in release mode
	if (!InShaderBackEnd->Validate(ir, ParseState))
	{
		goto done;
	}

	check(ParseState->error == 0);
	*OutShaderSource = InShaderBackEnd->GenerateCode(ir, ParseState, InShaderFrequency);

done:
	int Result = ParseState->error ? 0 : 1;

	if (ParseState->info_log && ParseState->info_log[0] != 0)
	{
		*OutErrorLog = strdup(ParseState->info_log);
		if ((Flags & HLSLCC_DumpIROnError) &&  ir)
		{
			IRDump(ir, ParseState, "ERROR DUMP");
		}
	}
	ralloc_free(MemContext);
	_mesa_glsl_release_types();

	return Result;
}

/**
 * Parses a semantic in to its base name and index.
 * @param MemContext - Memory context with which allocations can be made.
 * @param InSemantic - The semantic to parse.
 * @param OutSemantic - Upon return contains the base semantic.
 * @param OutIndex - Upon return contains the semantic index.
 */
void ParseSemanticAndIndex(
	void* MemContext,
	const char* InSemantic,
	const char** OutSemantic,
	int* OutIndex
	)
{
	check(InSemantic);
	const size_t SemanticLen = strlen(InSemantic);
	const char* p = InSemantic + SemanticLen - 1;

	*OutIndex = 0;
	while (p >= InSemantic && *p >= '0' && *p <= '9')
	{
		*OutIndex = *OutIndex * 10 + (*p - '0');
		p--;
	}
	*OutSemantic = ralloc_strndup(MemContext, InSemantic, p - InSemantic + 1);
}

/**
 * Optimize IR.
 * @param ir - The IR to optimize.
 * @param ParseState - Parse state.
 */
static void DoOptimizeIR(exec_list* ir, _mesa_glsl_parse_state* ParseState, bool bPerformGlobalDeadCodeRemoval)
{
	bool bProgress = false;
	do 
	{
		bProgress = MoveGlobalInstructionsToMain(ir);
		bProgress = do_optimization_pass(ir, ParseState, bPerformGlobalDeadCodeRemoval) || bProgress;
		if (bPerformGlobalDeadCodeRemoval)
		{
			bProgress = ExpandArrayAssignments(ir, ParseState) || bProgress;
		}
	} while (bProgress);
}

static void OptimizeIR(exec_list* ir, _mesa_glsl_parse_state* ParseState)
{
	// We split this into two passes, as there is an issue when we set a value into a static global and the global dead code removal
	// will remove the assignment, leaving the static uninitialized; this happens when a static has a non-const initializer, then
	// is read in a function that's not inline yet; the IR will see a reference, then an assignment
	// so it will then remove the assignment as it thinks it's not used (as it hasn't inlined the function where it will read it!)
	DoOptimizeIR(ir, ParseState, false);
	//do_function_inlining(ir);
	DoOptimizeIR(ir, ParseState, true);
}

/**
 * Moves any instructions in the global instruction stream to the beginning
 * of main. This can happen due to conversions and initializers of global
 * variables. Note however that instructions can be moved iff main() is the
 * only function in the program!
 */
bool MoveGlobalInstructionsToMain(exec_list* Instructions)
{
	ir_function_signature* MainSig = NULL;
	int NumFunctions = 0;
		
	foreach_iter(exec_list_iterator, iter, *Instructions)
	{
		ir_instruction *ir = (ir_instruction *)iter.get();
		ir_function* Function = ir->as_function();
		if (Function)
		{
			foreach_iter(exec_list_iterator, sigiter, *Function)
			{
				ir_function_signature* Sig = (ir_function_signature *)sigiter.get();
				if (Sig->is_main)
				{
					MainSig = Sig;
				}
				if (Sig->is_defined && !Sig->is_builtin)
				{
					NumFunctions++;
				}
			}
		}
	}

	if (MainSig)
	{
		exec_list GlobalIR;
		const bool bMoveGlobalVars = (NumFunctions == 1);

		foreach_iter(exec_list_iterator, iter, *Instructions)
		{
			ir_instruction *ir = (ir_instruction *)iter.get();
			switch (ir->ir_type)
			{
			case ir_type_variable:
				{
					ir_variable* Var = (ir_variable*)ir;
					const bool bBuiltin = Var->name && strncmp(Var->name, "gl_", 3) == 0;
					const bool bTemp = (Var->mode == ir_var_temporary) ||
						(Var->mode == ir_var_auto && bMoveGlobalVars);

					if (!bBuiltin && bTemp)
					{
						Var->remove();
						GlobalIR.push_tail(Var);
					}
				}
				break;

			case ir_type_assignment:
				ir->remove();
				GlobalIR.push_tail(ir);
				break;

			default:
				break;
			}
		}

		if (!GlobalIR.is_empty())
		{
			GlobalIR.append_list(&MainSig->body);
			GlobalIR.move_nodes_to(&MainSig->body);
			return true;
		}
	}
	return false;
}


bool FCodeBackend::Optimize(exec_list* Instructions, _mesa_glsl_parse_state* ParseState)
{
	if (ParseState->error != 0)
	{
		return false;
	}
	OptimizeIR(Instructions, ParseState);

	return (ParseState->error == 0);
}

bool FCodeBackend::Validate(exec_list* Instructions, _mesa_glsl_parse_state* ParseState)
{
	if (ParseState->error != 0)
	{
		return false;
	}

#ifndef NDEBUG
	// This validation always runs. The optimized IR is very small and you really
	// want to know if the final IR is valid.
	validate_ir_tree(Instructions, ParseState);
	if (ParseState->error != 0)
	{
		return false;
	}
#endif

	return true;
}

ir_function_signature* FCodeBackend::FindEntryPointFunction(exec_list* Instructions, _mesa_glsl_parse_state* ParseState, const char* EntryPoint)
{
	ir_function_signature* EntryPointSig = nullptr;
	foreach_iter(exec_list_iterator, Iter, *Instructions)
	{
		ir_instruction *ir = (ir_instruction *)Iter.get();
		ir_function *Function = ir->as_function();
		if (Function && strcmp(Function->name, EntryPoint) == 0)
		{
			int NumSigs = 0;
			foreach_iter(exec_list_iterator, SigIter, *Function)
			{
				if (++NumSigs == 1)
				{
					EntryPointSig = (ir_function_signature *)SigIter.get();
				}
			}
			if (NumSigs == 1)
			{
				break;
			}
			else
			{
				_mesa_glsl_error(ParseState, "shader entry point "
					"'%s' has multiple signatures", EntryPoint);
			}
		}
	}

	return EntryPointSig;
}

ir_function_signature* FCodeBackend::GetMainFunction(exec_list* Instructions)
{
	ir_function_signature* EntryPointSig = nullptr;
	foreach_iter(exec_list_iterator, Iter, *Instructions)
	{
		ir_instruction *ir = (ir_instruction *)Iter.get();
		ir_function *Function = ir->as_function();
		if (Function)
		{
			int NumSigs = 0;
			foreach_iter(exec_list_iterator, SigIter, *Function)
			{
				if (++NumSigs == 1)
				{
					EntryPointSig = (ir_function_signature *)SigIter.get();
				}
			}
			if (NumSigs == 1 && EntryPointSig->is_main)
			{
				return EntryPointSig;
			}
		}
	}
	return nullptr;
}
