#include "EnginePrivate.h"
#include "Landscape/LandscapeInfo.h"
#include "Landscape/LandscapeLayerInfoObject.h"

ENGINE_API FLandscapeInfoLayerSettings::FLandscapeInfoLayerSettings(ULandscapeLayerInfoObject* InLayerInfo, class ALandscapeProxy* InProxy)
: LayerInfoObj(InLayerInfo)
, LayerName((InLayerInfo != NULL) ? InLayerInfo->LayerName : NAME_None)
#if WITH_EDITORONLY_DATA
, ThumbnailMIC(NULL)
, Owner(InProxy)
, DebugColorChannel(0)
, bValid(false)
#endif
{ }