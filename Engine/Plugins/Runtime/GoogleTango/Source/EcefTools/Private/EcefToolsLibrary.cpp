// Copyright 2017 Google Inc.

#include "EcefToolsLibrary.h"
#include "EcefTools.h"
#include "Rotator.h"
#include "TangoEcefUtils.h"
#include "JsonSerializer.h"
#include "JsonObject.h"

FECEF_Transform UEcefToolsLibrary::WorldSpaceOrigin;
FECEF_Transform UEcefToolsLibrary::WorldSpaceOriginInverse;

static void InvertPose(const double* P, const double* Q, double* InvP, double* InvQ)
{
	double TargetRotation[9];
	double TargetRotationTranspose[9];
	ecef_tools::QuaternionToRotationMatrix(Q, TargetRotation);
	ecef_tools::GetMatrix3x3Transpose(TargetRotation, TargetRotationTranspose);
	ecef_tools::ExpressVector3(TargetRotationTranspose, P, InvP);
	for (int32 i = 0; i < 3; i++)
	{
		InvP[i] = -InvP[i];
		InvQ[i] = -Q[i];
	}
	InvQ[3] = Q[3];
}

// Composes P2, Q2 followed by P1, Q1

static void ComposePoses(
	const double* P1, const double* Q1,
	const double* P2, const double* Q2,
	double* P, double* Q)
{
	double SourceRotation[9];
	double TargetRotation[9];
	ecef_tools::QuaternionToRotationMatrix(Q2, TargetRotation);
	double P0[3];
	ecef_tools::ExpressVector3(TargetRotation, P1, P0);
	ecef_tools::QuaternionToRotationMatrix(Q1, SourceRotation);
	double ComposedRotation[9];
	ecef_tools::MultiplyMatrix3x3(SourceRotation, TargetRotation, ComposedRotation);
	ecef_tools::RotationMatrixToQuaternion(ComposedRotation, Q);
	for (int32 i = 0; i < 3; i++)
	{
		P[i] = P0[i] + P2[i];
	}
}

static void AddField(
	FString& Out, const FString& FieldName,
	double Value, FString& Sep)
{
	Out += Sep;
	Out += "\"";
	Out += FieldName;
	Out += "\":";
	FString ValueStr = FString::Printf(TEXT("%.17f"), Value);
	Out += ValueStr;
	Sep = ", ";
}

static void LocationToString(
	const ecef_tools::Location& Location,
	const FRotator& Orientation,
	FString *Result)
{
	FString Sep;
	*Result = "{";
	AddField(*Result, TEXT("Latitude"), FMath::RadiansToDegrees(Location.latitude), Sep);
	AddField(*Result, TEXT("Longitude"), FMath::RadiansToDegrees(Location.longitude), Sep);
	AddField(*Result, TEXT("Altitude"), Location.altitude, Sep);
	//	AddField(Result, TEXT("Heading"), FMath::RadiansToDegrees(Location.heading), Sep);
	AddField(*Result, TEXT("Pitch"), Orientation.Pitch, Sep);
	AddField(*Result, TEXT("Yaw"), Orientation.Yaw, Sep);
	AddField(*Result, TEXT("Roll"), Orientation.Roll, Sep);
	*Result += "}";
}


static void DoConvertEcefToLocation(
	const double* Position,
	const double* Orientation,
	FString* Result)
{
	ecef_tools::Location Location;
	ecef_tools::PositionInEcefToLocation(Position, &Location.latitude, &Location.longitude, &Location.altitude);
	FString Str;
	FRotator Rot(FQuat(Orientation[0], Orientation[1], Orientation[2], Orientation[3]));
	LocationToString(Location, Rot, Result);
}

static void EcefToLocation(
	const double* OriginPosInEcef, const double* OriginOrientInEcef,
	const FVector& Position,
	const FRotator& Orientation,
	FString* Result)
{
	double Pos[3];
	double Orient[4];
	Pos[0] = Position.X;
	Pos[1] = Position.Y;
	Pos[2] = Position.Z;
	FQuat Q = Orientation.Quaternion();
	Orient[0] = Q.X;
	Orient[1] = Q.Y;
	Orient[2] = Q.Z;
	Orient[3] = Q.W;
	ecef_tools::Location Loc;
	ComposePoses(Pos, Orient, OriginPosInEcef, OriginOrientInEcef, Pos, Orient);
	ecef_tools::PositionInEcefToLocation(Pos, &Loc.latitude, &Loc.longitude, &Loc.altitude);
	FString Str;
	FRotator EcefOrient(FQuat((float)Orient[0], (float)Orient[1], (float)Orient[2], (float)Orient[3]));
	LocationToString(Loc, EcefOrient, Result);
}

static void ECEF_FromWorldSpace(
	const double* OriginPosInEcef, const double* OriginOrientInEcef,
	const FVector& Position,
	const FQuat& Orientation,
	FECEF_Transform* Pose)
{
	double Pos[3];
	double Orient[4];
	double Scale = .01;
	Pos[0] = Position.X * Scale;
	Pos[1] = Position.Y * Scale;
	Pos[2] = Position.Z * Scale;
	Orient[0] = Orientation.X;
	Orient[1] = Orientation.Y;
	Orient[2] = Orientation.Z;
	Orient[3] = Orientation.W;
	FECEF_Transform::ConvertFromUnreal(Pos, Orient);
	ComposePoses(Pos, Orient, OriginPosInEcef, OriginOrientInEcef, Pose->Position, Pose->Orientation);
}

static bool ParseLocation(
	const FJsonObject *Location,
	FRotator* ResultOrientation,
	ecef_tools::Location* ResultLocation)
{
	static const FString LatField = TEXT("Latitude");
	static const FString LonField = TEXT("Longitude");
	static const FString AltField = TEXT("Altitude");
	static const FString PitchField = TEXT("Pitch");
	static const FString YawField = TEXT("Yaw");
	static const FString RollField = TEXT("Roll");
	const FString Fields[] = { LatField, LonField, AltField,
								   PitchField, YawField, RollField };
	double Values[6];
	for (int32 i = 0; i < 6; i++) {
		if (!Location->HasField(Fields[i])) {
			if (i < 3)
			{
				UE_LOG(LogEcefTools, Error, TEXT("Location is missing %s field"), *Fields[i]);
				return false;
			}
			Values[i] = 0;
		}
		else
		{
			Values[i] = Location->GetNumberField(Fields[i]);
			if (i < 2)
			{
				Values[i] = FMath::DegreesToRadians(Values[i]);
			}
		}
	}
	*ResultLocation = ecef_tools::Location(Values[0], Values[1], Values[2], 0.0);
	if (ResultOrientation != nullptr)
	{
		*ResultOrientation = FRotator(Values[3], Values[4], Values[5]);
	}
	return true;
}

static bool ParseEcef(const FJsonObject* Ecef, double* Pos, double* Orient)
{
	static const FString TXField("TX");
	static const FString TYField("TY");
	static const FString TZField("TZ");
	static const FString RXField("RX");
	static const FString RYField("RY");
	static const FString RZField("RZ");
	static const FString RWField("RW");
	const FString Fields[] =
	{
		TXField, TYField, TZField,
		RXField, RYField, RZField, RWField
	};
	for (int32 i = 0; i < 3; i++) {
		if (!Ecef->HasField(Fields[i]))
		{
			UE_LOG(LogEcefTools, Error, TEXT("Missing field: %s"), *Fields[i]);
			return false;
		}
		Pos[i] = Ecef->GetNumberField(Fields[i]);
	}
	for (int32 i = 0, j = 3; i < 4; i++, j++) {
		if (!Ecef->HasField(Fields[j]))
		{
			UE_LOG(LogEcefTools, Error, TEXT("Missing field: %s"), *Fields[j]);
			return false;
		}
		Orient[i] = Ecef->GetNumberField(Fields[j]);
	}
	return true;
}

static bool StringToLocation(
	const FString& Str,
	FRotator* Orient,
	ecef_tools::Location* Result,
	double* EcefPos,
	double* EcefOrient)
{
	TSharedPtr<FJsonObject> JsonObj(new FJsonObject());
	TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create(*Str);
	static const FString LocationField("Location");
	static const FString EcefField("Ecef");
	if (FJsonSerializer::Deserialize(Reader, JsonObj) && JsonObj.IsValid())
	{
		if (JsonObj->HasField(LocationField) && JsonObj->HasField(EcefField))
		{
			if (
				ParseLocation(JsonObj->GetObjectField(LocationField).Get(), Orient, Result) &&
				ParseEcef(JsonObj->GetObjectField(EcefField).Get(), EcefPos, EcefOrient)
				)
			{
				return true;
			}
		}
		else if (ParseLocation(JsonObj.Get(), Orient, Result))
		{
			ecef_tools::LocationToPositionInEcef(Result->latitude, Result->longitude, Result->altitude, EcefPos);
			FQuat Q = Orient->Quaternion();
			EcefOrient[0] = Q.X;
			EcefOrient[1] = Q.Y;
			EcefOrient[2] = Q.Z;
			EcefOrient[3] = Q.W;
			return true;
		}
	}
	return false;
}

static bool
DoConvertECEF_TransformToWorldSpace(
	const double* EcefPos,
	const double* EcefOrient,
	const double* RealWorldPos,
	const double* RealWorldOrient,
	FVector* WorldPosition,
	FRotator* WorldOrientation)
{
	double EcefInvPos[3];
	double EcefInvOrient[4];
	double Pos[3];
	double Orient[4];
	InvertPose(EcefPos, EcefOrient, EcefInvPos, EcefInvOrient);
	ComposePoses(RealWorldPos, RealWorldOrient, EcefInvPos, EcefInvOrient, Pos, Orient);
	FECEF_Transform::ConvertToUnreal(Pos, Orient);
	double Scale = 100.0;
	*WorldOrientation = FRotator(FQuat((float)Orient[0], (float)Orient[1], (float)Orient[2], (float)Orient[3]));
	*WorldPosition = FVector((float)Pos[0], (float)Pos[1], (float)Pos[2]) * Scale;
	return true;
}

static bool DoSetPoseFromLocationString(
	const FString& JsonString,
	double* Position, double* Orientation)
{
	ecef_tools::Location Location;
	double EcefPos[3];
	double EcefOrient[4];
	FRotator Orient;
	if (!StringToLocation(JsonString, &Orient, &Location, EcefPos, EcefOrient))
	{
		return false;
	}
	FQuat Q = Orient.Quaternion();
	Orientation[0] = Q.X;
	Orientation[1] = Q.Y;
	Orientation[2] = Q.Z;
	Orientation[3] = Q.W;
	ecef_tools::LocationToPositionInEcef(Location.latitude, Location.longitude, Location.altitude, Position);
	for (int32 i = 0; i < 3; i++)
	{
		if (!FMath::IsNearlyEqual(Position[i], EcefPos[i]))
		{
			UE_LOG(LogEcefTools, Warning, TEXT("Position %d differs: Loc %.17f != Ecef %.17f"), i, Position[i], EcefPos[i]);
			Position[i] = EcefPos[i];
		}
	}
	for (int32 i = 0; i < 4; i++)
	{
		if (!FMath::IsNearlyEqual((float)Orientation[i], (float)EcefOrient[i]))
		{
			UE_LOG(LogEcefTools, Warning, TEXT("Orientation %d differs: Loc %.17f != Ecef %.17f"), i, Orientation[i], EcefOrient[i]);
			Orientation[i] = EcefOrient[i];
		}
	}
	return true;
}

void UEcefToolsLibrary::SaveECEF_Transform(
	const FECEF_Transform& Pose,
	FString& SavedPose)
{
	FString Location;
	DoConvertEcefToLocation(Pose.Position, Pose.Orientation, &Location);
	FString Result = "{\"Location\":" + Location + ", \"Ecef\":{";
	FString Sep("");
	AddField(Result, FString("TX"), Pose.Position[0], Sep);
	AddField(Result, FString("TY"), Pose.Position[1], Sep);
	AddField(Result, FString("TZ"), Pose.Position[2], Sep);
	AddField(Result, FString("RX"), Pose.Orientation[0], Sep);
	AddField(Result, FString("RY"), Pose.Orientation[1], Sep);
	AddField(Result, FString("RZ"), Pose.Orientation[2], Sep);
	AddField(Result, FString("RW"), Pose.Orientation[3], Sep);
	Result += "}}";
	SavedPose = Result;
}

void UEcefToolsLibrary::LoadECEF_Transform(
	const FString& SavedPose,
	FECEF_Transform& Pose,
	bool& bSucceeded)
{
	double Pos[3];
	double Orient[4];
	bSucceeded = DoSetPoseFromLocationString(SavedPose, Pos, Orient);
	if (bSucceeded)
	{
		Pose = FECEF_Transform(Pos, Orient);
	}
}

void UEcefToolsLibrary::ECEF_FromWorldSpace(
	const FTransform &WorldTransform,
	FECEF_Transform& ECEF_Transform)
{
	::ECEF_FromWorldSpace(WorldSpaceOrigin.Position, WorldSpaceOrigin.Orientation,
		WorldTransform.GetTranslation(), WorldTransform.GetRotation(),
		&ECEF_Transform);
}

void UEcefToolsLibrary::WorldSpaceFromECEF(
	const FECEF_Transform& Pose,
	FTransform &WorldTransform)
{
	FVector Position;
	FRotator Orientation;
	DoConvertECEF_TransformToWorldSpace(
		WorldSpaceOrigin.Position, WorldSpaceOrigin.Orientation,
		Pose.Position, Pose.Orientation,
		&Position, &Orientation);
	WorldTransform = FTransform(Orientation, Position);
}

void FECEF_Transform::GetLocation(double* Latitude, double* Longitude, double* Altitude, double* Heading)
{
	ecef_tools::Location Location;
	AdfPoseInEcefToLocation(Position, Orientation, &Location);
	*Latitude = FMath::RadiansToDegrees(Location.latitude);
	*Longitude = FMath::RadiansToDegrees(Location.longitude);
	*Altitude = Location.altitude;
	*Heading = FMath::RadiansToDegrees(Location.heading);
}

FString FECEF_Transform::ToString()
{
	FString Result;
	UEcefToolsLibrary::SaveECEF_Transform(*this, Result);
	return Result;
}

void UEcefToolsLibrary::FromLocation(
	float Latitude, float Longitude, float Altitude,
	const FRotator& Orientation,
	FECEF_Transform& Pose
)
{
	ecef_tools::LocationToPositionInEcef(
		FMath::DegreesToRadians(Latitude),
		FMath::DegreesToRadians(Longitude),
		Altitude,
		Pose.Position
	);
	FQuat Quat = Orientation.Quaternion();
	Pose.Orientation[0] = Quat.Z;
	Pose.Orientation[1] = Quat.Y;
	Pose.Orientation[2] = Quat.X;
	Pose.Orientation[3] = Quat.W;
}

void UEcefToolsLibrary::InvertECEF_Transform(const FECEF_Transform& Pose, FECEF_Transform& InvertedPose)
{
	InvertPose(Pose.Position, Pose.Orientation, InvertedPose.Position, InvertedPose.Orientation);
}

void UEcefToolsLibrary::ComposeECEF_Transforms(
	const FECEF_Transform& Pose1, const FECEF_Transform& Pose2, FECEF_Transform& ComposedPose
)
{
	ComposePoses(
		Pose2.Position, Pose2.Orientation,
		Pose1.Position, Pose1.Orientation,
		ComposedPose.Position, ComposedPose.Orientation);
}

bool UEcefToolsLibrary::Equals(const FECEF_Transform& Pose1, const FECEF_Transform& Pose2)
{
	for (int i = 0; i < 3; i++)
	{
		if (Pose1.Position[i] != Pose2.Position[i])
		{
			return false;
		}
	}
	for (int i = 0; i < 4; i++)
	{
		if (Pose1.Orientation[i] != Pose2.Orientation[i])
		{
			return false;
		}
	}
	return true;
}
