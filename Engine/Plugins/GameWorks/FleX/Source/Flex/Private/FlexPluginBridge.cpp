#include "FlexPluginBridge.h"

#include "FlexAsset.h"
#include "Engine/StaticMesh.h"


void FFlexPluginBridge::ReImportAsset(class UFlexAsset* FlexAsset, class UStaticMesh* StaticMesh)
{
	if (FlexAsset)
	{
		FlexAsset->ReImport(StaticMesh);
	}
}
