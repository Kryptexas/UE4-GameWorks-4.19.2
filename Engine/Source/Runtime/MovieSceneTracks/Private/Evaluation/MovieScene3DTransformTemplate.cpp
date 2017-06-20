// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Evaluation/MovieScene3DTransformTemplate.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Sections/MovieScene3DTransformSection.h"
#include "Evaluation/MovieSceneTemplateCommon.h"
#include "MovieSceneCommonHelpers.h"
#include "MovieSceneEvaluation.h"
#include "IMovieScenePlayer.h"
#include "Evaluation/Blending/BlendableTokenStack.h"
#include "Evaluation/Blending/MovieSceneBlendingActuatorID.h"

DECLARE_CYCLE_STAT(TEXT("Transform Track Evaluate"), MovieSceneEval_TransformTrack_Evaluate, STATGROUP_MovieSceneEval);
DECLARE_CYCLE_STAT(TEXT("Transform Track Token Execute"), MovieSceneEval_TransformTrack_TokenExecute, STATGROUP_MovieSceneEval);

template<> FMovieSceneAnimTypeID GetBlendingDataType<MovieScene::FSimpleTransform>()
{
	static FMovieSceneAnimTypeID TypeID = FMovieSceneAnimTypeID::Unique();
	return TypeID;
}

namespace MovieScene
{
	FSimpleTransform::FSimpleTransform()
		: Translation(FVector::ZeroVector), Rotation(FVector::ZeroVector), Scale(FVector::ZeroVector)
	{}

	FSimpleTransform::FSimpleTransform(FVector InTranslation, FVector InRotation, FVector InScale)
		: Translation(InTranslation), Rotation(InRotation), Scale(InScale)
	{}

	FSimpleTransform& FSimpleTransform::operator+=(const FSimpleTransform& Other)
	{
		Translation += Other.Translation;
		Rotation += Other.Rotation;
		Scale += Other.Scale;
		return *this;
	}

	FSimpleTransform operator+(const FSimpleTransform& A, const FSimpleTransform& B)
	{
		return FSimpleTransform(A.Translation + B.Translation, A.Rotation + B.Rotation, A.Scale + B.Scale);
	}

	FSimpleTransform operator*(const FSimpleTransform& A, const FSimpleTransform& B)
	{
		return FSimpleTransform(A.Translation * B.Translation, A.Rotation * B.Rotation, A.Scale * B.Scale);
	}

	FSimpleTransform operator/(const FSimpleTransform& A, const FSimpleTransform& B)
	{
		FSimpleTransform Result = A;
		if (B.Translation.X != 0.f) Result.Translation.X /= B.Translation.X;
		if (B.Translation.Y != 0.f) Result.Translation.Y /= B.Translation.Y;
		if (B.Translation.Z != 0.f) Result.Translation.Z /= B.Translation.Z;

		if (B.Rotation.X != 0.f) Result.Rotation.X /= B.Rotation.X;
		if (B.Rotation.Y != 0.f) Result.Rotation.Y /= B.Rotation.Y;
		if (B.Rotation.Z != 0.f) Result.Rotation.Z /= B.Rotation.Z;

		if (B.Scale.X != 0.f) Result.Scale.X /= B.Scale.X;
		if (B.Scale.Y != 0.f) Result.Scale.Y /= B.Scale.Y;
		if (B.Scale.Z != 0.f) Result.Scale.Z /= B.Scale.Z;
		return Result;
	}

	FSimpleTransform operator*(const FSimpleTransform& In, float InScalar)
	{
		return FSimpleTransform(In.Translation * InScalar, In.Rotation * InScalar, In.Scale * InScalar);
	}

	FSimpleTransform GetMaskTransform(FMovieSceneTransformMask Mask)
	{
		return FSimpleTransform(Mask.GetTranslationFactor(), Mask.GetRotationFactor(), Mask.GetScaleFactor());
	}

	EMovieSceneTransformChannel GetMaskFromTransform(const FSimpleTransform& Transform)
	{
		EMovieSceneTransformChannel Mask = EMovieSceneTransformChannel::None;
		if (Transform.Translation.X != 0.f) Mask |= EMovieSceneTransformChannel::TranslationX;
		if (Transform.Translation.Y != 0.f) Mask |= EMovieSceneTransformChannel::TranslationY;
		if (Transform.Translation.Z != 0.f) Mask |= EMovieSceneTransformChannel::TranslationZ;

		if (Transform.Rotation.X != 0.f) Mask |= EMovieSceneTransformChannel::RotationX;
		if (Transform.Rotation.Y != 0.f) Mask |= EMovieSceneTransformChannel::RotationY;
		if (Transform.Rotation.Z != 0.f) Mask |= EMovieSceneTransformChannel::RotationZ;

		if (Transform.Scale.X != 0.f) Mask |= EMovieSceneTransformChannel::ScaleX;
		if (Transform.Scale.Y != 0.f) Mask |= EMovieSceneTransformChannel::ScaleY;
		if (Transform.Scale.Z != 0.f) Mask |= EMovieSceneTransformChannel::ScaleZ;
		return Mask;
	}

	FSimpleTransform FBlendableTransform::Resolve(TMovieSceneInitialValueStore<FSimpleTransform>& InitialValueStore)
	{
		// Any animated channels with a weight of 0 should match the object's initial position
		EMovieSceneTransformChannel ZeroWeightedChannels = AbsoluteMask & ~GetMaskFromTransform(AbsoluteWeight);
		if (EnumHasAnyFlags(ZeroWeightedChannels, EMovieSceneTransformChannel::AllTransform))
		{
			FSimpleTransform UnitTransform(FVector::OneVector, FVector::OneVector, FVector::OneVector);
			Absolute += FSimpleTransform(InitialValueStore.GetInitialValue()) * (UnitTransform * GetMaskTransform(ZeroWeightedChannels));
		}

		FSimpleTransform Result = Absolute / AbsoluteWeight + Additive;

		// Any non-animated channels must equal the current object value before application
		EMovieSceneTransformChannel AnimatedChannelMask = AbsoluteMask | AdditiveMask;
		if (!EnumHasAllFlags(AnimatedChannelMask, EMovieSceneTransformChannel::AllTransform))
		{
			FSimpleTransform CurrentValue = InitialValueStore.RetrieveCurrentValue();

			EMovieSceneTransformChannel NonAnimatedChannels = EMovieSceneTransformChannel::AllTransform & ~AnimatedChannelMask;
			Result = (FSimpleTransform(CurrentValue) * GetMaskTransform(NonAnimatedChannels)) + Result * GetMaskTransform(AnimatedChannelMask);
		}

		return Result;
	}

	void BlendValue(FBlendableTransform& OutBlend, const FMaskedTransform& InValue, float Weight, EMovieSceneBlendType BlendType, TMovieSceneInitialValueStore<FSimpleTransform>& InitialValueStore)
	{
		FSimpleTransform WeightTransform = GetMaskTransform(InValue.Mask) * Weight;

		if (BlendType == EMovieSceneBlendType::Absolute || BlendType == EMovieSceneBlendType::Relative)
		{
			if (BlendType == EMovieSceneBlendType::Relative)
			{
				FSimpleTransform InitialValue = InitialValueStore.GetInitialValue();
				OutBlend.Absolute += (FSimpleTransform(InitialValue) + InValue) * WeightTransform;
			}
			else
			{
				OutBlend.Absolute += InValue * WeightTransform;
			}

			OutBlend.AbsoluteWeight += WeightTransform;
			OutBlend.AbsoluteMask |= InValue.Mask.GetChannels();
		}
		else if (BlendType == EMovieSceneBlendType::Additive)
		{
			OutBlend.Additive += InValue * WeightTransform;
			OutBlend.AdditiveMask |= InValue.Mask.GetChannels();
		}
	}
}

struct FComponentTransformActuator : TMovieSceneBlendingActuator<MovieScene::FSimpleTransform>
{
	static FMovieSceneBlendingActuatorID GetActuatorTypeID()
	{
		static FMovieSceneAnimTypeID TypeID = TMovieSceneAnimTypeID<FComponentTransformActuator>();
		return FMovieSceneBlendingActuatorID(TypeID);
	}

	virtual MovieScene::FSimpleTransform RetrieveCurrentValue(UObject* InObject, IMovieScenePlayer* Player) const
	{
		if (USceneComponent* SceneComponent = MovieSceneHelpers::SceneComponentFromRuntimeObject(InObject))
		{
			return MovieScene::FSimpleTransform(SceneComponent->RelativeLocation, SceneComponent->RelativeRotation.Euler(), SceneComponent->RelativeScale3D);
		}

		return MovieScene::FSimpleTransform();
	}

	virtual void Actuate(UObject* InObject, const MovieScene::FSimpleTransform& InFinalValue, const TBlendableTokenStack<MovieScene::FSimpleTransform>& OriginalStack, const FMovieSceneContext& Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) override
	{
		check(InObject);

		USceneComponent* SceneComponent = MovieSceneHelpers::SceneComponentFromRuntimeObject(InObject);
		if (SceneComponent)
		{
			// Save preanimated state for all currently animating entities
			OriginalStack.SavePreAnimatedState(Player, *SceneComponent, FMobilityTokenProducer::GetAnimTypeID(), FMobilityTokenProducer());
			OriginalStack.SavePreAnimatedState(Player, *SceneComponent, F3DTransformTokenProducer::GetAnimTypeID(), F3DTransformTokenProducer());

			SceneComponent->SetMobility(EComponentMobility::Movable);

			F3DTransformTrackToken Token(InFinalValue.Translation, FRotator::MakeFromEuler(InFinalValue.Rotation), InFinalValue.Scale);
			Token.Apply(*SceneComponent, Context.GetDelta());
		}
	}

	virtual void Actuate(FMovieSceneInterrogationData& InterrogationData, const MovieScene::FSimpleTransform& InValue, const TBlendableTokenStack<MovieScene::FSimpleTransform>& OriginalStack, const FMovieSceneContext& Context) const override
	{
		InterrogationData.Add(FTransform(FRotator::MakeFromEuler(InValue.Rotation).Quaternion(), InValue.Translation, InValue.Scale), UMovieScene3DTransformTrack::GetInterrogationKey());
	}
};

FMovieSceneComponentTransformSectionTemplate::FMovieSceneComponentTransformSectionTemplate(const UMovieScene3DTransformSection& Section)
	: TemplateData(Section)
{
}

void FMovieSceneComponentTransformSectionTemplate::Evaluate(const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	using namespace MovieScene;

	FMaskedTransform TransformValue = TemplateData.Evaluate(Context.GetTime());

	// Ensure the accumulator knows how to actually apply component transforms
	FMovieSceneBlendingActuatorID ActuatorTypeID = FComponentTransformActuator::GetActuatorTypeID();
	if (!ExecutionTokens.GetBlendingAccumulator().FindActuator<FSimpleTransform>(ActuatorTypeID))
	{
		ExecutionTokens.GetBlendingAccumulator().DefineActuator(ActuatorTypeID, MakeShared<FComponentTransformActuator>());
	}

	// Add the blendable to the accumulator
	float Weight = EvaluateEasing(Context.GetTime());
	if (EnumHasAllFlags(TemplateData.Mask.GetChannels(), EMovieSceneTransformChannel::Weight))
	{
		Weight *= TemplateData.ManualWeight.Eval(Context.GetTime());
	}

	ExecutionTokens.BlendToken(ActuatorTypeID, TBlendableToken<FSimpleTransform>(TransformValue, TemplateData.BlendType, Weight));
}

void FMovieSceneComponentTransformSectionTemplate::Interrogate(const FMovieSceneContext& Context, FMovieSceneInterrogationData& Container, UObject* BindingOverride) const
{
	using namespace MovieScene;

	FMaskedTransform TransformValue = TemplateData.Evaluate(Context.GetTime());

	// Ensure the accumulator knows how to actually apply component transforms
	FMovieSceneBlendingActuatorID ActuatorTypeID = FComponentTransformActuator::GetActuatorTypeID();
	if (!Container.GetAccumulator().FindActuator<FSimpleTransform>(ActuatorTypeID))
	{
		Container.GetAccumulator().DefineActuator(ActuatorTypeID, MakeShared<FComponentTransformActuator>());
	}

	// Add the blendable to the accumulator
	float Weight = EvaluateEasing(Context.GetTime());
	if (EnumHasAllFlags(TemplateData.Mask.GetChannels(), EMovieSceneTransformChannel::Weight))
	{
		Weight *= TemplateData.ManualWeight.Eval(Context.GetTime());
	}

	Container.GetAccumulator().BlendToken(FMovieSceneEvaluationOperand(), ActuatorTypeID, FMovieSceneEvaluationScope(), Context, TBlendableToken<FSimpleTransform>(TransformValue, TemplateData.BlendType, Weight));
}

FMovieScene3DTransformTemplateData::FMovieScene3DTransformTemplateData(const UMovieScene3DTransformSection& Section)
	: BlendType(Section.GetBlendType().Get())
	, Mask(Section.GetMask())
{
	EMovieSceneTransformChannel MaskChannels = Mask.GetChannels();

	if (EnumHasAllFlags(MaskChannels, EMovieSceneTransformChannel::TranslationX))	TranslationCurve[0]	= Section.GetTranslationCurve(EAxis::X);
	if (EnumHasAllFlags(MaskChannels, EMovieSceneTransformChannel::TranslationY))	TranslationCurve[1]	= Section.GetTranslationCurve(EAxis::Y);
	if (EnumHasAllFlags(MaskChannels, EMovieSceneTransformChannel::TranslationZ))	TranslationCurve[2]	= Section.GetTranslationCurve(EAxis::Z);

	if (EnumHasAllFlags(MaskChannels, EMovieSceneTransformChannel::RotationX))		RotationCurve[0]	= Section.GetRotationCurve(EAxis::X);
	if (EnumHasAllFlags(MaskChannels, EMovieSceneTransformChannel::RotationY))		RotationCurve[1]	= Section.GetRotationCurve(EAxis::Y);
	if (EnumHasAllFlags(MaskChannels, EMovieSceneTransformChannel::RotationZ))		RotationCurve[2]	= Section.GetRotationCurve(EAxis::Z);

	if (EnumHasAllFlags(MaskChannels, EMovieSceneTransformChannel::ScaleX))			ScaleCurve[0]		= Section.GetScaleCurve(EAxis::X);
	if (EnumHasAllFlags(MaskChannels, EMovieSceneTransformChannel::ScaleY))			ScaleCurve[1]		= Section.GetScaleCurve(EAxis::Y);
	if (EnumHasAllFlags(MaskChannels, EMovieSceneTransformChannel::ScaleZ))			ScaleCurve[2]		= Section.GetScaleCurve(EAxis::Z);

	if (EnumHasAllFlags(MaskChannels, EMovieSceneTransformChannel::Weight))			ManualWeight		= Section.GetManualWeightCurve();
}

MovieScene::FMaskedTransform FMovieScene3DTransformTemplateData::Evaluate(float Time) const
{
	MovieScene::FMaskedTransform TransformValue(Mask);

	auto EvalChannel = [&TransformValue, Time](EMovieSceneTransformChannel Channel, float& ChannelValue, const FRichCurve& Curve)
	{
		if (!EnumHasAllFlags(TransformValue.Mask.GetChannels(), Channel))
		{
			return;
		}
		else if (Curve.GetConstRefOfKeys().Num() == 0 && Curve.GetDefaultValue() == MAX_flt)
		{
			// If no default was specified on this channel's curve, and it has no keys, mask out that channel
			TransformValue.Mask = FMovieSceneTransformMask(TransformValue.Mask.GetChannels() & ~Channel);
		}
		else
		{
			ChannelValue = Curve.Eval(Time);
		}
	};

	EvalChannel(EMovieSceneTransformChannel::TranslationX, TransformValue.Translation.X, TranslationCurve[0]);
	EvalChannel(EMovieSceneTransformChannel::TranslationY, TransformValue.Translation.Y, TranslationCurve[1]);
	EvalChannel(EMovieSceneTransformChannel::TranslationZ, TransformValue.Translation.Z, TranslationCurve[2]);

	EvalChannel(EMovieSceneTransformChannel::RotationX, TransformValue.Rotation.X, RotationCurve[0]);
	EvalChannel(EMovieSceneTransformChannel::RotationY, TransformValue.Rotation.Y, RotationCurve[1]);
	EvalChannel(EMovieSceneTransformChannel::RotationZ, TransformValue.Rotation.Z, RotationCurve[2]);

	EvalChannel(EMovieSceneTransformChannel::ScaleX, TransformValue.Scale.X, ScaleCurve[0]);
	EvalChannel(EMovieSceneTransformChannel::ScaleY, TransformValue.Scale.Y, ScaleCurve[1]);
	EvalChannel(EMovieSceneTransformChannel::ScaleZ, TransformValue.Scale.Z, ScaleCurve[2]);

	return TransformValue;
}