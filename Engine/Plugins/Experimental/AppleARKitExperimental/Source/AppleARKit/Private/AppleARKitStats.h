#pragma once

// UE4
#include "Math/Color.h"

FColor GetARKitTrackingTimeDisplayColor( float TrackingIntervalMS )
{
	const float UnacceptableTime = 33.33333; // 30 FPS
	const float TargetTime = 16.66666; // 60 FPS

	return ( TrackingIntervalMS > UnacceptableTime ) ? FColor::Red : ( ( TrackingIntervalMS > TargetTime ) ? FColor::Yellow : FColor::Green );
}
