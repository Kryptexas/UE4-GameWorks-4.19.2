/* Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EcefToolsLibrary.generated.h"

USTRUCT(BlueprintType)
struct FECEF_Transform
{
	GENERATED_USTRUCT_BODY();
	double Position[3]; // Ecef Coordinates
	double Orientation[4];
	FECEF_Transform()
	{
		Position[0] = 0;
		Position[1] = 0;
		Position[2] = 0;
		Orientation[0] = 0;
		Orientation[1] = 0;
		Orientation[2] = 0;
		Orientation[3] = 1;
	}
	FECEF_Transform(const double* InEcefPosition, const double* InAdfOrientation)
	{
		Position[0] = InEcefPosition[0];
		Position[1] = InEcefPosition[1];
		Position[2] = InEcefPosition[2];
		Orientation[0] = InAdfOrientation[0];
		Orientation[1] = InAdfOrientation[1];
		Orientation[2] = InAdfOrientation[2];
		Orientation[3] = InAdfOrientation[3];
	}
	FString ToString();
	void GetLocation(double* OutLatitude, double* OutLongitude, double* OutAltitude, double* OutHeading);
	void CopyTo(double* OutPosition, double* OutOrientation)
	{
		for (int32 i = 0; i < 3; i++)
		{
			OutPosition[i] = Position[i];
			OutOrientation[i] = Orientation[i];
		}
		OutOrientation[3] = Orientation[3];
	}
	// Convert WGS84 to Unreal coordinate conventions
	FECEF_Transform ToUnreal() const
	{
		return ToFromUnreal();
	}
	// Convert Unreal to WGS84 coordinate conventions
	FECEF_Transform FromUnreal() const
	{
		return ToFromUnreal();
	}
	static void ConvertToUnreal(double* InOutPosition, double* InOutOrientation)
	{
		FECEF_Transform(InOutPosition, InOutOrientation).ToUnreal().CopyTo(InOutPosition, InOutOrientation);
	}

	static void ConvertFromUnreal(double* InOutPosition, double* InOutOrientation)
	{
		FECEF_Transform(InOutPosition, InOutOrientation).FromUnreal().CopyTo(InOutPosition, InOutOrientation);
	}

private:
	// @TODO: this conversion matches the bugs in Tango support lib which returns
	// the IMU coordinate space for GLOBAL_WGS84 for TANGO_SUPPORT_ENGINE_XXX
	// conversions
	FECEF_Transform ToFromUnreal() const
	{
		const FECEF_Transform& ECEF_Transform = *this;
		const double OutPosition[3] =
		{
			-ECEF_Transform.Position[2],
			ECEF_Transform.Position[1],
			-ECEF_Transform.Position[0]
		};
		const double OutOrientation[4] =
		{
			-ECEF_Transform.Orientation[2],
			ECEF_Transform.Orientation[1],
			-ECEF_Transform.Orientation[0],
			ECEF_Transform.Orientation[3]
		};
		return FECEF_Transform(OutPosition, OutOrientation);
	}
};


UCLASS()
class ECEFTOOLS_API UEcefToolsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * Serialize an ECEF_Transform.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tango|WorldCoordinate (ECEF)", BlueprintPure)
	static void SaveECEF_Transform(
		const FECEF_Transform& ECEF_Transform,
		FString& SavedECEF_Transform);

	/**
	* Deserialize an ECEF_Transform.
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|WorldCoordinate (ECEF)", BlueprintPure)
	static void LoadECEF_Transform(
		const FString& SavedECEF_Transform,
		FECEF_Transform& ECEF_Transform,
		bool& bSucceeded);

	/**
	* Convert a world space transform to an ECEF_Transform.
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|WorldCoordinate (ECEF)", BlueprintPure)
	static void ECEF_FromWorldSpace(
		const FTransform& WorldTransform,
		FECEF_Transform& ECEF_Transform);

	/**
	* Convert an ECEF_Transform to a world space transform.
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|WorldCoordinate (ECEF)", BlueprintPure)
	static void WorldSpaceFromECEF(
		const FECEF_Transform& ECEF_Transform,
		FTransform& WorldTransform);

	/**
	* Converts a Geo location to an ECEF_Transform. Orientation will be copied to the ECEF_Transform.
	* Altitude is in meters. Latitude and Longitude are in degrees.
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|WorldCoordinate (ECEF)", BlueprintPure)
	static void FromLocation(
		float Latitude,
		float Longitude,
		float Altitude,
		const FRotator& Orientation,
		FECEF_Transform& ECEF_Transform);

	/**
	* Inverts an ECEF_Transform
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|WorldCoordinate (ECEF)", BlueprintPure)
	static void InvertECEF_Transform(const FECEF_Transform& ECEF_Transform, FECEF_Transform& InvertedECEF_Transform);

	/**
	* Composes ECEF_Transform2 after ECEF_Transform1
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|WorldCoordinate (ECEF)", BlueprintPure)
	static void ComposeECEF_Transforms(const FECEF_Transform& ECEF_Transform1, const FECEF_Transform& ECEF_Transform2, FECEF_Transform& ComposedECEF_Transform);

	/**
	* Returns whether two ECEF_Transforms are the same.
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|WorldCoordinate (ECEF)", BlueprintPure)
	static bool Equals(const FECEF_Transform& ECEF_Transform1, const FECEF_Transform& ECEF_Transform2);

	/**
	* Returns the inverse of the current ECEF origin of the engine's world space
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|WorldCoordinate (ECEF)", BlueprintPure)
	static FECEF_Transform GetWorldSpaceOriginInverse()
	{
		return WorldSpaceOriginInverse;
	}

	/**
	* Returns the current ECEF origin of the engine's world space
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|WorldCoordinate (ECEF)", BlueprintPure)
	static FECEF_Transform GetWorldSpaceOrigin()
	{
		return WorldSpaceOrigin;
	}

	/**
	* Sets the ECEF origin of the engine's world space
	*/
	UFUNCTION(BlueprintCallable, Category = "Tango|WorldCoordinate (ECEF)")
	static void SetWorldSpaceOrigin(const FECEF_Transform& InWorldSpaceOrigin)
	{
		WorldSpaceOrigin = InWorldSpaceOrigin;
		InvertECEF_Transform(WorldSpaceOrigin, WorldSpaceOriginInverse);
	}

private:
	static FECEF_Transform WorldSpaceOrigin;
	static FECEF_Transform WorldSpaceOriginInverse;
};
