#pragma once

class IFlexPluginBridge
{
public:
	virtual void ReImportAsset(class UFlexAsset* FlexAsset, class UStaticMesh* StaticMesh) = 0;

};

extern ENGINE_API class IFlexPluginBridge* GFlexPluginBridge;
