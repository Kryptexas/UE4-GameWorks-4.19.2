#include "EnginePrivate.h"
#include "Landscape/LandscapeComponent.h"
#include "Landscape/LandscapeLayerInfoObject.h"

FName FWeightmapLayerAllocationInfo::GetLayerName() const
{
	if (LayerInfo)
	{
		return LayerInfo->LayerName;
	}
	return NAME_None;
}