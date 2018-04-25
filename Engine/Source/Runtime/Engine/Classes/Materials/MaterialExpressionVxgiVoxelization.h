// NVCHANGE_BEGIN: Add VXGI

#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionVxgiVoxelization.generated.h"

UCLASS(collapsecategories, hidecategories=Object)
class UMaterialExpressionVxgiVoxelization : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	// Begin UMaterialExpression Interface
#if WITH_EDITOR
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual bool NeedsRealtimePreview() override { return true; }
	virtual uint32 GetOutputType(int32 OutputIndex) { return MCT_Float; }
#endif
	// End UMaterialExpression Interface
};

// NVCHANGE_END: Add VXGI
