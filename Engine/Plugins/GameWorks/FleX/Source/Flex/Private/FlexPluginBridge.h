#pragma once

#include "GameWorks/IFlexPluginBridge.h"


class FFlexPluginBridge : public IFlexPluginBridge
{
public:
	virtual void ReImportAsset(class UFlexAsset* FlexAsset, class UStaticMesh* StaticMesh);

private:
};
