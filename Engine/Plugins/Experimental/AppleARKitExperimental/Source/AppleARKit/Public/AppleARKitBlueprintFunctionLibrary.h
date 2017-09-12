#pragma once

// UE4
#include "Kismet/BlueprintFunctionLibrary.h"

// AppleARKit
#include "AppleARKitCamera.h"
#include "AppleARKitBlueprintFunctionLibrary.generated.h"

UCLASS()
class APPLEARKITEXPERIMENTAL_API UAppleARKitBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	/** 
	 * Returns the module singleton UAppleARKitSession create and destroyed during StartupModule &
	 * ShutdownModule respectively.
	 */
	UFUNCTION( BlueprintPure, Category="AppleARKit|Session" )
	static UAppleARKitSession* GetSession();

	/** Returns the ImageResolution aspect ration (Width / Height) */
	UFUNCTION( BlueprintPure, Category="AppleARKit|Camera" )
	static float GetAspectRatio( const FAppleARKitCamera& Camera );

	/** Returns the horizonal FOV of the camera on this frame in degrees. */
	UFUNCTION( BlueprintPure, Category="AppleARKit|Camera" )
	static float GetHorizontalFieldOfView( const FAppleARKitCamera& Camera );

	/** Returns the vertical FOV of the camera on this frame in degrees. */
	UFUNCTION( BlueprintPure, Category="AppleARKit|Camera" )
	static float GetVerticalFieldOfView( const FAppleARKitCamera& Camera );

	/** Returns the effective horizontal field of view for the screen dimensions and fit mode in those dimensions */
	UFUNCTION( BlueprintPure, Category="AppleARKit|Camera" )
	static float GetHorizontalFieldOfViewForScreen( const FAppleARKitCamera& Camera, EAppleARKitBackgroundFitMode BackgroundFitMode, float ScreenWidth, float ScreenHeight );
};
