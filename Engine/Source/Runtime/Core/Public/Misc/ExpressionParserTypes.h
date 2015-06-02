
#pragma once

#include "ValueOrError.h"

class FExpressionNode;
struct FExpressionError;

typedef TValueOrError<FExpressionNode, FExpressionError> FExpressionResult;

/** Simple error structure used for reporting parse errors */
struct FExpressionError
{
	FExpressionError(const FText& InText) : Text(InText) {}

	FText Text;
};

/** Simple struct that defines a specific token contained in an FTokenStream  */
class FStringToken
{
public:
	FStringToken() : TokenStart(nullptr), TokenEnd(nullptr), LineNumber(0), CharacterIndex(0) {}

	/** Get the string representation of this token */
	FString GetString() const { return FString(TokenEnd - TokenStart, TokenStart); }

	/** Check if this token is valid */
	bool IsValid() const { return TokenEnd != TokenStart; }

	/** Get the position of the start and end of this token in the stream */
	const TCHAR* GetTokenStartPos() const { return TokenStart; }
	const TCHAR* GetTokenEndPos() const { return TokenEnd; }

	/** Contextual information about this token */
	int32 GetCharacterIndex() const { return CharacterIndex; }
	int32 GetLineNumber() const { return LineNumber; }
	
	/** Accumulate another token into this one */
	void Accumulate(const FStringToken& InToken) { if (InToken.TokenEnd > TokenEnd) { TokenEnd = InToken.TokenEnd; } }

protected:
	friend class FTokenStream;

	FStringToken(const TCHAR* InStart, int32 Line = 0, int32 Character = 0)
		: TokenStart(InStart), TokenEnd(InStart), LineNumber(Line), CharacterIndex(Character)
	{}

	/** The start of the token */
	const TCHAR* TokenStart;
	/** The end of the token */
	const TCHAR* TokenEnd;
	/** Line number and Character index */
	int32 LineNumber, CharacterIndex;
};

/** Enum specifying how to treat the currently parsing character. */
enum class EParseState
{
	/** Include this character in the token and continue consuming */
	Continue,
	/** Include this character in the token and stop consuming */
	StopAfter,
	/** Exclude this character from the token and stop consuming */
	StopBefore, 
	/** Cancel parsing this token, and return nothing. */
	Cancel,
};

/** A token stream wraps up a raw string, providing accessors into it for consuming tokens */
class CORE_API FTokenStream
{
public:

	/**
	 * Parse out a token using the supplied predicate.
	 * Will keep consuming characters into the resulting token provided the predicate returns EParseState::Continue or EParseState::StopAfter.
	 * Optionally supply a token to accumulate into
	 * Returns a string token for the stream, or empty on error
	 */
	TOptional<FStringToken> ParseToken(const TFunctionRef<EParseState(TCHAR)>& Pred, FStringToken* Accumulate = nullptr) const;

	/** Attempt parse out the specified pre-defined string from the current read position (or accumulating into the specified existing token) */
	TOptional<FStringToken> ParseToken(const TCHAR* Symbol, FStringToken* Accumulate = nullptr) const;
	TOptional<FStringToken> ParseTokenIgnoreCase(const TCHAR* Symbol, FStringToken* Accumulate = nullptr) const;

	/** Return a string token for the next character in the stream (or accumulating into the specified existing token) */
	TOptional<FStringToken> ParseSymbol(FStringToken* Accumulate = nullptr) const;

	/** Attempt parse out the specified pre-defined string from the current read position (or accumulating into the specified existing token) */
	TOptional<FStringToken> ParseSymbol(TCHAR Symbol, FStringToken* Accumulate = nullptr) const;

	/** Parse a whitespace token */
	TOptional<FStringToken> ParseWhitespace(FStringToken* Accumulate = nullptr) const;

	/** Generate a token for the specified number of chars, at the current read position (or end of Accumulate) */
	TOptional<FStringToken> GenerateToken(int32 NumChars, FStringToken* Accumulate = nullptr) const;

public:

	/** Constructor. The stream is only valid for the lifetime of the string provided */
	FTokenStream(const TCHAR* In);
	
	/** Peek at the character at the specified offset from the current read position */
	TCHAR PeekChar(int32 Offset = 0) const;

	/** Get the number of characters remaining in the stream after the current read position */
	int32 CharsRemaining() const;

	/** Check if it is valid to read (the optional number of characters) from the specified position */
	bool IsReadPosValid(const TCHAR* InPos, int32 MinNumChars = 1) const;

	/** Check if the stream is empty */
	bool IsEmpty() const;

	/** Get the current read position from the start of the stream */
	int32 GetPosition() const;

	const TCHAR* GetStart() const { return Start; }
	const TCHAR* GetRead() const { return ReadPos; }
	const TCHAR* GetEnd() const { return End; }

	/** Get the error context from the current read position */
	FString GetErrorContext() const;

	/** Set the current read position to the character proceeding the specified token */
	void SetReadPos(const FStringToken& Token);

private:

	/** The start of the expression */
	const TCHAR* Start;
	/** The end of the expression */
	const TCHAR* End;
	/** The current read position in the expression */
	const TCHAR* ReadPos;
};

/** Helper macro to define the necessary template specialization for a particular expression node type */
/** Variable length arguments are passed the FGuid constructor. Must be unique per type */
#define DEFINE_EXPRESSION_NODE_TYPE(TYPE, ...) \
template<> struct TGetExpressionNodeTypeId<TYPE>\
{\
	static const FGuid& GetTypeId()\
	{\
		static FGuid Global(__VA_ARGS__);\
		return Global;\
	}\
};
template<typename T> struct TGetExpressionNodeTypeId;

/**
 * A node in an expression.
 * 	Can be constructed from any C++ type that has a corresponding DEFINE_EXPRESSION_NODE_TYPE.
 * 	Evaluation behaviour (unary/binary operator etc) is defined in the expression grammar, rather than the type itself.
 */
class FExpressionNode
{
public:
	/** Construction from client expression data type */
	template<typename T>
	FExpressionNode(const T& In,
		/** @todo: make this a default function template parameter when VS2012 support goes */
		typename TEnableIf<!TPointerIsConvertibleFromTo<T, FExpressionNode>::Value>::Type* = nullptr
		)
		: TypeId(TGetExpressionNodeTypeId<T>::GetTypeId())
	{
		if (sizeof(T) <= SizeOfInlineBytes)
		{
			FMemory::Memcpy(InlineBytes, &In, sizeof(T));
		}
		else
		{
			BigData = MakeShareable(new T(In));
		}
	}

	/** Copy construction/assignment */
	FExpressionNode(const FExpressionNode& In) { *this = In; }
	FExpressionNode& operator=(const FExpressionNode& In)
	{
		TypeId = In.TypeId;
		if (In.BigData.IsValid())
		{
			BigData = In.BigData;
		}
		else
		{
			BigData = nullptr;
			FMemory::Memcpy(InlineBytes, In.InlineBytes, SizeOfInlineBytes);
		}
		return *this;
	}

	/** Get the type identifier of this node */
	const FGuid& GetTypeId() const { return TypeId; }

	/** Cast this node to the specified type. Will asset if the types do not match. */
	template<typename T>
	const T* Cast() const
	{
		if (TypeId == TGetExpressionNodeTypeId<T>::GetTypeId())
		{
			if (BigData.IsValid())
			{
				return reinterpret_cast<const T*>(BigData.Get());
			}
			else
			{
				return reinterpret_cast<const T*>(InlineBytes);
			}
		}
		return nullptr;
	}

private:

	static const int32 SizeOfInlineBytes = 128 - sizeof(FGuid) - sizeof(TSharedPtr<void>);

	/** TypeID - 16 bytes */
	FGuid TypeId;
	/** SharedPtr for large types - 2*sizeof(void*) */
	TSharedPtr<void> BigData;
	uint8 InlineBytes[SizeOfInlineBytes];
};

/** A specific token in a stream. Comprises an expression node, and the stream token it was created from */
class FExpressionToken
{
public:
	FExpressionToken(const FStringToken& InContext, const FExpressionNode& InNode)
		: Node(InNode)
		, Context(InContext)
	{
	}

	FExpressionToken(const FExpressionToken& In) : Node(In.Node), Context(In.Context) {}
	FExpressionToken& operator=(const FExpressionToken& In) { Node = In.Node; Context = In.Context; return *this; }

	FExpressionNode Node;
	FStringToken Context;
};

/** Built-in wrapper tokens used for compilation */
struct FGroupMarker
{
	FGroupMarker(const FExpressionToken& InWrappedToken) : WrappedToken(InWrappedToken), NumTokens(0) {}

	template<typename T>	bool 		IsA() const 	{ return WrappedToken.Node.GetTypeId() == TGetExpressionNodeTypeId<T>::GetTypeId(); }
	template<typename T>	const T* 	Cast() const	{ return WrappedToken.Node.Cast<T>(); }

	FExpressionToken WrappedToken;
	int32 NumTokens;
};
DEFINE_EXPRESSION_NODE_TYPE(FGroupMarker, 0xEBF6DBA6, 0xE81B4684, 0x938EF46A, 0xD2FC0052)

struct FWrappedOperator
{
	enum EType { PreUnary, PostUnary, Binary };
	FWrappedOperator(EType InType, const FExpressionToken& InWrappedToken, int32 InPrec = 0) : Type(InType), WrappedToken(InWrappedToken), Precedence(InPrec) {}

	template<typename T>	bool 		IsA() const 	{ return WrappedToken.Node.GetTypeId() == TGetExpressionNodeTypeId<T>::GetTypeId(); }
	template<typename T>	const T* 	Cast() const	{ return WrappedToken.Node.Cast<T>(); }

	EType Type;
	FExpressionToken WrappedToken;
	int32 Precedence;
};
DEFINE_EXPRESSION_NODE_TYPE(FWrappedOperator, 0x449B352B, 0x17B14A76, 0xBD264E7B, 0x9669D8F4)

namespace impl
{
	template <typename> struct TParams;
	template <typename> struct TParamsImpl;

	template <typename Ret_, typename T, typename Arg1_>
	struct TParamsImpl<Ret_ (T::*)(Arg1_)> { typedef Ret_ Ret; typedef Arg1_ Arg1; };
	template <typename Ret_, typename T, typename Arg1_>
	struct TParamsImpl<Ret_ (T::*)(Arg1_) const> { typedef Ret_ Ret; typedef Arg1_ Arg1; };

	template <typename Ret_, typename T, typename Arg1_, typename Arg2_>
	struct TParamsImpl<Ret_ (T::*)(Arg1_, Arg2_)> { typedef Ret_ Ret; typedef Arg1_ Arg1; typedef Arg2_ Arg2; };
	template <typename Ret_, typename T, typename Arg1_, typename Arg2_>
	struct TParamsImpl<Ret_ (T::*)(Arg1_, Arg2_) const> { typedef Ret_ Ret; typedef Arg1_ Arg1; typedef Arg2_ Arg2; };

	/** @todo: decltype(&T::operator()) can go directly in the specialization below if it weren't for VS2012 support */
	template<typename T>
	struct TGetOperatorCallPtr { typedef decltype(&T::operator()) Type; };

	template <typename T>
	struct TParams : TParamsImpl<typename TGetOperatorCallPtr<T>::Type> {};
}

/** Jump table specifying how to execute an operator with different types */
struct FOperatorJumpTable
{
	/** Execute the specified token as a unary operator, if such an overload exists */
	FExpressionResult ExecPreUnary(const FExpressionToken& Operator, const FExpressionToken& R) const;
	/** Execute the specified token as a unary operator, if such an overload exists */
	FExpressionResult ExecPostUnary(const FExpressionToken& Operator, const FExpressionToken& L) const;
	/** Execute the specified token as a binary operator, if such an overload exists */
	FExpressionResult ExecBinary(const FExpressionToken& Operator, const FExpressionToken& L, const FExpressionToken& R) const;

	/**
	 * Map an expression node to a unary operator with the specified implementation.
	 * This overload accepts callable types that return an FExpressionResult (and thus support returning an error)
	 * Example usage that binds a '!' token to a function that attempts to do a boolean 'not'
	 *		JumpTable.MapUnary<FExclamation>([](bool A) -> FExpressionResult { return MakeValue(!A); });
	 */
	template<typename OperatorType, typename FuncType>
	typename TEnableIf<TIsSame<FExpressionResult, typename impl::TParams<FuncType>::Ret>::Value>::Type
		MapPreUnary(FuncType InFunc)
	{
		typedef typename TRemoveConst<typename TRemoveReference<typename impl::TParams<FuncType>::Arg1>::Type>::Type OperandType;

		FOperatorFunctionID ID = {
			TGetExpressionNodeTypeId<OperatorType>::GetTypeId(),
			FGuid(),
			TGetExpressionNodeTypeId<OperandType>::GetTypeId()
		};

		PreUnaryOps.Add(ID, [=](const FExpressionNode& InOperand) {
			return InFunc(*InOperand.Cast<OperandType>());
		});
	}

	/**
	 * Map an expression node to a unary operator with the specified implementation.
	 * This overload accepts callable types that return anything other than an FExpressionResult. Such functions cannot return errors.
	 * The returned type is passed into an FExpressionNode, and as such must be a valid expression node type (see DEFINE_EXPRESSION_NODE_TYPE).
	 * Example usage that binds a 'subtract' token to a function that negates the integer:
	 *		JumpTable.MapUnary<FSubtractToken>([](int32 A){ return -A; });
	 */
	template<typename OperatorType, typename FuncType>
	typename TEnableIf<!TIsSame<FExpressionResult, typename impl::TParams<FuncType>::Ret>::Value>::Type
		MapPreUnary(FuncType InFunc)
	{
		typedef typename TRemoveConst<typename TRemoveReference<typename impl::TParams<FuncType>::Arg1>::Type>::Type OperandType;

		FOperatorFunctionID ID = {
			TGetExpressionNodeTypeId<OperatorType>::GetTypeId(),
			FGuid(),
			TGetExpressionNodeTypeId<OperandType>::GetTypeId()
		};

		// Explicit return type is important here, to ensure that the proxy returned from MakeValue does not outlive the value it's proxying
		PreUnaryOps.Add(ID, [=](const FExpressionNode& InOperand) -> FExpressionResult {
			return MakeValue(InFunc(*InOperand.Cast<OperandType>()));
		});
	}

	/**
	 * Map an expression node to a unary operator with the specified implementation.
	 * This overload accepts callable types that return an FExpressionResult (and thus support returning an error)
	 * Example usage that binds a '!' token to a function that attempts to do a boolean 'not'
	 *		JumpTable.MapUnary<FExclamation>([](bool A) -> FExpressionResult { return MakeValue(!A); });
	 */
	template<typename OperatorType, typename FuncType>
	typename TEnableIf<TIsSame<FExpressionResult, typename impl::TParams<FuncType>::Ret>::Value>::Type
		MapPostUnary(FuncType InFunc)
	{
		typedef typename TRemoveConst<typename TRemoveReference<typename impl::TParams<FuncType>::Arg1>::Type>::Type OperandType;

		FOperatorFunctionID ID = {
			TGetExpressionNodeTypeId<OperatorType>::GetTypeId(),
			TGetExpressionNodeTypeId<OperandType>::GetTypeId(),
			FGuid()
		};

		PostUnaryOps.Add(ID, [=](const FExpressionNode& InOperand) {
			return InFunc(*InOperand.Cast<OperandType>());
		});
	}

	/**
	 * Map an expression node to a unary operator with the specified implementation.
	 * This overload accepts callable types that return anything other than an FExpressionResult. Such functions cannot return errors.
	 * The returned type is passed into an FExpressionNode, and as such must be a valid expression node type (see DEFINE_EXPRESSION_NODE_TYPE).
	 * Example usage that binds a 'subtract' token to a function that negates the integer:
	 *		JumpTable.MapUnary<FSubtractToken>([](int32 A){ return -A; });
	 */
	template<typename OperatorType, typename FuncType>
	typename TEnableIf<!TIsSame<FExpressionResult, typename impl::TParams<FuncType>::Ret>::Value>::Type
		MapPostUnary(FuncType InFunc)
	{
		typedef typename TRemoveConst<typename TRemoveReference<typename impl::TParams<FuncType>::Arg1>::Type>::Type OperandType;

		FOperatorFunctionID ID = {
			TGetExpressionNodeTypeId<OperatorType>::GetTypeId(),
			TGetExpressionNodeTypeId<OperandType>::GetTypeId(),
			FGuid()
		};

		// Explicit return type is important here, to ensure that the proxy returned from MakeValue does not outlive the value it's proxying
		PostUnaryOps.Add(ID, [=](const FExpressionNode& InOperand) -> FExpressionResult {
			return MakeValue(InFunc(*InOperand.Cast<OperandType>()));
		});
	}

	/**
	 * Map an expression node to a unary operator with the specified implementation.
	 * This overload accepts callable types that return an FExpressionResult (and thus support returning an error)
	 * Example usage that binds a 'slash' token to a function that subtracts two integers:
	 *		JumpTable.MapBinary<FSlashToken>([](double A, double B){
	 			if (B == 0) {
	 				return MakeError(LOCTEXT("DivisionByZero", "Division by zero"));
	 			} else {
	 				return MakeValue(A / B);
	 			}
	 		});
	 */
	template<typename OperatorType, typename FuncType>
	typename TEnableIf<TIsSame<FExpressionResult, typename impl::TParams<FuncType>::Ret>::Value>::Type
		MapBinary(FuncType InFunc)
	{
		typedef typename TRemoveConst<typename TRemoveReference<typename impl::TParams<FuncType>::Arg1>::Type>::Type LeftOperandType;
		typedef typename TRemoveConst<typename TRemoveReference<typename impl::TParams<FuncType>::Arg2>::Type>::Type RightOperandType;

		FOperatorFunctionID ID = {
			TGetExpressionNodeTypeId<OperatorType>::GetTypeId(),
			TGetExpressionNodeTypeId<LeftOperandType>::GetTypeId(),
			TGetExpressionNodeTypeId<RightOperandType>::GetTypeId()
		};

		BinaryOps.Add(ID, [=](const FExpressionNode& InLeftOperand, const FExpressionNode& InRightOperand){
			return InFunc(*InLeftOperand.Cast<LeftOperandType>(), *InRightOperand.Cast<RightOperandType>());
		});
	}

	/**
	 * Map an expression node to a binary operator with the specified implementation.
	 * This overload accepts callable types that return anything other than an FExpressionResult. Such functions cannot return errors.
	 * The returned type is passed into an FExpressionNode, and as such must be a valid expression node type (see DEFINE_EXPRESSION_NODE_TYPE).
	 * Example usage that binds a 'subtract' token to a function that subtracts two integers:
	 *		JumpTable.MapBinary<FSubtractToken>([](int32 A, int32 B){ return A - B; });
	 */
	template<typename OperatorType, typename FuncType>
	typename TEnableIf<!TIsSame<FExpressionResult, typename impl::TParams<FuncType>::Ret>::Value>::Type
		MapBinary(FuncType InFunc)
	{
		typedef typename TRemoveConst<typename TRemoveReference<typename impl::TParams<FuncType>::Arg1>::Type>::Type LeftOperandType;
		typedef typename TRemoveConst<typename TRemoveReference<typename impl::TParams<FuncType>::Arg2>::Type>::Type RightOperandType;

		FOperatorFunctionID ID = {
			TGetExpressionNodeTypeId<OperatorType>::GetTypeId(),
			TGetExpressionNodeTypeId<LeftOperandType>::GetTypeId(),
			TGetExpressionNodeTypeId<RightOperandType>::GetTypeId()
		};

		// Explicit return type is important here, to ensure that the proxy returned from MakeValue does not outlive the value it's proxying
		BinaryOps.Add(ID, [=](const FExpressionNode& InLeftOperand, const FExpressionNode& InRightOperand) -> FExpressionResult {
			return MakeValue(InFunc(*InLeftOperand.Cast<LeftOperandType>(), *InRightOperand.Cast<RightOperandType>()));
		});
	}

private:
	/** Struct used to identify a function for a specific operator overload */
	struct FOperatorFunctionID
	{
		FGuid OperatorType;
		FGuid LeftOperandType;
		FGuid RightOperandType;

		friend bool operator==(const FOperatorFunctionID& A, const FOperatorFunctionID& B)
		{
			return A.OperatorType == B.OperatorType &&
				A.LeftOperandType == B.LeftOperandType &&
				A.RightOperandType == B.RightOperandType;
		}

		friend uint32 GetTypeHash(const FOperatorFunctionID& In)
		{
			const uint32 Hash = HashCombine(GetTypeHash(In.OperatorType), GetTypeHash(In.LeftOperandType));
			return HashCombine(GetTypeHash(In.RightOperandType), Hash);
		}
	};

	/** Maps of unary/binary operators */
	TMap<FOperatorFunctionID, TFunction<FExpressionResult(const FExpressionNode&)>> PreUnaryOps;
	TMap<FOperatorFunctionID, TFunction<FExpressionResult(const FExpressionNode&)>> PostUnaryOps;
	TMap<FOperatorFunctionID, TFunction<FExpressionResult(const FExpressionNode&, const FExpressionNode&)>> BinaryOps;
};

/** Class used to consume tokens from a string */
class CORE_API FExpressionTokenConsumer
{
public:
	/** Construction from a raw string. The consumer is only valid as long as the string is valid */
	FExpressionTokenConsumer(const TCHAR* InExpression);

	/** Extract the list of tokens from this consumer */
	TArray<FExpressionToken> Extract();

	/** Add an expression node to the consumer, specifying the FStringToken this node relates to.
	 *	Adding a node to the consumer will move its stream read position to the end of the added token.
	 */
	void Add(const FStringToken& SourceToken, FExpressionNode Node);

	/** Get the expression stream */
	FTokenStream& GetStream() { return Stream; }

private:
	FExpressionTokenConsumer(const FExpressionTokenConsumer&);
	FExpressionTokenConsumer& operator=(const FExpressionTokenConsumer&);

	/** Array of added tokens */
	TArray<FExpressionToken> Tokens;

	/** Stream that looks at the constructed expression */
	FTokenStream Stream;
};

/** 
 * Typedef that defines a function used to consume tokens
 * 	Definitions may add FExpressionNodes parsed from the provided consumer's stream, or return an optional error.
 *	Where a definition performs no mutable operations, subsequent token definitions will be invoked.
 */
typedef TOptional<FExpressionError> (FExpressionDefinition)(FExpressionTokenConsumer&);


/** A lexeme dictionary defining how to lex an expression. */
class CORE_API FTokenDefinitions
{
public:
	FTokenDefinitions() : bIgnoreWhitespace(false) {}

	/** Define the grammer to ignore whitespace between tokens, unless explicitly included in a token */
	void IgnoreWhitespace() { bIgnoreWhitespace = true; }

	/** Define a token by way of a function to be invoked to attempt to parse a token from a stream */
	void DefineToken(const TFunction<FExpressionDefinition>& Definition);

public:

	/** Check if the grammar ignores whitespace */
	bool DoesIgnoreWhitespace() { return bIgnoreWhitespace; }

	/** Consume a token for the specified consumer */
	TOptional<FExpressionError> ConsumeTokens(FExpressionTokenConsumer& Consumer) const;

private:

	TOptional<FExpressionError> ConsumeToken(FExpressionTokenConsumer& Consumer) const;

private:

	bool bIgnoreWhitespace;
	TArray<TFunction<FExpressionDefinition>> Definitions;
};

/** A lexical gammer defining how to parse an expression. Clients must define the tokens and operators to be interpreted by the parser. */
class CORE_API FExpressionGrammar
{
public:
	/** Define a grouping operator from two expression node types */
	template<typename TStartGroup, typename TEndGroup>
	void DefineGrouping() { Groupings.Add(TGetExpressionNodeTypeId<TStartGroup>::GetTypeId(), TGetExpressionNodeTypeId<TEndGroup>::GetTypeId()); }

	/** Define a pre-unary operator for the specified symbol */
	template<typename TExpressionNode>
	void DefinePreUnaryOperator() { PreUnaryOperators.Add(TGetExpressionNodeTypeId<TExpressionNode>::GetTypeId()); }

	/** Define a post-unary operator for the specified symbol */
	template<typename TExpressionNode>
	void DefinePostUnaryOperator() { PostUnaryOperators.Add(TGetExpressionNodeTypeId<TExpressionNode>::GetTypeId()); }

	/** Define a binary operator for the specified symbol, with the specified precedence */
	template<typename TExpressionNode>
	void DefineBinaryOperator(int32 InPrecedence) { BinaryOperators.Add(TGetExpressionNodeTypeId<TExpressionNode>::GetTypeId(), InPrecedence); }

public:

	/** Retrieve the corresponding grouping token for the specified open group type, or nullptr if it's not a group token */
	const FGuid* GetGrouping(const FGuid& TypeId) const;

	/** Check if this grammar defines a pre-unary operator for the specified symbol */
	bool HasPreUnaryOperator(const FGuid& TypeId) const;
	
	/** Check if this grammar defines a post-unary operator for the specified symbol */
	bool HasPostUnaryOperator(const FGuid& TypeId) const;

	/** Get the binary operator precedence for the specified symbol, if any */
	const int* GetBinaryOperatorPrecedence(const FGuid& TypeId) const;

private:

	TMap<FGuid, FGuid>	Groupings;
	TSet<FGuid>			PreUnaryOperators;
	TSet<FGuid>			PostUnaryOperators;
	TMap<FGuid, int32>	BinaryOperators;
};