// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "EngineKismetLibraryClasses.h"
#include "CoreStats.h"

UKismetMathLibrary::UKismetMathLibrary(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

bool UKismetMathLibrary::RandomBool()
{
	return (FMath::RandRange(0,1) == 1) ? true : false;
}

bool UKismetMathLibrary::Not_PreBool(bool A)
{
	return !A;
}

bool UKismetMathLibrary::EqualEqual_BoolBool(bool A, bool B)
{
	return ((!A) == (!B));
}

bool UKismetMathLibrary::NotEqual_BoolBool(bool A, bool B)
{
	return ((!A) != (!B));
}

bool UKismetMathLibrary::BooleanAND(bool A, bool B)
{
	return A && B;
}


bool UKismetMathLibrary::BooleanOR(bool A, bool B)
{
	return A || B;
}

bool UKismetMathLibrary::BooleanXOR(bool A, bool B)
{
	return A ^ B;
}

uint8 UKismetMathLibrary::Multiply_ByteByte(uint8 A, uint8 B)
{
	return A * B;
}

uint8 UKismetMathLibrary::Divide_ByteByte(uint8 A, uint8 B)
{
	if (B == 0)
	{
		//@TODO: EXCEPTION: Throw script exception 
		FFrame::KismetExecutionMessage(TEXT("Divide by zero: Divide_ByteByte"), ELogVerbosity::Warning);
	}

	return (B != 0) ? (A / B) : 0;
}

uint8 UKismetMathLibrary::Percent_ByteByte(uint8 A, uint8 B)
{
	if (B == 0)
	{
		//@TODO: EXCEPTION: Whatever on these sites
		FFrame::KismetExecutionMessage(TEXT("Modulo by zero"), ELogVerbosity::Warning);
	}

	return (B != 0) ? (A % B) : 0;
}

uint8 UKismetMathLibrary::Add_ByteByte(uint8 A, uint8 B)
{
	return A + B;
}

uint8 UKismetMathLibrary::Subtract_ByteByte(uint8 A, uint8 B)
{
	return A - B;
}

bool UKismetMathLibrary::Less_ByteByte(uint8 A, uint8 B)
{
	return A < B;
}

bool UKismetMathLibrary::Greater_ByteByte(uint8 A, uint8 B)
{
	return A > B;
}

bool UKismetMathLibrary::LessEqual_ByteByte(uint8 A, uint8 B)
{
	return A <= B;
}

bool UKismetMathLibrary::GreaterEqual_ByteByte(uint8 A, uint8 B)
{
	return A >= B;
}

bool UKismetMathLibrary::EqualEqual_ByteByte(uint8 A, uint8 B)
{
	return A == B;
}

bool UKismetMathLibrary::NotEqual_ByteByte(uint8 A, uint8 B)
{
	return A != B;
}

int32 UKismetMathLibrary::Multiply_IntInt(int32 A, int32 B)
{
	return A * B;
}

int32 UKismetMathLibrary::Divide_IntInt(int32 A, int32 B)
{
	if (B == 0)
	{
		//@TODO: EXCEPTION: Throw script exception 
		FFrame::KismetExecutionMessage(TEXT("Divide by zero: Divide_IntInt"), ELogVerbosity::Warning);
	}

	return (B != 0) ? (A / B) : 0;
}

int32 UKismetMathLibrary::Percent_IntInt(int32 A, int32 B)
{
	if (B == 0)
	{
		//@TODO: EXCEPTION: Throw script exception 
		FFrame::KismetExecutionMessage(TEXT("Modulo by zero"), ELogVerbosity::Warning);
	}

	return (B != 0) ? (A % B) : 0;
}

int32 UKismetMathLibrary::Add_IntInt(int32 A, int32 B)
{
	return A + B;
}

int32 UKismetMathLibrary::Subtract_IntInt(int32 A, int32 B)
{
	return A - B;
}

bool UKismetMathLibrary::Less_IntInt(int32 A, int32 B)
{
	return A < B;
}

bool UKismetMathLibrary::Greater_IntInt(int32 A, int32 B)
{
	return A > B;
}

bool UKismetMathLibrary::LessEqual_IntInt(int32 A, int32 B)
{
	return A <= B;
}

bool UKismetMathLibrary::GreaterEqual_IntInt(int32 A, int32 B)
{
	return A >= B;
}

bool UKismetMathLibrary::EqualEqual_IntInt(int32 A, int32 B)
{
	return A == B;
}

bool UKismetMathLibrary::NotEqual_IntInt(int32 A, int32 B)
{
	return A != B;
}

int32 UKismetMathLibrary::And_IntInt(int32 A, int32 B)
{
	return A & B;
}

int32 UKismetMathLibrary::Xor_IntInt(int32 A, int32 B)
{
	return A ^ B;
}

int32 UKismetMathLibrary::Or_IntInt(int32 A, int32 B)
{
	return A | B;
}

int32 UKismetMathLibrary::SignOfInteger(int32 A)
{
	return FMath::Sign<int32>(A);
}

int32 UKismetMathLibrary::RandomInteger(int32 A)
{
	return FMath::RandHelper(A);
}

int32 UKismetMathLibrary::RandomIntegerInRange(int32 Min, int32 Max)
{
	return FMath::RandRange(Min, Max);
}

int32 UKismetMathLibrary::Min(int32 A, int32 B)
{
	return FMath::Min(A, B);
}

int32 UKismetMathLibrary::Max(int32 A, int32 B)
{
	return FMath::Max(A, B);
}

int32 UKismetMathLibrary::Clamp(int32 V, int32 A, int32 B)
{
	return FMath::Clamp(V, A, B);
}

int32 UKismetMathLibrary::Abs_Int(int32 A)
{
	return FMath::Abs(A);
}


float UKismetMathLibrary::MultiplyMultiply_FloatFloat(float Base, float Exp)
{
	return FMath::Pow(Base, Exp);
}	

float UKismetMathLibrary::Multiply_FloatFloat(float A, float B)
{
	return A * B;
}	

float UKismetMathLibrary::Multiply_IntFloat(int32 A, float B)
{
	return A * B;
}	

float UKismetMathLibrary::Divide_FloatFloat(float A, float B)
{
	if (B == 0.f)
	{
		//@TODO: EXCEPTION: Throw script exception 
		FFrame::KismetExecutionMessage(TEXT("Divide by zero: Divide_FloatFloat"), ELogVerbosity::Warning);
		return 0.f;
	}

	return A / B;
}	

float UKismetMathLibrary::Percent_FloatFloat(float A, float B)
{
	if (B == 0.f)
	{
		//@TODO: EXCEPTION: Throw script exception 
		FFrame::KismetExecutionMessage(TEXT("Modulo by zero"), ELogVerbosity::Warning);
		return 0.f;
	}

	return (B != 0.f) ? FMath::Fmod(A, B) : 0.f;
}	

float UKismetMathLibrary::Fraction(float A)
{
	return FMath::Fractional(A);
}

float UKismetMathLibrary::Add_FloatFloat(float A, float B)
{
	return A + B;
}	

float UKismetMathLibrary::Subtract_FloatFloat(float A, float B)
{
	return A - B;
}	

bool UKismetMathLibrary::Less_FloatFloat(float A, float B)
{
	return A < B;
}	

bool UKismetMathLibrary::Greater_FloatFloat(float A, float B)
{
	return A > B;
}	

bool UKismetMathLibrary::LessEqual_FloatFloat(float A, float B)
{
	return A <= B;
}	

bool UKismetMathLibrary::GreaterEqual_FloatFloat(float A, float B)
{
	return A >= B;
}	

bool UKismetMathLibrary::EqualEqual_FloatFloat(float A, float B)
{
	return A == B;
}	

bool UKismetMathLibrary::NotEqual_FloatFloat(float A, float B)
{
	return A != B;
}	

bool UKismetMathLibrary::InRange_FloatFloat(float Value, float Min, float Max, bool InclusiveMin, bool InclusiveMax)
{
	return ((InclusiveMin ? (Value >= Min) : (Value > Min)) && (InclusiveMax ? (Value <= Max) : (Value < Max)));
}	

float UKismetMathLibrary::Abs(float A)
{
	return FMath::Abs(A);
}	

float UKismetMathLibrary::Sin(float A)
{
	return FMath::Sin(A);
}	

float UKismetMathLibrary::Asin(float A)
{
	return FMath::Asin(A);
}	

float UKismetMathLibrary::Cos(float A)
{
	return FMath::Cos(A);
}	

float UKismetMathLibrary::Acos(float A)
{
	return FMath::Acos(A);
}	

float UKismetMathLibrary::Tan(float A)
{
	return FMath::Tan(A);
}	

float UKismetMathLibrary::Atan2(float A, float B)
{
	return FMath::Atan2(A, B);
}	

float UKismetMathLibrary::DegSin(float A)
{
	return FMath::Sin(PI/(180.f) * A);
}

float UKismetMathLibrary::DegAsin(float A)
{
	return (180.f)/PI * FMath::Asin(A);
}

float UKismetMathLibrary::DegCos(float A)
{
	return FMath::Cos(PI/(180.f) * A);
}

float UKismetMathLibrary::DegAcos(float A)
{
	return (180.f)/PI * FMath::Acos(A);
}

float UKismetMathLibrary::DegTan(float A)
{
	return FMath::Tan(PI/(180.f) * A);
}

float UKismetMathLibrary::DegAtan2(float A, float B)
{
	return (180.f)/PI * FMath::Atan2(A, B);
}

float UKismetMathLibrary::ClampAngle(float AngleDegrees, float MinAngleDegrees, float MaxAngleDegrees)
{
	return FMath::ClampAngle(AngleDegrees, MinAngleDegrees, MaxAngleDegrees);
}

float UKismetMathLibrary::Exp(float A)
{
	return FMath::Exp(A);
}	

float UKismetMathLibrary::Loge(float A)
{
	return FMath::Loge(A);
}	

float UKismetMathLibrary::Sqrt(float A)
{
	float Result = 0.f;
	if (A > 0.f)
	{
		// Can't use FMath::Sqrt(0) as it computes it as 1 / FMath::InvSqrt(). Not a problem as Sqrt variable defaults to 0 == sqrt(0).
		Result = FMath::Sqrt( A );
	}
	else if (A < 0.f)
	{
		FFrame::KismetExecutionMessage(TEXT("Attempt to take Sqrt() of negative number - returning 0."), ELogVerbosity::Warning);
	}

	return Result;
}	

float UKismetMathLibrary::Square(float A)
{
	return FMath::Square(A);
}	

int32 UKismetMathLibrary::Round(float A)
{
	return FMath::Round(A);
}	

int32 UKismetMathLibrary::FFloor(float A)
{
	return FMath::Floor(A);
}	

int32 UKismetMathLibrary::FCeil(float A)
{
	return FMath::Ceil(A);
}	

int32 UKismetMathLibrary::FMod(float Dividend, float Divisor, float& Remainder)
{
	int32 Result;
	if( Divisor != 0.f )
	{
		const float Quotient = Dividend / Divisor;
		Result = (Quotient < 0.f ? -1 : 1) * FMath::Floor( FMath::Abs(Quotient) );
		Remainder = FMath::Fmod(Dividend, Divisor);
	}
	else
	{
		//@TODO: EXCEPTION: Throw script exception 
		FFrame::KismetExecutionMessage(TEXT("Attempted modulo 0 - returning 0."), ELogVerbosity::Warning);

		Result = 0;
		Remainder = 0.f;
	}

	return Result;
}

float UKismetMathLibrary::SignOfFloat(float A)
{
	return FMath::Sign<float>(A);
}

float UKismetMathLibrary::NormalizeToRange(float Value, float RangeMin, float RangeMax)
{
	if (RangeMin > RangeMax)
	{
		Swap(RangeMin, RangeMax);
	}
	return (Value - RangeMin) / (RangeMax - RangeMin);
}

float UKismetMathLibrary::MapRange(float Value, float InRangeA, float InRangeB, float OutRangeA, float OutRangeB)
{
	return (Value - InRangeA) * (OutRangeB - OutRangeA) / (InRangeB - InRangeA) + OutRangeA;
}

float UKismetMathLibrary::MultiplyByPi(float Value)
{
	return Value * PI;
}

float UKismetMathLibrary::RandomFloat()
{
	return FMath::FRand();
}	

float UKismetMathLibrary::RandomFloatInRange(float Min, float Max)
{
	return FMath::FRandRange(Min, Max);
}	

float UKismetMathLibrary::FMin(float A, float B)
{
	return FMath::Min(A, B);
}	

float UKismetMathLibrary::FMax(float A, float B)
{
	return FMath::Max(A, B);
}	

float UKismetMathLibrary::FClamp(float V, float A, float B)
{
	return FMath::Clamp(V, A, B);
}	

float UKismetMathLibrary::Lerp(float A, float B, float V)
{
	return A + V*(B-A);
}	

float UKismetMathLibrary::FInterpTo(float Current, float Target, float DeltaTime, float InterpSpeed)
{
	return FMath::FInterpTo(Current, Target, DeltaTime, InterpSpeed);
}

FVector UKismetMathLibrary::Multiply_VectorFloat(FVector A, float B)
{
	return A * B;
}	

FVector UKismetMathLibrary::Multiply_VectorVector(FVector A, FVector B)
{
	return A * B;
}	

FVector UKismetMathLibrary::Divide_VectorFloat(FVector A, float B)
{
	if (B == 0.f)
	{
		//@TODO: EXCEPTION: Throw script exception 
		FFrame::KismetExecutionMessage(TEXT("Divide by zero: Divide_VectorFloat"), ELogVerbosity::Warning);
		return FVector::ZeroVector;
	}

	return A/B;
}	

FVector UKismetMathLibrary::Divide_VectorVector(FVector A, FVector B)
{
	if (B.X == 0.f || B.Y == 0.f || B.Z == 0.f)
	{
		//@TODO: EXCEPTION: Throw script exception 
		FFrame::KismetExecutionMessage(TEXT("Divide by zero: Divide_VectorVector"), ELogVerbosity::Warning);
		return FVector::ZeroVector;
	}

	return A/B;
}


FVector UKismetMathLibrary::Add_VectorVector(FVector A, FVector B)
{
	return A + B;
}	

FVector UKismetMathLibrary::Add_VectorFloat(FVector A, float B)
{
	return A + B;
}	

FVector UKismetMathLibrary::Subtract_VectorVector(FVector A, FVector B)
{
	return A - B;
}	

FVector UKismetMathLibrary::Subtract_VectorFloat(FVector A, float B)
{
	return A - B;
}	

FVector UKismetMathLibrary::LessLess_VectorRotator(FVector A, FRotator B)
{
	return B.UnrotateVector(A);
}	

FVector UKismetMathLibrary::GreaterGreater_VectorRotator(FVector A, FRotator B)
{
	return B.RotateVector(A);
}	

FVector  UKismetMathLibrary::RotateAngleAxis(FVector InVect, float AngleDeg, FVector Axis)
{
	return InVect.RotateAngleAxis(AngleDeg, Axis.SafeNormal());
}

bool UKismetMathLibrary::EqualEqual_VectorVector(FVector A, FVector B)
{
	return (A.X == B.X) && (A.Y == B.Y) && (A.Z == B.Z);
}	

bool UKismetMathLibrary::NotEqual_VectorVector(FVector A, FVector B)
{
	return (A.X != B.X) || (A.Y != B.Y) || (A.Z != B.Z);
}	

float UKismetMathLibrary::Dot_VectorVector(FVector A, FVector B)
{
	return A | B;
}	

FVector UKismetMathLibrary::Cross_VectorVector(FVector A, FVector B)
{
	return A ^ B;
}	

float UKismetMathLibrary::VSize(FVector A)
{
	return A.Size();
}	

float UKismetMathLibrary::VSize2D(FVector2D A)
{
	return A.Size();
}

FVector UKismetMathLibrary::Normal(FVector A)
{
	return A.SafeNormal();
}

FVector2D UKismetMathLibrary::Normal2D(FVector2D A)
{
	return A.SafeNormal();
}

FVector UKismetMathLibrary::VLerp(FVector A, FVector B, float V)
{
	return A + V*(B-A);
}	

FVector UKismetMathLibrary::VInterpTo(FVector Current, FVector Target, float DeltaTime, float InterpSpeed)
{
	return FMath::VInterpTo( Current, Target, DeltaTime, InterpSpeed );
}

FVector UKismetMathLibrary::RandomUnitVector()
{
	return FMath::VRand();
}

FVector UKismetMathLibrary::MirrorVectorByNormal(FVector A, FVector B)
{
	B = B.SafeNormal();
	return A - 2.f * B * (B | A);
}

FRotator UKismetMathLibrary::RandomRotator(bool bRoll)
{
	FRotator RRot;
	RRot.Yaw = FMath::FRand() * 360.f;
	RRot.Pitch = FMath::FRand() * 360.f;

	if (bRoll)
	{
		RRot.Roll = FMath::FRand() * 360.f;
	}
	else
	{
		RRot.Roll = 0;
	}
	return RRot;
}

FVector UKismetMathLibrary::ProjectOnTo(FVector X, FVector Y)
{
	return X.ProjectOnTo( Y );
}

FVector UKismetMathLibrary::NegateVector(FVector A)
{
	return -A;
}

FVector UKismetMathLibrary::ClampVectorSize(FVector A, float Min, float Max)
{
	return A.ClampSize(Min, Max);
}

float UKismetMathLibrary::GetMinElement(FVector A)
{
	return A.GetMin();
}

float UKismetMathLibrary::GetMaxElement(FVector A)
{
	return A.GetMax();
}

FVector UKismetMathLibrary::GetVectorArrayAverage(const TArray<FVector>& Vectors)
{
	FVector Sum(0.f);
	FVector Average(0.f);

	if(Vectors.Num() > 0)
	{
		for(int32 VecIdx=0; VecIdx<Vectors.Num(); VecIdx++)
		{
			Sum += Vectors[VecIdx];
		}

		Average = Sum / ((float)Vectors.Num());
	}

	return Average;
}

/** Find the unit direction vector from one position to another. */
FVector UKismetMathLibrary::GetDirectionVector(FVector From, FVector To)
{
	return (To - From).SafeNormal();
}


bool UKismetMathLibrary::EqualEqual_RotatorRotator(FRotator A, FRotator B)
{
	return (A.Pitch == B.Pitch) && (A.Yaw == B.Yaw) && (A.Roll == B.Roll);
}	

bool UKismetMathLibrary::NotEqual_RotatorRotator(FRotator A, FRotator B)
{
	return (A.Pitch != B.Pitch) || (A.Yaw != B.Yaw) || (A.Roll != B.Roll);
}	

FRotator UKismetMathLibrary::Multiply_RotatorFloat(FRotator A, float B)
{
	return A * B;
}	

FRotator UKismetMathLibrary::ComposeRotators(FRotator A, FRotator B)
{
	FQuat AQuat = FQuat(A);
	FQuat BQuat = FQuat(B);

	return FRotator(BQuat*AQuat);
}

FRotator UKismetMathLibrary::NegateRotator( FRotator A )
{
	FQuat AQuat = FQuat(A);
	return FRotator(AQuat.Inverse());
}

void UKismetMathLibrary::GetAxes(FRotator A, FVector& X, FVector& Y, FVector& Z)
{
	FRotationMatrix R(A);
	R.GetScaledAxes(X,Y,Z);
}

FRotator UKismetMathLibrary::RLerp(FRotator A, FRotator B, float Alpha, bool bShortestPath)
{
	FRotator DeltaAngle = B - A;

	if( bShortestPath )
	{
		DeltaAngle = DeltaAngle.GetNormalized();
	}

	return A + Alpha*DeltaAngle;
}

FRotator UKismetMathLibrary::NormalizedDeltaRotator(FRotator A, FRotator B)
{
	FRotator Delta = A - B;
	Delta.Normalize();
	return Delta;
}

FRotator UKismetMathLibrary::RotatorFromAxisAndAngle(FVector Axis, float Angle)
{
	FVector SafeAxis = Axis.SafeNormal(); // Make sure axis is unit length
	return FQuat(SafeAxis, FMath::DegreesToRadians(Angle)).Rotator();
}


FRotator UKismetMathLibrary::RInterpTo(FRotator Current, FRotator Target, float DeltaTime, float InterpSpeed)
{
	return FMath::RInterpTo( Current, Target, DeltaTime, InterpSpeed);
}

FLinearColor UKismetMathLibrary::LinearColorLerp(FLinearColor A, FLinearColor B, float Alpha)
{
	return A + Alpha * (B - A);
}

FLinearColor UKismetMathLibrary::Multiply_LinearColorLinearColor(FLinearColor A, FLinearColor B)
{
	return A * B;
}


FVector UKismetMathLibrary::TransformLocation(const FTransform& T, FVector Location)
{
	return T.TransformPosition(Location);
}

FVector UKismetMathLibrary::TransformDirection(const FTransform& T, FVector Direction)
{
	return T.TransformVectorNoScale(Direction);
}

FVector UKismetMathLibrary::InverseTransformLocation(const FTransform& T, FVector Location)
{
	return T.InverseTransformPosition(Location);
}

FVector UKismetMathLibrary::InverseTransformDirection(const FTransform& T, FVector Direction)
{
	return T.InverseTransformVectorNoScale(Direction);
}

FTransform UKismetMathLibrary::ComposeTransforms(const FTransform& A, const FTransform& B)
{
	return A * B;
}

FTransform UKismetMathLibrary::ConvertTransformToRelative(const FTransform& WorldTransform, const FTransform& LocalTransform)
{
	return LocalTransform.GetRelativeTransformReverse(WorldTransform);
}

FTransform UKismetMathLibrary::TLerp(const FTransform& A, const FTransform& B, float Alpha)
{
	FTransform Result;
	
	FTransform NA = A;
	FTransform NB = B;
	NA.NormalizeRotation();
	NB.NormalizeRotation();
	Result.Blend(NA, NB, Alpha);
	return Result;
}

FTransform UKismetMathLibrary::TInterpTo(const FTransform& Current, const FTransform& Target, float DeltaTime, float InterpSpeed)
{
	if( InterpSpeed <= 0.f )
	{
		return Target;
	}

	const float Alpha = FClamp(DeltaTime * InterpSpeed, 0.f, 1.f);

	return TLerp(Current, Target, Alpha);
}

FVector2D UKismetMathLibrary::Add_Vector2DVector2D(FVector2D A, FVector2D B)
{
	return A + B;
}

FVector2D UKismetMathLibrary::Subtract_Vector2DVector2D(FVector2D A, FVector2D B)
{
	return A - B;
}

FVector2D UKismetMathLibrary::Multiply_Vector2DFloat(FVector2D A, float B)
{
	return A * B;
}

FVector2D UKismetMathLibrary::Divide_Vector2DFloat(FVector2D A, float B)
{
	if (B == 0.f)
	{
		//@TODO: EXCEPTION: Throw script exception 
		FFrame::KismetExecutionMessage(TEXT("Divide by zero: Divide_Vector2DFloat"), ELogVerbosity::Warning);
		return FVector2D::ZeroVector;
	}

	return A / B;
}

FVector2D UKismetMathLibrary::Add_Vector2DFloat(FVector2D A, float B)
{
	return A+B;
}

FVector2D UKismetMathLibrary::Subtract_Vector2DFloat(FVector2D A, float B)
{
	return A-B;
}


bool UKismetMathLibrary::EqualEqual_NameName(FName A, FName B)
{
	return A == B;
}

bool UKismetMathLibrary::NotEqual_NameName(FName A, FName B)
{
	return A != B;
}

bool UKismetMathLibrary::EqualEqual_ObjectObject(class UObject* A, class UObject* B)
{
	return A == B;
}

bool UKismetMathLibrary::NotEqual_ObjectObject(class UObject* A, class UObject* B)
{
	return A != B;
}

bool UKismetMathLibrary::ClassIsChildOf(TSubclassOf<class UObject> TestClass, TSubclassOf<class UObject> ParentClass)
{
	return ((*ParentClass != NULL) && (*TestClass != NULL)) ? (*TestClass)->IsChildOf(*ParentClass) : false;
}

float UKismetMathLibrary::Conv_ByteToFloat(uint8 InByte)
{
	return (float)InByte;
}

float UKismetMathLibrary::Conv_IntToFloat(int32 InInt)
{
	return (float)InInt;
}

uint8 UKismetMathLibrary::Conv_IntToByte(int32 InInt)
{
	return (uint8)InInt;
}

bool UKismetMathLibrary::Conv_IntToBool(int32 InInt)
{
	return InInt == 0 ? false : true;
}

int32 UKismetMathLibrary::Conv_BoolToInt(bool InBool)
{
	return InBool ? 1 : 0;
}

uint8 UKismetMathLibrary::Conv_BoolToByte(bool InBool)
{
	return InBool ? 1 : 0;
}

float UKismetMathLibrary::Conv_BoolToFloat(bool InBool)
{
	return InBool ? 1.0f : 0.0f;
}

int32 UKismetMathLibrary::Conv_ByteToInt(uint8 InByte)
{
	return (int32)InByte;
}

FRotator UKismetMathLibrary::Conv_VectorToRotator(FVector InVec)
{
	return InVec.Rotation();
}

FVector UKismetMathLibrary::Conv_RotatorToVector(FRotator InRot)
{
	return InRot.Vector();
}

FLinearColor UKismetMathLibrary::Conv_VectorToLinearColor(FVector InVec)
{
	return FLinearColor(InVec);	
}

FVector UKismetMathLibrary::Conv_LinearColorToVector(FLinearColor InLinearColor)
{
	return FVector(InLinearColor);
}

FLinearColor UKismetMathLibrary::Conv_ColorToLinearColor(FColor InColor)
{
	return FLinearColor(InColor);
}

FColor UKismetMathLibrary::Conv_LinearColorToColor(FLinearColor InLinearColor)
{
	return FColor(InLinearColor);
}

FTransform UKismetMathLibrary::Conv_VectorToTransform(FVector InTranslation)
{
	return FTransform(InTranslation);
}

FVector UKismetMathLibrary::Conv_FloatToVector(float InFloat)
{
	return FVector(InFloat);
}

FLinearColor UKismetMathLibrary::Conv_FloatToLinearColor(float InFloat)
{
	return FLinearColor(InFloat, InFloat, InFloat);
}

FVector UKismetMathLibrary::MakeVector(float X, float Y, float Z)
{
	return FVector(X,Y,Z);
}

void UKismetMathLibrary::BreakVector(FVector InVec, float& X, float& Y, float& Z)
{
	X = InVec.X;
	Y = InVec.Y;
	Z = InVec.Z;
}

FVector2D UKismetMathLibrary::MakeVector2D(float X, float Y)
{
	return FVector2D(X, Y);
}

void UKismetMathLibrary::BreakVector2D(FVector2D InVec, float& X, float& Y)
{
	X = InVec.X;
	Y = InVec.Y;
}

FVector UKismetMathLibrary::GetForwardVector(FRotator InRot)
{
	return FRotationMatrix(InRot).GetScaledAxis( EAxis::X );
}

FVector UKismetMathLibrary::GetRightVector(FRotator InRot)
{
	return FRotationMatrix(InRot).GetScaledAxis( EAxis::Y );
}

FVector UKismetMathLibrary::GetUpVector(FRotator InRot)
{
	return FRotationMatrix(InRot).GetScaledAxis( EAxis::Z );
}

FRotator UKismetMathLibrary::MakeRot(float Pitch, float Yaw, float Roll)
{
	return FRotator(Pitch,Yaw,Roll);
}

FRotator UKismetMathLibrary::FindLookAtRotation(const FVector& Start, const FVector& Target)
{
	return MakeRotFromX(Target - Start);
}

FRotator UKismetMathLibrary::MakeRotFromX(const FVector& X)
{
	return FRotationMatrix::MakeFromX(X).Rotator();
}

FRotator UKismetMathLibrary::MakeRotFromY(const FVector& Y)
{
	return FRotationMatrix::MakeFromY(Y).Rotator();
}

FRotator UKismetMathLibrary::MakeRotFromZ(const FVector& Z)
{
	return FRotationMatrix::MakeFromZ(Z).Rotator();
}

FRotator UKismetMathLibrary::MakeRotFromXY(const FVector& X, const FVector& Y)
{
	return FRotationMatrix::MakeFromXY(X, Y).Rotator();
}

FRotator UKismetMathLibrary::MakeRotFromXZ(const FVector& X, const FVector& Z)
{
	return FRotationMatrix::MakeFromXZ(X, Z).Rotator();
}

FRotator UKismetMathLibrary::MakeRotFromYX(const FVector& Y, const FVector& X)
{
	return FRotationMatrix::MakeFromYX(Y, X).Rotator();
}

FRotator UKismetMathLibrary::MakeRotFromYZ(const FVector& Y, const FVector& Z)
{
	return FRotationMatrix::MakeFromYZ(Y, Z).Rotator();
}

FRotator UKismetMathLibrary::MakeRotFromZX(const FVector& Z, const FVector& X)
{
	return FRotationMatrix::MakeFromZX(Z, X).Rotator();
}

FRotator UKismetMathLibrary::MakeRotFromZY(const FVector& Z, const FVector& Y)
{
	return FRotationMatrix::MakeFromZY(Z, Y).Rotator();
}


void UKismetMathLibrary::BreakRot(FRotator InRot, float& Pitch, float& Yaw, float& Roll)
{
	Pitch = InRot.Pitch;
	Yaw = InRot.Yaw;
	Roll = InRot.Roll;
}

void UKismetMathLibrary::BreakRotIntoAxes(const FRotator& InRot, FVector& X, FVector& Y, FVector& Z)
{
	FRotationMatrix(InRot).GetScaledAxes(X, Y, Z);
}


FTransform UKismetMathLibrary::MakeTransform(FVector Translation, FRotator Rotation, FVector Scale)
{
	return FTransform(Rotation,Translation,Scale);
}

void UKismetMathLibrary::BreakTransform(const FTransform& InTransform, FVector& Translation, FRotator& Rotation, FVector& Scale)
{
	Translation = InTransform.GetLocation();
	Rotation = InTransform.Rotator();
	Scale = InTransform.GetScale3D();
}

FLinearColor UKismetMathLibrary::MakeColor(float R, float G, float B, float A)
{
	return FLinearColor(R,G,B,A);
}

void UKismetMathLibrary::BreakColor(const FLinearColor InColor, float& R, float& G, float& B, float& A)
{
	R = InColor.R;
	G = InColor.G;
	B = InColor.B;
	A = InColor.A;
}

FLinearColor UKismetMathLibrary::HSVToRGB(float H, float S, float V, float A)
{
	const FLinearColor HSV(H, S, V, A);
	return HSV.HSVToLinearRGB();
}

void UKismetMathLibrary::RGBToHSV(const FLinearColor InColor, float& H, float& S, float& V, float& A)
{
	const FLinearColor HSV(InColor.LinearRGBToHSV());
	H = HSV.R;
	S = HSV.G;
	V = HSV.B;
	A = HSV.A;
}

void UKismetMathLibrary::HSVToRGB_Vector(const FLinearColor HSV, FLinearColor& RGB)
{
	RGB = HSV.HSVToLinearRGB();
}

void UKismetMathLibrary::RGBToHSV_Vector(const FLinearColor RGB, FLinearColor& HSV)
{
	HSV = RGB.LinearRGBToHSV();
}

FString UKismetMathLibrary::SelectString(const FString& A, const FString& B, bool bSelectA)
{
	return bSelectA ? A : B;
}

int32 UKismetMathLibrary::SelectInt(int32 A, int32 B, bool bSelectA)
{
	return bSelectA ? A : B;
}

float UKismetMathLibrary::SelectFloat(float A, float B, bool bSelectA)
{
	return bSelectA ? A : B;
}

FVector UKismetMathLibrary::SelectVector(FVector A, FVector B, bool bSelectA)
{
	return bSelectA ? A : B;
}

FRotator UKismetMathLibrary::SelectRotator(FRotator A, FRotator B, bool bSelectA)
{
	return bSelectA ? A : B;
}

FLinearColor UKismetMathLibrary::SelectColor(FLinearColor A, FLinearColor B, bool bSelectA)
{
	return bSelectA ? A : B;
}

FTransform UKismetMathLibrary::SelectTransform(const FTransform& A, const FTransform& B, bool bSelectA)
{
	return bSelectA ? A : B;
}

UObject* UKismetMathLibrary::SelectObject(UObject* A, UObject* B, bool bSelectA)
{
	return bSelectA ? A : B;
}

FRotator UKismetMathLibrary::MakeRotationFromAxes(FVector Forward, FVector Right, FVector Up)
{
	Forward.Normalize();
	Right.Normalize();
	Up.Normalize();

	FMatrix RotMatrix(Forward, Right, Up, FVector::ZeroVector);

	return RotMatrix.Rotator();
}


// Random stream functions


int32 UKismetMathLibrary::RandomIntegerFromStream(int32 Max, const FRandomStream& Stream)
{
	return Stream.RandHelper(Max);
}

int32 UKismetMathLibrary::RandomIntegerInRangeFromStream(int32 Min, int32 Max, const FRandomStream& Stream)
{
	return Stream.RandRange(Min, Max);
}

bool UKismetMathLibrary::RandomBoolFromStream(const FRandomStream& Stream)
{
	return (Stream.RandRange(0,1) == 1) ? true : false;
}

float UKismetMathLibrary::RandomFloatFromStream(const FRandomStream& Stream)
{
	return Stream.FRand();
}

float UKismetMathLibrary::RandomFloatInRangeFromStream(float Min, float Max, const FRandomStream& Stream)
{
	return Min + (Max - Min) * RandomFloatFromStream(Stream);
}

FVector UKismetMathLibrary::RandomUnitVectorFromStream(const FRandomStream& Stream)
{
	return Stream.VRand();
}

FRotator UKismetMathLibrary::RandomRotatorFromStream(bool bRoll, const FRandomStream& Stream)
{
	FRotator RRot;
	RRot.Yaw = RandomFloatFromStream(Stream) * 360.f;
	RRot.Pitch = RandomFloatFromStream(Stream) * 360.f;

	if (bRoll)
	{
		RRot.Roll = RandomFloatFromStream(Stream) * 360.f;
	}
	else
	{
		RRot.Roll = 0;
	}
	return RRot;
}

void UKismetMathLibrary::ResetRandomStream(const FRandomStream& Stream)
{
	Stream.Reset();
}

void UKismetMathLibrary::SeedRandomStream(FRandomStream& Stream)
{
	Stream.GenerateNewSeed();
}

void UKismetMathLibrary::SetRandomStreamSeed(FRandomStream& Stream, int32 NewSeed)
{
	Stream.Initialize(NewSeed);
}


void UKismetMathLibrary::MinimumAreaRectangle(class UObject* WorldContextObject, const TArray<FVector>& InVerts, const FVector& SampleSurfaceNormal, FVector& OutRectCenter, FRotator& OutRectRotation, float& OutSideLengthX, float& OutSideLengthY, bool bDebugDraw)
{
	float MinArea = -1.f;
	float CurrentArea = -1.f;
	FVector SupportVectorA, SupportVectorB;
	FVector RectSideA, RectSideB;
	float MinDotResultA, MinDotResultB, MaxDotResultA, MaxDotResultB;
	FVector TestEdge;
	float TestEdgeDot = 0.f;
	FVector PolyNormal(0.f, 0.f, 1.f);
	TArray<int32> PolyVertIndices;

	// Bail if we receive an empty InVerts array
	if( InVerts.Num() == 0 )
	{
		return;
	}

	// Compute the approximate normal of the poly, using the direction of SampleSurfaceNormal for guidance
	PolyNormal = (InVerts[InVerts.Num() / 3] - InVerts[0]) ^ (InVerts[InVerts.Num() * 2 / 3] - InVerts[InVerts.Num() / 3]);
	if( (PolyNormal | SampleSurfaceNormal) < 0.f )
	{
		PolyNormal = -PolyNormal;
	}

	// Transform the sample points to 2D
	FMatrix SurfaceNormalMatrix = FRotationMatrix::MakeFromZX(PolyNormal, FVector(1.f, 0.f, 0.f));
	TArray<FVector> TransformedVerts;
	OutRectCenter = FVector(0.f);
	for( int32 Idx = 0; Idx < InVerts.Num(); ++Idx )
	{
		OutRectCenter += InVerts[Idx];
		TransformedVerts.Add(SurfaceNormalMatrix.InverseTransformVector(InVerts[Idx]));
	}
	OutRectCenter /= InVerts.Num();

	// Compute the convex hull of the sample points
	ConvexHull2D::ComputeConvexHull(TransformedVerts, PolyVertIndices);

	// Minimum area rectangle as computed by http://www.geometrictools.com/Documentation/MinimumAreaRectangle.pdf
	for( int32 Idx = 1; Idx < PolyVertIndices.Num() - 1; ++Idx )
	{
		SupportVectorA = (TransformedVerts[PolyVertIndices[Idx]] - TransformedVerts[PolyVertIndices[Idx-1]]).SafeNormal();
		SupportVectorA.Z = 0.f;
		SupportVectorB.X = -SupportVectorA.Y;
		SupportVectorB.Y = SupportVectorA.X;
		SupportVectorB.Z = 0.f;
		MinDotResultA = MinDotResultB = MaxDotResultA = MaxDotResultB = 0.f;

		for (int TestVertIdx = 1; TestVertIdx < PolyVertIndices.Num(); ++TestVertIdx )
		{
			TestEdge = TransformedVerts[PolyVertIndices[TestVertIdx]] - TransformedVerts[PolyVertIndices[0]];
			TestEdgeDot = SupportVectorA | TestEdge;
			if( TestEdgeDot < MinDotResultA )
			{
				MinDotResultA = TestEdgeDot;
			}
			else if(TestEdgeDot > MaxDotResultA )
			{
				MaxDotResultA = TestEdgeDot;
			}

			TestEdgeDot = SupportVectorB | TestEdge;
			if( TestEdgeDot < MinDotResultB )
			{
				MinDotResultB = TestEdgeDot;
			}
			else if( TestEdgeDot > MaxDotResultB )
			{
				MaxDotResultB = TestEdgeDot;
			}
		}

		CurrentArea = (MaxDotResultA - MinDotResultA) * (MaxDotResultB - MinDotResultB);
		if( MinArea < 0.f || CurrentArea < MinArea )
		{
			MinArea = CurrentArea;
			RectSideA = SupportVectorA * (MaxDotResultA - MinDotResultA);
			RectSideB = SupportVectorB * (MaxDotResultB - MinDotResultB);
		}
	}

	RectSideA = SurfaceNormalMatrix.TransformVector(RectSideA);
	RectSideB = SurfaceNormalMatrix.TransformVector(RectSideB);
	OutRectRotation = FRotationMatrix::MakeFromZX(PolyNormal, RectSideA).Rotator();
	OutSideLengthX = RectSideA.Size();
	OutSideLengthY = RectSideB.Size();

	if( bDebugDraw )
	{
		DrawDebugSphere(GEngine->GetWorldFromContextObject(WorldContextObject), OutRectCenter, 10.f, 12, FColor::Yellow, true);
		DrawDebugCoordinateSystem(GEngine->GetWorldFromContextObject(WorldContextObject), OutRectCenter, SurfaceNormalMatrix.Rotator(), 100.f, true);
		DrawDebugLine(GEngine->GetWorldFromContextObject(WorldContextObject), OutRectCenter - RectSideA * 0.5f + FVector(0,0,10.f), OutRectCenter + RectSideA * 0.5f + FVector(0,0,10.f), FColor::Green, true,-1, 0, 5.f);
		DrawDebugLine(GEngine->GetWorldFromContextObject(WorldContextObject), OutRectCenter - RectSideB * 0.5f + FVector(0,0,10.f), OutRectCenter + RectSideB * 0.5f + FVector(0,0,10.f), FColor::Blue, true,-1, 0, 5.f);
	}
}

