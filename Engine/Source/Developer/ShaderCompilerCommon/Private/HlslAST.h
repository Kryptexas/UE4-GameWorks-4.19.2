// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	HlslAST.h - Abstract Syntax Tree interfaces for HLSL.
=============================================================================*/

#pragma once

#include "HlslLexer.h"

namespace CrossCompiler
{
	namespace AST
	{
		class FNode
		{
		public:
			FSourceInfo SourceInfo;

		protected:
			FNode();
			FNode(const FSourceInfo& InInfo);
		};

		/**
		* Operators for AST expression nodes.
		*/
		enum class EOperators
		{
			Assign,
			Plus,        /**< Unary + operator. */
			Neg,
			Add,
			Sub,
			Mul,
			Div,
			Mod,
			LShift,
			RShift,
			Less,
			Greater,
			LEqual,
			GEqual,
			Equal,
			NEqual,
			BitAnd,
			BitXor,
			BitOr,
			BitNot,
			LogicAnd,
			LogicXor,
			LogicOr,
			LogicNot,

			MulAssign,
			DivAssign,
			ModAssign,
			AddAssign,
			SubAssign,
			LSAssign,
			RSAssign,
			AndAssign,
			XorAssign,
			OrAssign,

			Conditional,

			PreInc,
			PreDec,
			//Post_inc,
			//Post_dec,
			//Field_selection,
			//Array_index,

			//Function_call,
			//Initializer_list,

			Identifier,
			//Int_constant,
			UintConstant,
			FloatConstant,
			BoolConstant,

			//Sequence,

			//Type_cast,
		};

		inline EOperators TokenToASTOperator(EHlslToken Token)
		{
			switch (Token)
			{
			case EHlslToken::Equal:
				return AST::EOperators::Assign;

			case EHlslToken::PlusEqual:
				return AST::EOperators::AddAssign;

			case EHlslToken::MinusEqual:
				return AST::EOperators::SubAssign;

			case EHlslToken::TimesEqual:
				return AST::EOperators::MulAssign;

			case EHlslToken::DivEqual:
				return AST::EOperators::DivAssign;

			case EHlslToken::ModEqual:
				return AST::EOperators::ModAssign;

			case EHlslToken::GreaterGreaterEqual:
				return AST::EOperators::RSAssign;

			case EHlslToken::LowerLowerEqual:
				return AST::EOperators::LSAssign;

			case EHlslToken::AndEqual:
				return AST::EOperators::AndAssign;

			case EHlslToken::OrEqual:
				return AST::EOperators::OrAssign;

			case EHlslToken::XorEqual:
				return AST::EOperators::XorAssign;

			case EHlslToken::Question:
				return AST::EOperators::Conditional;

			case EHlslToken::OrOr:
				return AST::EOperators::LogicOr;

			case EHlslToken::AndAnd:
				return AST::EOperators::LogicAnd;

			case EHlslToken::Or:
				return AST::EOperators::BitOr;

			case EHlslToken::Xor:
				return AST::EOperators::BitXor;

			case EHlslToken::And:
				return AST::EOperators::BitAnd;

			case EHlslToken::EqualEqual:
				return AST::EOperators::Equal;

			case EHlslToken::NotEqual:
				return AST::EOperators::NEqual;

			case EHlslToken::Lower:
				return AST::EOperators::Less;

			case EHlslToken::Greater:
				return AST::EOperators::Greater;

			case EHlslToken::LowerEqual:
				return AST::EOperators::LEqual;

			case EHlslToken::GreaterEqual:
				return AST::EOperators::GEqual;

			case EHlslToken::LowerLower:
				return AST::EOperators::LShift;

			case EHlslToken::GreaterGreater:
				return AST::EOperators::RShift;

			case EHlslToken::Plus:
				return AST::EOperators::Add;

			case EHlslToken::Minus:
				return AST::EOperators::Sub;

			case EHlslToken::Times:
				return AST::EOperators::Mul;

			case EHlslToken::Div:
				return AST::EOperators::Div;

			case EHlslToken::Mod:
				return AST::EOperators::Mod;

			default:
				check(0);
				break;
			}

			return AST::EOperators::Plus;
		}

		struct FExpression : public FNode
		{
			FExpression(EOperators InOperator, FExpression* E0, FExpression* E1, FExpression* E2, const FSourceInfo& InInfo);

			EOperators Operator;
			FExpression* SubExpressions[3];

			union
			{
				uint32 UintConstant;
				float FloatConstant;
				bool BoolConstant;
			};
			FString Identifier;
		};

		struct FUnaryExpression : public FExpression
		{
			FUnaryExpression(EOperators InOperator, FExpression* Expr, const FSourceInfo& InInfo);
		};

		struct FBinaryExpression : public FExpression
		{
			FBinaryExpression(EOperators InOperator, FExpression* E0, FExpression* E1, const FSourceInfo& InInfo);
		};
/*
		class FFunctionExpression : public FExpression
		{
		public:
		private:
		};

		class FInitializerListExpression : public FExpression
		{
		public:
		private:
		};

		class FCompoundStatement : public FNode
		{
		public:
		private:
		};

		class FDeclaration : public FNode
		{
		public:
		private:
		};

		class FStructSpecifier : public FNode
		{
		public:
		private:
		};

		class FCBufferDeclaration : public FNode
		{
		public:
		private:
		};

		class FTypeSpecifier : public FNode
		{
		public:
		private:
		};

		class FFullySpecifiedType : public FNode
		{
		public:
		private:
		};

		class FDeclaratorList : public FNode
		{
		public:
		private:
		};

		class FParameterDeclarator : public FNode
		{
		public:
		private:
		};

		class FFunction : public FNode
		{
		public:
		private:
		};

		class FExpressionStatement : public FNode
		{
		public:
		private:
		};

		class FCaseLabel : public FNode
		{
		public:
		private:
		};

		class FCaseLabelList : public FNode
		{
		public:
		private:
		};

		class FCaseLabelStatement : public FNode
		{
		public:
		private:
		};

		class FCaseLabelStatementList : public FNode
		{
		public:
		private:
		};

		class FSwitchBody : public FNode
		{
		public:
		private:
		};

		class FSelectionStatement : public FNode
		{
		public:
		private:
		};

		class FSwitchStatement : public FNode
		{
		public:
		private:
		};

		class FIterationStatement : public FNode
		{
		public:
		private:
		};

		class FJumpStatement : public FNode
		{
		public:
		private:
		};

		class FFunctionDefinition : public FNode
		{
		public:
		private:
		};

		class FAttributeArgument : public FNode
		{
		public:
		private:
		};

		class FAttribute : public FNode
		{
		public:
		private:
		};
*/
	/*
		union UASTUnion
		{
			int n;
			float real;
			const char *identifier;
			const char *string_literal;

			struct ast_type_qualifier type_qualifier;

			ast_node *node;
			ast_type_specifier *type_specifier;
			ast_fully_specified_type *fully_specified_type;
			ast_function *function;
			ast_parameter_declarator *parameter_declarator;
			ast_function_definition *function_definition;
			ast_compound_statement *compound_statement;
			*/

/*
			FExpression* Expression;
*/
/*
			ast_declarator_list *declarator_list;
			ast_struct_specifier *struct_specifier;
			ast_declaration *declaration;
			ast_switch_body *switch_body;
			ast_case_label *case_label;
			ast_case_label_list *case_label_list;
			ast_case_statement *case_statement;
			ast_case_statement_list *case_statement_list;

			struct
			{
				ast_node *cond;
				ast_expression *rest;
			} for_rest_statement;

			struct
			{
				ast_node *then_statement;
				ast_node *else_statement;
			} selection_rest_statement;

			ast_attribute* attribute;
			ast_attribute_arg* attribute_arg;
		};
	*/
	}
}
