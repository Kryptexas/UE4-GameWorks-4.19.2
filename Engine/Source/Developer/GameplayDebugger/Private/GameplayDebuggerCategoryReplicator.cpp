// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "GameplayDebuggerPrivatePCH.h"
#include "GameplayDebuggerCategoryReplicator.h"
#include "GameplayDebuggerAddonManager.h"
#include "GameplayDebuggerPlayerManager.h"
#include "GameplayDebuggerRenderingComponent.h"
#include "Net/UnrealNetwork.h"

//////////////////////////////////////////////////////////////////////////
// FGameplayDebuggerCategoryReplicatorData

DEFINE_LOG_CATEGORY_STATIC(LogGameplayDebugReplication, Display, All);

class FNetFastCategoryBaseState : public INetDeltaBaseState
{
public:
	struct FCategoryState
	{
		int32 BaseRepCounter;
		int32 TextLinesRepCounter;
		int32 ShapesRepCounter;
		TArray<int32> DataPackRepCounters;

		bool operator==(const FCategoryState& OtherState) const
		{
			return (BaseRepCounter == OtherState.BaseRepCounter) &&
				(TextLinesRepCounter == OtherState.TextLinesRepCounter) &&
				(ShapesRepCounter == OtherState.ShapesRepCounter) &&
				(DataPackRepCounters == OtherState.DataPackRepCounters);
		}
	};

	virtual bool IsStateEqual(INetDeltaBaseState* OtherState) override
	{
		FNetFastCategoryBaseState* Other = static_cast<FNetFastCategoryBaseState*>(OtherState);
		return (CategoryStates == Other->CategoryStates);
	}

	TArray<FCategoryState> CategoryStates;
};

bool FGameplayDebuggerNetPack::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
{
	if (DeltaParms.bUpdateUnmappedObjects || Owner == nullptr)
	{
		return true;
	}

	const uint32 DataPackMarker = 0x11223344;

	if (DeltaParms.Writer)
	{
		FBitWriter& Writer = *DeltaParms.Writer;
		int32 NumChangedCategories = 0;

		FNetFastCategoryBaseState* OldState = static_cast<FNetFastCategoryBaseState*>(DeltaParms.OldState);
		FNetFastCategoryBaseState* NewState = nullptr;
		TArray<uint8> ChangedCategories;

		// find delta to replicate
		if (Owner->bIsEnabled && Owner->Categories.Num() == SavedData.Num())
		{
			NewState = new FNetFastCategoryBaseState();
			check(DeltaParms.NewState);
			*DeltaParms.NewState = TSharedPtr<INetDeltaBaseState>(NewState);
			NewState->CategoryStates.SetNum(SavedData.Num());
			ChangedCategories.AddZeroed(SavedData.Num());

			for (int32 Idx = 0; Idx < SavedData.Num(); Idx++)
			{
				FGameplayDebuggerCategory& CategoryOb = Owner->Categories[Idx].Get();
				FCategoryData& SavedCategory = SavedData[Idx];
				const bool bMissingOldState = (OldState == nullptr) || !OldState->CategoryStates.IsValidIndex(Idx);
				ChangedCategories[Idx] = bMissingOldState ? 1 : 0;

				if (bMissingOldState || (SavedCategory.bIsEnabled != CategoryOb.bIsEnabled))
				{
					SavedCategory.bIsEnabled = CategoryOb.bIsEnabled;
					NewState->CategoryStates[Idx].BaseRepCounter = bMissingOldState ? 1 : (OldState->CategoryStates[Idx].BaseRepCounter + 1);
					ChangedCategories[Idx]++;
				}

				if (bMissingOldState || (SavedCategory.TextLines != CategoryOb.ReplicatedLines))
				{
					SavedCategory.TextLines = CategoryOb.ReplicatedLines;
					NewState->CategoryStates[Idx].TextLinesRepCounter = bMissingOldState ? 1 : (OldState->CategoryStates[Idx].TextLinesRepCounter + 1);
					ChangedCategories[Idx]++;
				}

				if (bMissingOldState || (SavedCategory.Shapes != CategoryOb.ReplicatedShapes))
				{
					SavedCategory.Shapes = CategoryOb.ReplicatedShapes;
					NewState->CategoryStates[Idx].ShapesRepCounter = bMissingOldState ? 1 : (OldState->CategoryStates[Idx].ShapesRepCounter + 1);
					ChangedCategories[Idx]++;
				}

				const int32 NumDataPacks = CategoryOb.ReplicatedDataPacks.Num();
				SavedCategory.DataPacks.SetNum(NumDataPacks);
				NewState->CategoryStates[Idx].DataPackRepCounters.SetNum(NumDataPacks);
				for (int32 DataIdx = 0; DataIdx < CategoryOb.ReplicatedDataPacks.Num(); DataIdx++)
				{
					if (bMissingOldState || !(SavedCategory.DataPacks[DataIdx] == CategoryOb.ReplicatedDataPacks[DataIdx].Header))
					{
						SavedCategory.DataPacks[DataIdx] = CategoryOb.ReplicatedDataPacks[DataIdx].Header;
						NewState->CategoryStates[Idx].DataPackRepCounters[DataIdx] =
							bMissingOldState || !OldState->CategoryStates[Idx].DataPackRepCounters.IsValidIndex(DataIdx) ? 1 : (OldState->CategoryStates[Idx].DataPackRepCounters[DataIdx] + 1);

						ChangedCategories[Idx]++;
					}
				}

				NumChangedCategories += ChangedCategories[Idx] ? 1 : 0;
			}
		}

		if (NumChangedCategories == 0)
		{
			return false;
		}

		int32 CategoryCount = SavedData.Num();
		Writer << CategoryCount;

		UE_LOG(LogGameplayDebugReplication, Verbose, TEXT("NetDeltaSerialize WRITE START, OldState:%d"), OldState ? 1 : 0);
		for (int32 Idx = 0; Idx < SavedData.Num(); Idx++)
		{
			FGameplayDebuggerCategory& CategoryOb = Owner->Categories[Idx].Get();
			const bool bMissingOldState = (OldState == nullptr) || !OldState->CategoryStates.IsValidIndex(Idx);
			FCategoryData& SavedCategory = SavedData[Idx];

			uint8 BaseFlags = SavedCategory.bIsEnabled;
			uint8 ShouldUpdateTextLines = bMissingOldState || (OldState->CategoryStates[Idx].TextLinesRepCounter != NewState->CategoryStates[Idx].TextLinesRepCounter);
			uint8 ShouldUpdateShapes = bMissingOldState || (OldState->CategoryStates[Idx].ShapesRepCounter != NewState->CategoryStates[Idx].ShapesRepCounter);
			uint8 NumDataPacks = SavedCategory.DataPacks.Num();

			Writer.WriteBit(BaseFlags);
			Writer.WriteBit(ShouldUpdateTextLines);
			Writer.WriteBit(ShouldUpdateShapes);
			Writer << NumDataPacks;

			if (ChangedCategories[Idx])
			{
				UE_LOG(LogGameplayDebugReplication, Verbose, TEXT("  CATEGORY[%d]:%s"), Idx, *CategoryOb.GetCategoryName().ToString());
			}

			if (ShouldUpdateTextLines)
			{
				UE_LOG(LogGameplayDebugReplication, Verbose, TEXT("  >> replicate lines"));
				Writer << SavedCategory.TextLines;
			}

			if (ShouldUpdateShapes)
			{
				UE_LOG(LogGameplayDebugReplication, Verbose, TEXT("  >> replicate shapes"));
				Writer << SavedCategory.Shapes;
			}

			for (int32 DataIdx = 0; DataIdx < NumDataPacks; DataIdx++)
			{
				uint8 ShouldUpdateDataPack = bMissingOldState || !OldState->CategoryStates[Idx].DataPackRepCounters.IsValidIndex(DataIdx) ||
					(OldState->CategoryStates[Idx].DataPackRepCounters[DataIdx] != NewState->CategoryStates[Idx].DataPackRepCounters[DataIdx]);

				Writer.WriteBit(ShouldUpdateDataPack);
				if (ShouldUpdateDataPack)
				{
					// send single packet
					FGameplayDebuggerDataPack& DataPack = CategoryOb.ReplicatedDataPacks[DataIdx];

					uint8 IsCompressed = DataPack.Header.bIsCompressed ? 1 : 0;
					Writer.WriteBit(IsCompressed);

					Writer << DataPack.Header.DataVersion;
					Writer << DataPack.Header.DataSize;
					Writer << DataPack.Header.DataOffset;

					const int32 PacketSize = FMath::Min(FGameplayDebuggerDataPack::PacketSize, DataPack.Header.DataSize - DataPack.Header.DataOffset);
					if (PacketSize > 0)
					{
						Writer.Serialize(DataPack.Data.GetData() + DataPack.Header.DataOffset, PacketSize);
					}

					uint32 Marker = DataPackMarker;
					Writer << Marker;

					UE_LOG(LogGameplayDebugReplication, Verbose, TEXT("  >> replicate data pack[%d] progress:%0.f%% (offset:%d packet:%d)"), DataIdx, DataPack.GetProgress() * 100.0f, DataPack.Header.DataOffset, PacketSize);
				}
			}
		}
	}
	else if (DeltaParms.Reader)
	{
		FBitReader& Reader = *DeltaParms.Reader;

		UE_LOG(LogGameplayDebugReplication, Verbose, TEXT("NetDeltaSerialize READ START"));

		int32 CategoryCount = 0;
		Reader << CategoryCount;

		if (CategoryCount != Owner->Categories.Num())
		{
			UE_LOG(LogGameplayDebugReplication, Error, TEXT("Category count mismtach! received:%d expected:%d"), CategoryCount, Owner->Categories.Num());
			Reader.SetError();
			return false;
		}

		for (int32 Idx = 0; Idx < CategoryCount; Idx++)
		{
			FGameplayDebuggerCategory& CategoryOb = Owner->Categories[Idx].Get();
			UE_LOG(LogGameplayDebugReplication, Verbose, TEXT("  CATEGORY[%d]:%s"), Idx, *CategoryOb.GetCategoryName().ToString());

			uint8 BaseFlags = Reader.ReadBit();
			uint8 ShouldUpdateTextLines = Reader.ReadBit();
			uint8 ShouldUpdateShapes = Reader.ReadBit();

			uint8 NumDataPacks = 0;
			Reader << NumDataPacks;

			if ((int32)NumDataPacks != CategoryOb.ReplicatedDataPacks.Num())
			{
				UE_LOG(LogGameplayDebugReplication, Error, TEXT("Data pack count mismtach! received:%d expected:%d"), NumDataPacks, CategoryOb.ReplicatedDataPacks.Num());
				Reader.SetError();
				return false;
			}

			CategoryOb.bIsEnabled = (BaseFlags != 0);

			if (ShouldUpdateTextLines)
			{
				UE_LOG(LogGameplayDebugReplication, Verbose, TEXT("  >> received lines"));
				Reader << CategoryOb.ReplicatedLines;
			}

			if (ShouldUpdateShapes)
			{
				UE_LOG(LogGameplayDebugReplication, Verbose, TEXT("  >> received shapes"));
				Reader << CategoryOb.ReplicatedShapes;
			}

			for (uint8 DataIdx = 0; DataIdx < NumDataPacks; DataIdx++)
			{
				uint8 ShouldUpdateDataPack = Reader.ReadBit();

				if (ShouldUpdateDataPack)
				{
					// receive single packet
					FGameplayDebuggerDataPack DataPacket;

					uint8 IsCompressed = Reader.ReadBit();
					DataPacket.Header.bIsCompressed = (IsCompressed != 0);

					Reader << DataPacket.Header.DataVersion;
					Reader << DataPacket.Header.DataSize;
					Reader << DataPacket.Header.DataOffset;

					const int32 PacketSize = FMath::Min(FGameplayDebuggerDataPack::PacketSize, DataPacket.Header.DataSize - DataPacket.Header.DataOffset);
					if (PacketSize > 0)
					{
						DataPacket.Data.AddUninitialized(PacketSize);
						Reader.Serialize(DataPacket.Data.GetData(), PacketSize);
					}

					uint32 Marker = 0;
					Reader << Marker;

					if (Marker != DataPackMarker)
					{				
						UE_LOG(LogGameplayDebugReplication, Error, TEXT("Data pack[%d] corrupted! marker:%X expected:%X"), DataIdx, Marker, DataPackMarker);
						Reader.SetError();
						return false;
					}

					Owner->OnReceivedDataPackPacket(Idx, DataIdx, DataPacket);
					UE_LOG(LogGameplayDebugReplication, Verbose, TEXT("  >> replicate data pack[%d] progress:%.0f%%"), DataIdx, CategoryOb.ReplicatedDataPacks[DataIdx].GetProgress() * 100.0f);
				}
			}
		}
	}

	return true;
}

void FGameplayDebuggerNetPack::OnCategoriesChanged()
{
	SavedData.Reset();
	SavedData.SetNum(Owner->Categories.Num());
}

//////////////////////////////////////////////////////////////////////////
// AGameplayDebuggerCategoryReplicator

AGameplayDebuggerCategoryReplicator::AGameplayDebuggerCategoryReplicator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

#if WITH_EDITOR
	SetIsTemporarilyHiddenInEditor(true);
#endif

#if WITH_EDITORONLY_DATA
	bHiddenEdLevel = true;
	bHiddenEdLayer = true;
	bHiddenEd = true;
	bEditable = false;
#endif

	bIsEnabled = false;
	bReplicates = true;

	ReplicatedData.Owner = this;
}

void AGameplayDebuggerCategoryReplicator::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	const ENetMode NetMode = World->GetNetMode();
	bHasAuthority = (NetMode != NM_Client);
	bIsLocal = (NetMode != NM_DedicatedServer);

	FGameplayDebuggerAddonManager& CategoryManager = FGameplayDebuggerAddonManager::GetCurrent();
	CategoryManager.OnCategoriesChanged.AddUObject(this, &AGameplayDebuggerCategoryReplicator::OnCategoriesChanged);
	
	OnCategoriesChanged();

	AGameplayDebuggerPlayerManager& PlayerManager = AGameplayDebuggerPlayerManager::GetCurrent(GetWorld());
	PlayerManager.RegisterReplicator(*this);

	SetActorHiddenInGame(!bIsLocal);
	if (bIsLocal)
	{
		RenderingComp = NewObject<UGameplayDebuggerRenderingComponent>(this, TEXT("RenderingComp"));
		RenderingComp->RegisterComponentWithWorld(World);
		RootComponent = RenderingComp;
	}
	
	if (bHasAuthority)
	{
		SetEnabled(FGameplayDebuggerAddonBase::IsSimulateInEditor());
	}
}

void AGameplayDebuggerCategoryReplicator::Destroyed()
{
	Super::Destroyed();

	FGameplayDebuggerAddonManager& CategoryManager = FGameplayDebuggerAddonManager::GetCurrent();
	CategoryManager.OnCategoriesChanged.RemoveAll(this);
}

void AGameplayDebuggerCategoryReplicator::OnCategoriesChanged()
{
	FGameplayDebuggerAddonManager& CategoryManager = FGameplayDebuggerAddonManager::GetCurrent();
	CategoryManager.CreateCategories(*this, Categories);

	ReplicatedData.OnCategoriesChanged();

	if (bIsLocal)
	{
		AGameplayDebuggerPlayerManager& PlayerManager = AGameplayDebuggerPlayerManager::GetCurrent(GetWorld());
		PlayerManager.RefreshInputBindings(*this);
	}
}

UNetConnection* AGameplayDebuggerCategoryReplicator::GetNetConnection() const
{
	return IsValid(OwnerPC) ? OwnerPC->GetNetConnection() : nullptr;
}

bool AGameplayDebuggerCategoryReplicator::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	return (RealViewer == OwnerPC);
}

void AGameplayDebuggerCategoryReplicator::PostNetReceive()
{
	Super::PostNetReceive();

	if (PendingReplicationRequests.Num() && !bHasAuthority)
	{
		ServerRequestDataPackets(PendingReplicationRequests);
		PendingReplicationRequests.Reset();
	}
}

void AGameplayDebuggerCategoryReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGameplayDebuggerCategoryReplicator, OwnerPC);
	DOREPLIFETIME(AGameplayDebuggerCategoryReplicator, DebugActor);
	DOREPLIFETIME(AGameplayDebuggerCategoryReplicator, bIsEnabled);
	DOREPLIFETIME(AGameplayDebuggerCategoryReplicator, ReplicatedData);
}

bool AGameplayDebuggerCategoryReplicator::ServerSetEnabled_Validate(bool bEnable)
{
	return true;
}

void AGameplayDebuggerCategoryReplicator::ServerSetEnabled_Implementation(bool bEnable)
{
	SetEnabled(bEnable);
}

bool AGameplayDebuggerCategoryReplicator::ServerSetDebugActor_Validate(AActor* Actor)
{
	return true;
}

void AGameplayDebuggerCategoryReplicator::ServerSetDebugActor_Implementation(AActor* Actor)
{
	SetDebugActor(Actor);
}

bool AGameplayDebuggerCategoryReplicator::ServerSetCategoryEnabled_Validate(int32 CategoryId, bool bEnable)
{
	return true;
}

void AGameplayDebuggerCategoryReplicator::ServerSetCategoryEnabled_Implementation(int32 CategoryId, bool bEnable)
{
	SetCategoryEnabled(CategoryId, bEnable);
}

bool AGameplayDebuggerCategoryReplicator::ServerRequestDataPackets_Validate(const TArray<FGameplayDebuggerDataPacketRequest>& RequestList)
{
	return true;
}

void AGameplayDebuggerCategoryReplicator::ServerRequestDataPackets_Implementation(const TArray<FGameplayDebuggerDataPacketRequest>& RequestList)
{
	for (int32 Idx = 0; Idx < RequestList.Num(); Idx++)
	{
		const FGameplayDebuggerDataPacketRequest& Request = RequestList[Idx];
		if (Categories.IsValidIndex(Request.CategoryId) && Categories[Request.CategoryId]->ReplicatedDataPacks.IsValidIndex(Request.DataPackId))
		{
			Categories[Request.CategoryId]->ReplicatedDataPacks[Request.DataPackId].OnPacketRequest(Request.DataVersion, Request.DataOffset);
		}
	}
}

void AGameplayDebuggerCategoryReplicator::OnReceivedDataPackPacket(int32 CategoryId, int32 DataPackId, const FGameplayDebuggerDataPack& DataPacket)
{
	if (Categories.IsValidIndex(CategoryId) && Categories[CategoryId]->ReplicatedDataPacks.IsValidIndex(DataPackId))
	{
		FGameplayDebuggerDataPack& DataPack = Categories[CategoryId]->ReplicatedDataPacks[DataPackId];
		bool bIsPacketValid = true;

		if (DataPack.Header.DataVersion != DataPacket.Header.DataVersion)
		{
			// new content of data pack:
			if (DataPacket.Header.DataOffset == 0)
			{
				// first packet of data, replace old data pack's intermediate data
				DataPack.Header = DataPacket.Header;
				DataPack.Data = DataPacket.Data;
			}
			else
			{
				// somewhere in the middle, discard packet
				UE_LOG(LogGameplayDebugReplication, Verbose, TEXT("Error: received packet from the middle of content with different version, discarding! (category[%d]:%s, packet.DataVersion:%d packet.DataOffset:%d, data[%d].DataVersion:%d)"),
					CategoryId, *Categories[CategoryId]->GetCategoryName().ToString(),
					DataPacket.Header.DataVersion,
					DataPacket.Header.DataOffset,
					DataPackId, DataPack.Header.DataVersion,
					DataPackId, DataPack.Header.DataOffset);

				bIsPacketValid = false;
			}
		}
		else
		{
			// another packet for existing data pack
			if (DataPacket.Header.DataOffset == DataPack.Data.Num())
			{
				// offset matches, this is next expected packet
				DataPack.Data.Append(DataPacket.Data);
				DataPack.Header.DataOffset = DataPack.Data.Num();
			}
			else
			{
				// offset mismatch, discard packet
				UE_LOG(LogGameplayDebugReplication, Verbose, TEXT("Error: received packet doesn't match expected chunk, discarding! (category[%d]:%s, packet.DataOffset:%d, data[%d].DataOffset:%d data[%d].Data.Num:%d)"),
					CategoryId, *Categories[CategoryId]->GetCategoryName().ToString(),
					DataPacket.Header.DataOffset,
					DataPackId, DataPack.Header.DataOffset,
					DataPackId, DataPack.Data.Num());

				bIsPacketValid = false;
			}
		}

		// check if data pack is now complete
		if (bIsPacketValid)
		{
			bool bCreatePacketRequest = true;
			if (DataPack.Data.Num() == DataPack.Header.DataSize)
			{
				// complete
				UE_LOG(LogGameplayDebugReplication, Verbose, TEXT("Category[%d].DataPack[%d] RECEIVED, DataVersion:%d DataSize:%d"),
					CategoryId, DataPackId, DataPack.Header.DataVersion, DataPack.Header.DataSize);

				// larger data packs will wait on server until receiving confirmation
				bCreatePacketRequest = (DataPack.Header.DataSize > FGameplayDebuggerDataPack::PacketSize);

				DataPack.OnReplicated();
				Categories[CategoryId]->OnDataPackReplicated(DataPackId);
			}

			if (bCreatePacketRequest)
			{
				FGameplayDebuggerDataPacketRequest NewRequest;
				NewRequest.CategoryId = CategoryId;
				NewRequest.DataPackId = DataPackId;
				NewRequest.DataVersion = DataPack.Header.DataVersion;
				NewRequest.DataOffset = DataPack.Data.Num();

				PendingReplicationRequests.Add(NewRequest);
			}
		}
	}
}

void AGameplayDebuggerCategoryReplicator::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);
	
	const float GameTime = GetWorld()->GetTimeSeconds();
	for (int32 Idx = 0; Idx < Categories.Num(); Idx++)
	{
		FGameplayDebuggerCategory& CategoryOb = Categories[Idx].Get();
		if (CategoryOb.bHasAuthority && CategoryOb.bIsEnabled && ((GameTime - CategoryOb.LastCollectDataTime) > CategoryOb.CollectDataInterval))
		{
			// prepare data packs before calling CollectData
			for (int32 DataPackIdx = 0; DataPackIdx < CategoryOb.ReplicatedDataPacks.Num(); DataPackIdx++)
			{
				FGameplayDebuggerDataPack& DataPack = CategoryOb.ReplicatedDataPacks[DataPackIdx];
				DataPack.bIsDirty = false;

				if (DataPack.Flags == EGameplayDebuggerDataPack::AutoResetData)
				{
					DataPack.ResetDelegate.Execute();
				}
			}

			CategoryOb.ReplicatedLines.Reset();
			CategoryOb.ReplicatedShapes.Reset();

			CategoryOb.CollectData(OwnerPC, DebugActor);

			// update dirty data packs
			for (int32 DataPackIdx = 0; DataPackIdx < CategoryOb.ReplicatedDataPacks.Num(); DataPackIdx++)
			{
				FGameplayDebuggerDataPack& DataPack = CategoryOb.ReplicatedDataPacks[DataPackIdx];
				if (CategoryOb.bIsLocal)
				{
					const bool bWasDirty = DataPack.CheckDirtyAndUpdate();
					if (bWasDirty)
					{
						CategoryOb.OnDataPackReplicated(DataPackIdx);
					}
				}
				else
				{
					const bool bWasDirty = DataPack.RequestReplication();
					if (bWasDirty)
					{
						UE_LOG(LogGameplayDebugReplication, Verbose, TEXT("Category[%d].DataPack[%d] SENT, DataVersion:%d DataSize:%d"),
							Idx, DataPackIdx, DataPack.Header.DataVersion, DataPack.Header.DataSize);
					}
				}
			}
		}
	}
}

void AGameplayDebuggerCategoryReplicator::SetReplicatorOwner(APlayerController* InOwnerPC)
{
	if (!bIsEnabled)
	{
		// can't use bHasAuthority, BeginPlay was not called yet
		UWorld* World = GetWorld();
		const ENetMode NetMode = World->GetNetMode();

		if (NetMode != NM_Client)
		{
			OwnerPC = InOwnerPC;
		}
	}
}

void AGameplayDebuggerCategoryReplicator::SetEnabled(bool bEnable)
{
	if (bHasAuthority)
	{
		bIsEnabled = bEnable;
		SetActorTickEnabled(bEnable);
	}
	else
	{
		ServerSetEnabled(bEnable);
	}

	MarkComponentsRenderStateDirty();
}

void AGameplayDebuggerCategoryReplicator::SetDebugActor(AActor* Actor)
{
	if (bHasAuthority)
	{
		DebugActor = Actor;
	}
	else
	{
		ServerSetDebugActor(Actor);
	}
}

void AGameplayDebuggerCategoryReplicator::SetCategoryEnabled(int32 CategoryId, bool bEnable)
{
	if (bHasAuthority)
	{
		if (Categories.IsValidIndex(CategoryId))
		{
			Categories[CategoryId]->bIsEnabled = bEnable;
		}
	}
	else
	{
		ServerSetCategoryEnabled(CategoryId, bEnable);
	}

	MarkComponentsRenderStateDirty();
}

bool AGameplayDebuggerCategoryReplicator::IsCategoryEnabled(int32 CategoryId) const
{
	return Categories.IsValidIndex(CategoryId) && Categories[CategoryId]->IsCategoryEnabled(); 
}
