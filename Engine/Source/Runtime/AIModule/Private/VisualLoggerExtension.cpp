#include "AIModulePrivate.h"
#include "EnvironmentQuery/EnvQueryDebugHelpers.h"
#include "EnvironmentQuery/EQSRenderingComponent.h"
#include "VisualLoggerExtension.h"

#if ENABLE_VISUAL_LOG
FVisualLoggerExtension::FVisualLoggerExtension()
	:  CachedEQSId(INDEX_NONE)
	, CurrentTimestamp(FLT_MIN)
{

}

void FVisualLoggerExtension::OnTimestampChange(float Timestamp, class UWorld* InWorld, class AActor* HelperActor )
{
#if USE_EQS_DEBUGGER
	if (CurrentTimestamp != Timestamp)
	{
		CurrentTimestamp = Timestamp;
		CachedEQSId = INDEX_NONE;
		DisableEQSRendering(HelperActor);
	}
#endif
}

void FVisualLoggerExtension::DisableDrawingForData(class UWorld* InWorld, class UCanvas* Canvas, class AActor* HelperActor, const FName& TagName, const FVisLogEntry::FDataBlock& DataBlock, float Timestamp)
{
#if USE_EQS_DEBUGGER
	if (TagName == *EVisLogTags::TAG_EQS && CurrentTimestamp == Timestamp)
	{
		DisableEQSRendering(HelperActor);
	}
#endif
}

void FVisualLoggerExtension::DisableEQSRendering(class AActor* HelperActor)
{
#if USE_EQS_DEBUGGER
	if (HelperActor)
	{
		UEQSRenderingComponent* EQSRenderComp = HelperActor->FindComponentByClass<UEQSRenderingComponent>();
		if (EQSRenderComp)
		{
			EQSRenderComp->SetHiddenInGame(true);
			EQSRenderComp->Deactivate();
			EQSRenderComp->DebugData.Reset();
			EQSRenderComp->MarkRenderStateDirty();
		}
	}
#endif
}

void FVisualLoggerExtension::DrawData(class UWorld* InWorld, class UCanvas* Canvas, class AActor* HelperActor, const FName& TagName, const FVisLogEntry::FDataBlock& DataBlock, float Timestamp)
{
#if USE_EQS_DEBUGGER
	if (TagName == *EVisLogTags::TAG_EQS && HelperActor)
	{
		UEQSRenderingComponent* EQSRenderComp = HelperActor->FindComponentByClass<UEQSRenderingComponent>();
		if (!EQSRenderComp)
		{
			EQSRenderComp = ConstructObject<UEQSRenderingComponent>(UEQSRenderingComponent::StaticClass(), HelperActor);
			EQSRenderComp->bDrawOnlyWhenSelected = false;
			EQSRenderComp->RegisterComponent();
			EQSRenderComp->SetHiddenInGame(true);
		}

		EQSDebug::FQueryData DebugData;
		UEnvQueryDebugHelpers::BlobArrayToDebugData(DataBlock.Data, DebugData, false);
		if (DebugData.Id != CachedEQSId || (EQSRenderComp && EQSRenderComp->bHiddenInGame))
		{
			CachedEQSId = DebugData.Id;
			EQSRenderComp->DebugData = DebugData;
			EQSRenderComp->Activate();
			EQSRenderComp->SetHiddenInGame(false);
			EQSRenderComp->MarkRenderStateDirty();
		}
	}
#endif
}
#endif //ENABLE_VISUAL_LOG
