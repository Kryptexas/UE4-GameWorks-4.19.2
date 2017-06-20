// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Curves/RichCurve.h"
#include "Evaluation/MovieSceneEvalTemplate.h"
#include "Evaluation/MovieScenePropertyTemplate.h"
#include "Curves/IntegralCurve.h"
#include "Curves/StringCurve.h"
#include "Templates/UnrealTypeTraits.h"
#include "MovieScene3DTransformTemplate.h"
#include "MovieScenePropertyTemplates.generated.h"

class UMovieSceneBoolSection;
class UMovieSceneByteSection;
class UMovieSceneFloatSection;
class UMovieSceneIntegerSection;
class UMovieScenePropertyTrack;
class UMovieSceneStringSection;
class UMovieSceneVectorSection;
class UMovieSceneEnumSection;
class UMovieScene3DTransformSection;

USTRUCT()
struct FMovieSceneBoolPropertySectionTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()
	
	FMovieSceneBoolPropertySectionTemplate(){}
	FMovieSceneBoolPropertySectionTemplate(const UMovieSceneBoolSection& Section, const UMovieScenePropertyTrack& Track);

protected:

	virtual UScriptStruct& GetScriptStructImpl() const override
	{
		return *StaticStruct();
	}
	virtual void SetupOverrides() override
	{
		EnableOverrides(RequiresSetupFlag | RequiresInitializeFlag);
	}
	virtual void Setup(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override
	{
		PropertyData.SetupCachedTrack<bool>(PersistentData);
	}
	virtual void Initialize(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override
	{
		PropertyData.SetupCachedFrame<bool>(Operand, PersistentData, Player);
	}

	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;

	UPROPERTY()
	FMovieScenePropertySectionData PropertyData;

	UPROPERTY()
	FIntegralCurve BoolCurve;
};

USTRUCT()
struct FMovieSceneFloatPropertySectionTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()
	
	FMovieSceneFloatPropertySectionTemplate(){}
	FMovieSceneFloatPropertySectionTemplate(const UMovieSceneFloatSection& Section, const UMovieScenePropertyTrack& Track);

protected:

	virtual UScriptStruct& GetScriptStructImpl() const override
	{
		return *StaticStruct();
	}
	virtual void SetupOverrides() override
	{
		EnableOverrides(RequiresSetupFlag | RequiresInitializeFlag);
	}
	virtual void Setup(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override
	{
		PropertyData.SetupCachedTrack<float>(PersistentData);
	}
	virtual void Initialize(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override
	{
		PropertyData.SetupCachedFrame<float>(Operand, PersistentData, Player);
	}

	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;

	UPROPERTY()
	FMovieScenePropertySectionData PropertyData;

	UPROPERTY()
	FRichCurve FloatCurve;
};

USTRUCT()
struct FMovieSceneBytePropertySectionTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()
	
	FMovieSceneBytePropertySectionTemplate(){}
	FMovieSceneBytePropertySectionTemplate(const UMovieSceneByteSection& Section, const UMovieScenePropertyTrack& Track);

protected:

	virtual UScriptStruct& GetScriptStructImpl() const override
	{
		return *StaticStruct();
	}
	virtual void SetupOverrides() override
	{
		EnableOverrides(RequiresSetupFlag | RequiresInitializeFlag);
	}
	virtual void Setup(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override
	{
		PropertyData.SetupCachedTrack<uint8>(PersistentData);
	}
	virtual void Initialize(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override
	{
		PropertyData.SetupCachedFrame<uint8>(Operand, PersistentData, Player);
	}

	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;

	UPROPERTY()
	FMovieScenePropertySectionData PropertyData;

	UPROPERTY()
	FIntegralCurve ByteCurve;
};

USTRUCT()
struct FMovieSceneEnumPropertySectionTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()
	
	FMovieSceneEnumPropertySectionTemplate(){}
	FMovieSceneEnumPropertySectionTemplate(const UMovieSceneEnumSection& Section, const UMovieScenePropertyTrack& Track);

protected:

	virtual UScriptStruct& GetScriptStructImpl() const override
	{
		return *StaticStruct();
	}
	virtual void SetupOverrides() override
	{
		EnableOverrides(RequiresSetupFlag | RequiresInitializeFlag);
	}
	virtual void Setup(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override
	{
		PropertyData.SetupCachedTrack<int64>(PersistentData);
	}
	virtual void Initialize(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override
	{
		PropertyData.SetupCachedFrame<int64>(Operand, PersistentData, Player);
	}

	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;

	UPROPERTY()
	FMovieScenePropertySectionData PropertyData;

	UPROPERTY()
	FIntegralCurve EnumCurve;
};

USTRUCT()
struct FMovieSceneIntegerPropertySectionTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()
	
	FMovieSceneIntegerPropertySectionTemplate(){}
	FMovieSceneIntegerPropertySectionTemplate(const UMovieSceneIntegerSection& Section, const UMovieScenePropertyTrack& Track);

protected:

	virtual UScriptStruct& GetScriptStructImpl() const override
	{
		return *StaticStruct();
	}
	virtual void SetupOverrides() override
	{
		EnableOverrides(RequiresSetupFlag | RequiresInitializeFlag);
	}
	virtual void Setup(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override
	{
		PropertyData.SetupCachedTrack<int32>(PersistentData);
	}
	virtual void Initialize(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override
	{
		PropertyData.SetupCachedFrame<int32>(Operand, PersistentData, Player);
	}

	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;

	UPROPERTY()
	FMovieScenePropertySectionData PropertyData;

	UPROPERTY()
	FIntegralCurve IntegerCurve;
};

USTRUCT()
struct FMovieSceneStringPropertySectionTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()
	
	FMovieSceneStringPropertySectionTemplate(){}
	FMovieSceneStringPropertySectionTemplate(const UMovieSceneStringSection& Section, const UMovieScenePropertyTrack& Track);

protected:

	virtual UScriptStruct& GetScriptStructImpl() const override
	{
		return *StaticStruct();
	}
	virtual void SetupOverrides() override
	{
		EnableOverrides(RequiresSetupFlag | RequiresInitializeFlag);
	}
	virtual void Setup(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override
	{
		PropertyData.SetupCachedTrack<FString>(PersistentData);
	}
	virtual void Initialize(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override
	{
		PropertyData.SetupCachedFrame<FString>(Operand, PersistentData, Player);
	}

	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;

	UPROPERTY()
	FMovieScenePropertySectionData PropertyData;

	UPROPERTY()
	FStringCurve StringCurve;
};

Expose_TNameOf(FVector2D);
Expose_TNameOf(FVector);
Expose_TNameOf(FVector4);

USTRUCT()
struct FMovieSceneVectorPropertySectionTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()
	
	FMovieSceneVectorPropertySectionTemplate(){}
	FMovieSceneVectorPropertySectionTemplate(const UMovieSceneVectorSection& Section, const UMovieScenePropertyTrack& Track);

protected:

	virtual UScriptStruct& GetScriptStructImpl() const override
	{
		return *StaticStruct();
	}
	virtual void SetupOverrides() override
	{
		EnableOverrides(RequiresSetupFlag | RequiresInitializeFlag);
	}

	virtual void Setup(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override;
	virtual void Initialize(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override;
	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;

	UPROPERTY()
	FMovieScenePropertySectionData PropertyData;

	UPROPERTY()
	FRichCurve ComponentCurves[4];

	UPROPERTY()
	int32 NumChannelsUsed;
};

Expose_TNameOf(FTransform);

USTRUCT()
struct FMovieSceneTransformPropertySectionTemplate : public FMovieSceneEvalTemplate
{
	GENERATED_BODY()

	FMovieSceneTransformPropertySectionTemplate(){}
	FMovieSceneTransformPropertySectionTemplate(const UMovieScene3DTransformSection& Section, const UMovieScenePropertyTrack& Track);

protected:

	virtual UScriptStruct& GetScriptStructImpl() const override { return *StaticStruct(); }
	virtual void SetupOverrides() override { EnableOverrides(RequiresSetupFlag); }

	virtual void Setup(FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const override;
	virtual void Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const override;

	UPROPERTY()
	FMovieScene3DTransformTemplateData TemplateData;

	UPROPERTY()
	FMovieScenePropertySectionData PropertyData;
};