// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	HlslUtils.cpp - Utils for HLSL.
=============================================================================*/

#include "ShaderCompilerCommon.h"
#include "HlslUtils.h"
#include "HlslAST.h"
#include "HlslParser.h"

namespace CrossCompiler
{
	namespace Memory
	{
		static struct FPagePoolInstance
		{
			~FPagePoolInstance()
			{
				check(UsedPages.Num() == 0);
				for (int32 Index = 0; Index < FreePages.Num(); ++Index)
				{
					delete FreePages[Index];
				}
			}

			FPage* AllocatePage()
			{
				FScopeLock ScopeLock(&CriticalSection);

				if (FreePages.Num() == 0)
				{
					FreePages.Add(new FPage(PageSize));
				}

				auto* Page = FreePages.Last();
				FreePages.RemoveAt(FreePages.Num() - 1, 1, false);
				UsedPages.Add(Page);
				return Page;
			}

			void FreePage(FPage* Page)
			{
				FScopeLock ScopeLock(&CriticalSection);

				int32 Index = UsedPages.Find(Page);
				check(Index >= 0);
				UsedPages.RemoveAt(Index, 1, false);
				FreePages.Add(Page);
			}

			TArray<FPage*, TInlineAllocator<8> > FreePages;
			TArray<FPage*, TInlineAllocator<8> > UsedPages;

			FCriticalSection CriticalSection;

		} GMemoryPagePool;

		FPage* FPage::AllocatePage()
		{
#if USE_PAGE_POOLING
			return GMemoryPagePool.AllocatePage();
#else
			return new FPage(PageSize);
#endif
		}

		void FPage::FreePage(FPage* Page)
		{
#if USE_PAGE_POOLING
			GMemoryPagePool.FreePage(Page);
#else
			delete Page;
#endif
		}
	}
}


struct FRemoveUnusedOutputsData
{
	const TArray<FString>& SystemOutputs;
	const TArray<FString>& UsedOutputs;
	FString EntryPoint;
	bool bSuccess;
	FString GeneratedCode;

	FRemoveUnusedOutputsData(const TArray<FString>& InSystemOutputs, const TArray<FString>& InUsedOutputs) :
		SystemOutputs(InSystemOutputs),
		UsedOutputs(InUsedOutputs),
		bSuccess(false)
	{
	}
};

static CrossCompiler::AST::FUnaryExpression* MakeIdentifierExpression(CrossCompiler::FLinearAllocator* Allocator, const TCHAR* Name, const CrossCompiler::FSourceInfo& SourceInfo)
{
	using namespace CrossCompiler::AST;
	auto* Expression = new(Allocator) FUnaryExpression(Allocator, EOperators::Identifier, nullptr, SourceInfo);
	Expression->Identifier = Allocator->Strdup(Name);
	return Expression;
};

/*
static void CopyVar(CrossCompiler::FLinearAllocator* Allocator, const TCHAR* DestVar, CrossCompiler::AST::FDeclaration* DestMemberDeclaration, CrossCompiler::AST::FStructSpecifier* DestStruct, const TCHAR* SourceVar, CrossCompiler::AST::FDeclaration* SourceMemberDeclaration, CrossCompiler::AST::FStructSpecifier* SourceStruct, CrossCompiler::AST::FCompoundStatement* Body)
{
	using namespace CrossCompiler::AST;

	auto* DestVarMember = MakeIdentifierExpression(Allocator, *FString::Printf(TEXT("%s.%s"), DestVar, DestMemberDeclaration->Identifier), DestStruct->SourceInfo);
	auto* SourceVarMember = MakeIdentifierExpression(Allocator, *FString::Printf(TEXT("%s.%s"), SourceVar, SourceMemberDeclaration->Identifier), SourceStruct->SourceInfo);
	auto* NewAssignment = new(Allocator) FBinaryExpression(Allocator, EOperators::Assign, DestVarMember, SourceVarMember, DestStruct->SourceInfo);
	Body->Statements.Add(new(Allocator) FExpressionStatement(Allocator, NewAssignment, DestStruct->SourceInfo));
}*/

static bool CopyVariable(CrossCompiler::FLinearAllocator* Allocator, CrossCompiler::AST::FCompoundStatement* Body, CrossCompiler::AST::FTypeSpecifier* DestType, const TCHAR* DestVar, CrossCompiler::AST::FTypeSpecifier* SourceType, const TCHAR* SourceVar)
{
	using namespace CrossCompiler::AST;
	if (DestType->Structure)
	{
		//#todo-rco: Handle nested structures
		check(0);
	}
	else if (DestType->bIsArray)
	{
		//#todo-rco: Handle arrays. Shouldn't hit here as it was trapped before this
		check(0);
	}
	else
	{
		auto* DestVarMember = MakeIdentifierExpression(Allocator, DestVar, DestType->SourceInfo);
		auto* SourceVarMember = MakeIdentifierExpression(Allocator, SourceVar, SourceType->SourceInfo);
		auto* NewAssignment = new(Allocator) FBinaryExpression(Allocator, EOperators::Assign, DestVarMember, SourceVarMember, DestType->SourceInfo);
		Body->Statements.Add(new(Allocator) FExpressionStatement(Allocator, NewAssignment, DestType->SourceInfo));
	}

	return true;
}

static bool CopyStructMembers(CrossCompiler::FLinearAllocator* Allocator, CrossCompiler::AST::FCompoundStatement* Body, CrossCompiler::AST::FStructSpecifier* DestStruct, const TCHAR* DestVar, CrossCompiler::AST::FStructSpecifier* SourceStruct, const TCHAR* SourceVar)
{
	using namespace CrossCompiler::AST;

	auto ArrayTypesMatch = [](FDeclaration* A, FDeclaration* B)
	{
		if (A->bIsArray != B->bIsArray)
		{
			return false;
		}
		else if (A->bIsArray && B->bIsArray)
		{
			if (A->ArraySize.Num() != B->ArraySize.Num())
			{
				return false;
			}

			for (int32 Index = 0; Index < A->ArraySize.Num(); ++Index)
			{
				FUnaryExpression* UnaryA = A->ArraySize[Index]->AsUnaryExpression();
				FUnaryExpression* UnaryB = A->ArraySize[Index]->AsUnaryExpression();
				if (!UnaryA || !UnaryB || !UnaryA->IsConstant() || !UnaryB->IsConstant())
				{
					return false;
				}

				uint32 DimA = UnaryA->GetUintConstantValue();
				uint32 DimB = UnaryB->GetUintConstantValue();
				if (DimA != DimB)
				{
					return false;
				}
			}
		}

		return true;
	};

	auto TypesMatch = [](FTypeSpecifier* A, FTypeSpecifier* B)
		{
			if (A->bIsArray != B->bIsArray)
			{
				return false;
			}
			else if (A->bIsArray && B->bIsArray)
			{
				if ((A->ArraySize && !B->ArraySize) || (!A->ArraySize && B->ArraySize))
				{
					return false;
				}
				else if (A->ArraySize && B->ArraySize)
				{
					//#todo-rco
					check(0);
				}
			}

			if (FCString::Strcmp(A->TypeName, B->TypeName) != 0)
			{
				return false;
			}

			if ((A->InnerType && !B->InnerType) || (!A->InnerType && B->InnerType))
			{
				return false;
			}
			else if (A->InnerType && B->InnerType)
			{
				if (FCString::Strcmp(A->InnerType, B->InnerType) != 0)
				{
					return false;
				}
			}

			if ((A->Structure && !B->Structure) || (!A->Structure && B->Structure))
			{
				return false;
			}
			else if (A->Structure && B->Structure)
			{
				//#todo-rco
				check(0);
			}

			if (A->TextureMSNumSamples != B->TextureMSNumSamples || A->PatchSize != B->PatchSize)
			{
				return false;
			}

			return true;
		};

	int32 SourceIndex = 0;
	for (int32 DestIndex = 0; DestIndex < DestStruct->Declarations.Num(); ++DestIndex)
	{
		FDeclaratorList* DestMemberDeclarator = DestStruct->Declarations[DestIndex]->AsDeclaratorList();
		for (int32 DeclIndex = 0; DeclIndex < DestMemberDeclarator->Declarations.Num(); ++DeclIndex)
		{
			FDeclaration* DestMemberDeclaration = DestMemberDeclarator->Declarations[DeclIndex]->AsDeclaration();

			// Find corresponding source member
			FDeclaratorList* SourceMemberDeclarator = nullptr;
			FDeclaration* SourceMemberDeclaration = nullptr;
			bool bFound = false;
			for ( ; SourceIndex < SourceStruct->Declarations.Num(); ++SourceIndex)
			{
				FDeclaratorList* TestSourceDeclarator = SourceStruct->Declarations[SourceIndex]->AsDeclaratorList();
				if (TypesMatch(TestSourceDeclarator->Type->Specifier, DestMemberDeclarator->Type->Specifier))
				{
					for (int32 SourceDeclIndex = 0; SourceDeclIndex < TestSourceDeclarator->Declarations.Num(); ++SourceDeclIndex)
					{
						FDeclaration* TestSourceDeclaration = TestSourceDeclarator->Declarations[SourceDeclIndex]->AsDeclaration();
						if (!FCString::Strcmp(TestSourceDeclaration->Identifier, DestMemberDeclaration->Identifier))
						{
							if (TestSourceDeclaration->Semantic && DestMemberDeclaration->Semantic)
							{
								if (!FCString::Strcmp(TestSourceDeclaration->Semantic, DestMemberDeclaration->Semantic))
								{
									if (!ArrayTypesMatch(TestSourceDeclaration, DestMemberDeclaration))
									{
										return false;
									}

									SourceMemberDeclarator = TestSourceDeclarator;
									SourceMemberDeclaration = TestSourceDeclaration;
									++SourceIndex;
									break;
								}
							}
						}
					}
				}

				if (SourceMemberDeclarator && SourceMemberDeclaration)
				{
					break;
				}
			}

			if (!SourceMemberDeclarator || !SourceMemberDeclaration)
			{
				//Internal error!!??!!
				check(0);
			}

			FString DestVarMember = *FString::Printf(TEXT("%s.%s"), DestVar, DestMemberDeclaration->Identifier);
			FString SourceVarMember = *FString::Printf(TEXT("%s.%s"), SourceVar, SourceMemberDeclaration->Identifier);

			if (DestMemberDeclaration->bIsArray)
			{
				// Already validated by ArrayTypesMatch
				uint32 ArrayLength = DestMemberDeclaration->ArraySize[0]->AsUnaryExpression()->GetUintConstantValue();
				for (uint32 Index = 0; Index < ArrayLength; ++Index)
				{
					FString DestVarMemberElement = *FString::Printf(TEXT("%s[%d]"), *DestVarMember, Index);
					FString SourceVarMemberElement = *FString::Printf(TEXT("%s[%d]"), *SourceVarMember, Index);
					if (!CopyVariable(Allocator, Body, DestMemberDeclarator->Type->Specifier, *DestVarMemberElement, SourceMemberDeclarator->Type->Specifier, *SourceVarMemberElement))
					{
						return false;
					}
				}
			}
			else if (!CopyVariable(Allocator, Body, DestMemberDeclarator->Type->Specifier, *DestVarMember, SourceMemberDeclarator->Type->Specifier, *SourceVarMember))
			{
				return false;
			}
		}
	}

	return true;
}

static void HlslParserCallback(void* CallbackData, CrossCompiler::FLinearAllocator* Allocator, CrossCompiler::TLinearArray<CrossCompiler::AST::FNode*>& ASTNodes)
{
	using namespace CrossCompiler::AST;

	auto* RemoveUnusedOutputsData = (FRemoveUnusedOutputsData*)CallbackData;

	// Find Entry point
	FFunctionDefinition* EntryFunction = nullptr;
	TArray<FStructSpecifier*> MiniSymbolTable;

	FString TempOut;

	for (int32 Index = 0; Index < ASTNodes.Num(); ++Index)
	{
		auto* Node = ASTNodes[Index];
		if (FDeclaratorList* DeclaratorList = Node->AsDeclaratorList())
		{
			// Skip unnamed structures
			if (DeclaratorList->Type->Specifier->Structure && DeclaratorList->Type->Specifier->Structure->Name)
			{
				MiniSymbolTable.Add(DeclaratorList->Type->Specifier->Structure);
			}
		}
		else if (FFunctionDefinition* FunctionDefinition = Node->AsFunctionDefinition())
		{
			if (FCString::Strcmp(*RemoveUnusedOutputsData->EntryPoint, FunctionDefinition->Prototype->Identifier) == 0)
			{
				EntryFunction = FunctionDefinition;
			}
		}

		FASTWriter Writer(TempOut);
		Node->Write(Writer);
	}

	if (EntryFunction)
	{
		FCompoundStatement* Body = new(Allocator) FCompoundStatement(Allocator, EntryFunction->SourceInfo);

		// Utilities
		auto MakeSimpleType = [&](const TCHAR* Name)
			{
				auto* ReturnType = new(Allocator) FFullySpecifiedType(Allocator, EntryFunction->SourceInfo);
				ReturnType->Specifier = new(Allocator) FTypeSpecifier(Allocator, EntryFunction->SourceInfo);
				ReturnType->Specifier->TypeName = Allocator->Strdup(Name);
				return ReturnType;
			};

		auto MakeIdentifierExpression = [&](const TCHAR* Name)
			{
				return ::MakeIdentifierExpression(Allocator, Name, EntryFunction->SourceInfo);
			};

		auto FindStructSpecifier = [&](const TCHAR* StructName) -> FStructSpecifier*
			{
				for (auto* StructSpecifier : MiniSymbolTable)
				{
					if (!FCString::Strcmp(StructSpecifier->Name, StructName))
					{
						return StructSpecifier;
					}
				}

				return nullptr;
			};

		auto IsStringInArray = [&](const TArray<FString>& Array, const TCHAR* Semantic)
			{
				for (const FString& String : Array)
				{
					if (FCString::Strcmp(*String, Semantic) == 0)
					{
						return true;
					}
				}

				return false;
			};

		auto IsSemanticUsed = [&](const TCHAR* Semantic)
			{
				static bool bLeaveAllUsed = false;
				return bLeaveAllUsed || IsStringInArray(RemoveUnusedOutputsData->UsedOutputs, Semantic) || IsStringInArray(RemoveUnusedOutputsData->SystemOutputs, Semantic);
			};

		TArray<FString> RemovedSemantics;

		// Mine the original input parameters
		TArray<FParameterDeclarator*> OriginalParameters;
		for (FNode* ParamNode : EntryFunction->Prototype->Parameters)
		{
			auto* ParameterDeclarator = ParamNode->AsParameterDeclarator();
			check(ParameterDeclarator);
			OriginalParameters.Add(ParameterDeclarator);
		}

		FDeclaratorList* NewReturnStructDefinition = nullptr;
		FFunctionExpression* OriginalFunctionCall = nullptr;
		FFullySpecifiedType* NewReturnType = nullptr;
		//#todo-rco: Handle the case of returning just SV_POSITION
		auto* OriginalReturnType = EntryFunction->Prototype->ReturnType;
		if (OriginalReturnType && OriginalReturnType->Specifier && OriginalReturnType->Specifier->TypeName && !EntryFunction->Prototype->ReturnSemantic)
		{
			// Make the new optimized type/return struct
			auto* OriginalReturnType = EntryFunction->Prototype->ReturnType->Specifier;
			auto* NewStrucTypeSpecifier = MakeSimpleType(*(OriginalReturnType->TypeName + FString(TEXT("__OPTIMIZED"))));
			NewStrucTypeSpecifier->Specifier->Structure = new(Allocator) FStructSpecifier(Allocator, OriginalReturnType->SourceInfo);
			NewStrucTypeSpecifier->Specifier->Structure->Name = NewStrucTypeSpecifier->Specifier->TypeName;

			// Find original type
			FStructSpecifier* OriginalStructSpecifier = FindStructSpecifier(OriginalReturnType->TypeName);
			if (!OriginalStructSpecifier)
			{
				RemoveUnusedOutputsData->bSuccess = false;
				return;
			}

			// Copy members
			//#todo-rco: Nested structures
			for (auto* MemberNodes : OriginalStructSpecifier->Declarations)
			{
				FDeclaratorList* MemberDeclarator = MemberNodes->AsDeclaratorList();
				check(MemberDeclarator);
				for (auto* DeclarationNode : MemberDeclarator->Declarations)
				{
					FDeclaration* MemberDeclaration = DeclarationNode->AsDeclaration();
					check(MemberDeclaration);
					if (MemberDeclaration->Semantic)
					{
						if (IsSemanticUsed(MemberDeclaration->Semantic))
						{
							auto* NewDeclaratorList = new(Allocator) FDeclaratorList(Allocator, MemberDeclarator->SourceInfo);
							NewDeclaratorList->Type = MemberDeclarator->Type;
							NewDeclaratorList->Declarations.Add(MemberDeclaration);
							NewStrucTypeSpecifier->Specifier->Structure->Declarations.Add(NewDeclaratorList);
						}
						else
						{
							RemovedSemantics.Add(MemberDeclaration->Semantic);
						}
					}
					else
					{
						// No semantic, so make sure this is a nested struct
						FStructSpecifier* NestedStructSpecifier = FindStructSpecifier(MemberDeclaration->Identifier);
						check(NestedStructSpecifier);

						//#todo-rco: Handle nested structures & n-nested structures
						check(0);
					}
				}
			}

			NewReturnStructDefinition = new(Allocator) FDeclaratorList(Allocator, OriginalReturnType->SourceInfo);
			NewReturnStructDefinition->Type = NewStrucTypeSpecifier;

			// Declare variable for the return value of calling the original entry point
			auto* LocalVarDeclaratorList = new(Allocator) FDeclaratorList(Allocator, EntryFunction->SourceInfo);
			LocalVarDeclaratorList->Type = EntryFunction->Prototype->ReturnType;
			auto* LocalVarDeclaration = new(Allocator) FDeclaration(Allocator, EntryFunction->SourceInfo);
			LocalVarDeclaration->Identifier = TEXT("Out__ORIGINAL");
			LocalVarDeclaratorList->Declarations.Add(LocalVarDeclaration);
			Body->Statements.Add(LocalVarDeclaratorList);

			// Call original
			OriginalFunctionCall = new(Allocator) FFunctionExpression(Allocator, EntryFunction->SourceInfo, MakeIdentifierExpression(*RemoveUnusedOutputsData->EntryPoint));
			auto* LHS = MakeIdentifierExpression(LocalVarDeclaration->Identifier);
			auto* Assignment = new(Allocator) FBinaryExpression(Allocator, EOperators::Assign, LHS, OriginalFunctionCall, EntryFunction->SourceInfo);
			Body->Statements.Add(new(Allocator) FExpressionStatement(Allocator, Assignment, EntryFunction->SourceInfo));

			// Declare variable for the new return type
			auto* OptimizedVarDeclaratorList = new(Allocator)FDeclaratorList(Allocator, EntryFunction->SourceInfo);
			OptimizedVarDeclaratorList->Type = MakeSimpleType(NewStrucTypeSpecifier->Specifier->TypeName);
			auto* OptimizedVarDeclaration = new(Allocator)FDeclaration(Allocator, EntryFunction->SourceInfo);
			OptimizedVarDeclaration->Identifier = TEXT("Out__OPTIMIZED");
			OptimizedVarDeclaratorList->Declarations.Add(OptimizedVarDeclaration);
			Body->Statements.Add(OptimizedVarDeclaratorList);

			// Copy members
			if (!CopyStructMembers(Allocator, Body, NewStrucTypeSpecifier->Specifier->Structure, OptimizedVarDeclaration->Identifier, OriginalStructSpecifier, LocalVarDeclaration->Identifier))
			{
				RemoveUnusedOutputsData->bSuccess = false;
				return;
			}

			auto* ReturnStatement = new(Allocator) FJumpStatement(Allocator, EJumpType::Return, OriginalReturnType->SourceInfo);
			ReturnStatement->OptionalExpression = MakeIdentifierExpression(OptimizedVarDeclaration->Identifier);
			Body->Statements.Add(ReturnStatement);

			NewReturnType = MakeSimpleType(NewStrucTypeSpecifier->Specifier->TypeName);
		}
		else
		{
			// Call original
			OriginalFunctionCall = new(Allocator) FFunctionExpression(Allocator, EntryFunction->SourceInfo, MakeIdentifierExpression(*RemoveUnusedOutputsData->EntryPoint));
			Body->Statements.Add(new(Allocator) FExpressionStatement(Allocator, OriginalFunctionCall, EntryFunction->SourceInfo));

			NewReturnType = MakeSimpleType(TEXT("void"));
		}

		// Add the parameters for calling the original
		for (FParameterDeclarator* Parameter : OriginalParameters)
		{
			OriginalFunctionCall->Expressions.Add(MakeIdentifierExpression(Parameter->Identifier));
		}

		// New Entry definition/prototype
		FFunctionDefinition* NewEntryFunction = new(Allocator) FFunctionDefinition(Allocator, EntryFunction->SourceInfo);
		NewEntryFunction->Prototype = new(Allocator) FFunction(Allocator, EntryFunction->SourceInfo);
		NewEntryFunction->Prototype->Identifier = Allocator->Strdup(*(RemoveUnusedOutputsData->EntryPoint + TEXT("__OPTIMIZED")));
		NewEntryFunction->Prototype->ReturnType = NewReturnType;
		NewEntryFunction->Prototype->Parameters = EntryFunction->Prototype->Parameters;
		NewEntryFunction->Body = Body;

		FASTWriter Writer(RemoveUnusedOutputsData->GeneratedCode);
		RemoveUnusedOutputsData->GeneratedCode = TEXT("#line 1 \"RemoveUnusedOutputs.usf\"\n// Generated Entry Point: \n");
		if (RemovedSemantics.Num() > 0)
		{
			RemoveUnusedOutputsData->GeneratedCode = TEXT("// Removed Outputs: ");
			for (int32 Index = 0; Index < RemovedSemantics.Num(); ++Index)
			{
				RemoveUnusedOutputsData->GeneratedCode += RemovedSemantics[Index];
				RemoveUnusedOutputsData->GeneratedCode += TEXT(" ");
			}
			RemoveUnusedOutputsData->GeneratedCode += TEXT("\n");
		}
		if (NewReturnStructDefinition)
		{
			NewReturnStructDefinition->Write(Writer);
		}
		NewEntryFunction->Write(Writer);
//FPlatformMisc::LowLevelOutputDebugStringf(TEXT("*********************************\n%s\n"), *RemoveUnusedOutputsData->GeneratedCode);
		RemoveUnusedOutputsData->bSuccess = true;
	}
}

bool RemoveUnusedOutputs(FString& SourceCode, const TArray<FString>& SystemOutputs, const TArray<FString>& UsedOutputs, const FString& EntryPoint)
{
	FString DummyFilename(TEXT("RemoveUnusedOutputs.usf"));
	FRemoveUnusedOutputsData Data(SystemOutputs, UsedOutputs);
	Data.EntryPoint = EntryPoint;
	if (!CrossCompiler::Parser::Parse(SourceCode, DummyFilename, HlslParserCallback, &Data))
	{
		return false;
	}

	if (Data.bSuccess)
	{
		SourceCode += '\n';
		SourceCode += Data.GeneratedCode;

		return true;
	}

	return false;
}
