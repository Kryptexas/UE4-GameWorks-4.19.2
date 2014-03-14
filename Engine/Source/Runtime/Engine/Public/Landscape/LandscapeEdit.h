// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
LandscapeEdit.h: Classes for the editor to access to Landscape data
=============================================================================*/

#ifndef _LANDSCAPEEDIT_H
#define _LANDSCAPEEDIT_H

#if WITH_EDITOR

struct FLandscapeTextureDataInfo
{
	struct FMipInfo
	{
		void* MipData;
		TArray<FUpdateTextureRegion2D> MipUpdateRegions;
	};

	FLandscapeTextureDataInfo(UTexture2D* InTexture);
	virtual ~FLandscapeTextureDataInfo();

	// returns true if we need to block on the render thread before unlocking the mip data
	bool UpdateTextureData();

	int32 NumMips() { return MipInfo.Num(); }

	void AddMipUpdateRegion(int32 MipNum, int32 InX1, int32 InY1, int32 InX2, int32 InY2)
	{
		check( MipNum < MipInfo.Num() );
		new(MipInfo[MipNum].MipUpdateRegions) FUpdateTextureRegion2D(InX1, InY1, InX1, InY1, 1+InX2-InX1, 1+InY2-InY1);
	}

	void* GetMipData(int32 MipNum)
	{
		check( MipNum < MipInfo.Num() );
		if( !MipInfo[MipNum].MipData )
		{
			MipInfo[MipNum].MipData = Texture->Source.LockMip(MipNum);
		}
		return MipInfo[MipNum].MipData;
	}

	int32 GetMipSizeX(int32 MipNum)
	{
		return FMath::Max(Texture->Source.GetSizeX() >> MipNum, 1);
	}

	int32 GetMipSizeY(int32 MipNum)
	{
		return FMath::Max(Texture->Source.GetSizeY() >> MipNum, 1);
	}

private:
	UTexture2D* Texture;
	TArray<FMipInfo> MipInfo;
};

struct ENGINE_API FLandscapeEditDataInterface
{
	// tors
	FLandscapeEditDataInterface(ULandscapeInfo* InLandscape);
	virtual ~FLandscapeEditDataInterface();

	// Misc
	bool GetComponentsInRegion(int32 X1, int32 Y1, int32 X2, int32 Y2, TSet<ULandscapeComponent*>* OutComponents = NULL);

	//
	// Heightmap access
	//
	void SetHeightData(int32 X1, int32 Y1, int32 X2, int32 Y2, const uint16* Data, int32 Stride, bool CalcNormals, const uint16* NormalData = NULL, bool CreateComponents=false);

	// Helper accessor
	FORCEINLINE uint16 GetHeightMapData(const ULandscapeComponent* Component, int32 TexU, int32 TexV, FColor* TextureData = NULL);
	// Generic
	template<typename TStoreData>
	void GetHeightDataTempl(int32& X1, int32& Y1, int32& X2, int32& Y2, TStoreData& StoreData);
	// Without data interpolation, able to get normal data
	template<typename TStoreData>
	void GetHeightDataTemplFast(const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, TStoreData& StoreData, TStoreData* NormalData = NULL);
	// Implementation for fixed array
	void GetHeightData(int32& X1, int32& Y1, int32& X2, int32& Y2, uint16* Data, int32 Stride);
	void GetHeightDataFast(const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, uint16* Data, int32 Stride, uint16* NormalData = NULL);
	// Implementation for sparse array
	void GetHeightData(int32& X1, int32& Y1, int32& X2, int32& Y2, TMap<FIntPoint, uint16>& SparseData);
	void GetHeightDataFast(const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, TMap<FIntPoint, uint16>& SparseData, TMap<FIntPoint, uint16>* NormalData = NULL);

	// Recaclulate normals for the entire landscape.
	void RecalculateNormals();

	//
	// Weightmap access
	//
	// Helper accessor
	FORCEINLINE uint8 GetWeightMapData(const ULandscapeComponent* Component, ULandscapeLayerInfoObject* LayerInfo, int32 TexU, int32 TexV, uint8 Offset = 0, UTexture2D* Texture = NULL, uint8* TextureData = NULL);
	template<typename TStoreData>
	void GetWeightDataTempl(ULandscapeLayerInfoObject* LayerInfo, int32& X1, int32& Y1, int32& X2, int32& Y2, TStoreData& StoreData);
	// Without data interpolation
	template<typename TStoreData>
	void GetWeightDataTemplFast(ULandscapeLayerInfoObject* LayerInfo, const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, TStoreData& StoreData);
	// Implementation for fixed array
	void GetWeightData(ULandscapeLayerInfoObject* LayerInfo, int32& X1, int32& Y1, int32& X2, int32& Y2, uint8* Data, int32 Stride);
	//void GetWeightData(FName LayerName, int32& X1, int32& Y1, int32& X2, int32& Y2, TArray<uint8>* Data, int32 Stride);
	void GetWeightDataFast(ULandscapeLayerInfoObject* LayerInfo, const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, uint8* Data, int32 Stride);
	void GetWeightDataFast(ULandscapeLayerInfoObject* LayerInfo, const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, TArray<uint8>* Data, int32 Stride);
	// Implementation for sparse array
	void GetWeightData(ULandscapeLayerInfoObject* LayerInfo, int32& X1, int32& Y1, int32& X2, int32& Y2, TMap<FIntPoint, uint8>& SparseData);
	//void GetWeightData(FName LayerName, int32& X1, int32& Y1, int32& X2, int32& Y2, TMap<uint64, TArray<uint8>>& SparseData);
	void GetWeightDataFast(ULandscapeLayerInfoObject* LayerInfo, const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, TMap<FIntPoint, uint8>& SparseData);
	void GetWeightDataFast(ULandscapeLayerInfoObject* LayerInfo, const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, TMap<FIntPoint, TArray<uint8>>& SparseData);
	// Updates weightmap for LayerInfo, optionally adjusting all other weightmaps.
	void SetAlphaData(ULandscapeLayerInfoObject* LayerInfo, const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, const uint8* Data, int32 Stride, ELandscapeLayerPaintingRestriction::Type PaintingRestriction = ELandscapeLayerPaintingRestriction::None, bool bWeightAdjust = true, bool bTotalWeightAdjust = false);
	// Updates weightmaps for all layers. Data points to packed data for all layers in the landscape info
	void SetAlphaData(const TSet<ULandscapeLayerInfoObject*>& DirtyLayerInfos, const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, const uint8* Data, int32 Stride, ELandscapeLayerPaintingRestriction::Type PaintingRestriction = ELandscapeLayerPaintingRestriction::None);
	// Delete a layer and re-normalize other layers
	void DeleteLayer(ULandscapeLayerInfoObject* LayerInfo);
	// Replace/merge a layer
	void ReplaceLayer(ULandscapeLayerInfoObject* FromLayerInfo, ULandscapeLayerInfoObject* ToLayerInfo);

	// Without data interpolation, Select Data 
	template<typename TStoreData>
	void GetSelectDataTempl(const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, TStoreData& StoreData);
	void GetSelectData(const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, uint8* Data, int32 Stride);
	void GetSelectData(const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, TMap<FIntPoint, uint8>& SparseData);
	void SetSelectData(const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, const uint8* Data, int32 Stride);

	//
	// XYOffsetmap access
	//
	template<typename T>
	void SetXYOffsetDataTempl(int32 X1, int32 Y1, int32 X2, int32 Y2, const T* Data, int32 Stride);
	void SetXYOffsetData(int32 X1, int32 Y1, int32 X2, int32 Y2, const FVector2D* Data, int32 Stride);
	void SetXYOffsetData(int32 X1, int32 Y1, int32 X2, int32 Y2, const FVector* Data, int32 Stride);
	// Helper accessor
	FORCEINLINE FVector2D GetXYOffsetmapData(const ULandscapeComponent* Component, int32 TexU, int32 TexV, FColor* TextureData = NULL);
	
	// No interpolation supported for XY Offset due to collision problem...
	template<typename TStoreData>
	void GetXYOffsetDataTempl(const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, TStoreData& StoreData);
	// Without data interpolation, able to get normal data
	void GetXYOffsetData(const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, FVector2D* Data, int32 Stride);
	void GetXYOffsetData(const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, TMap<FIntPoint, FVector2D>& SparseData);
	void GetXYOffsetData(const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, FVector* Data, int32 Stride);
	void GetXYOffsetData(const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, TMap<FIntPoint, FVector>& SparseData);

	// Texture data access
	FLandscapeTextureDataInfo* GetTextureDataInfo(UTexture2D* Texture);

	// Flush texture updates
	void Flush();

	// Texture bulk operations for weightmap reallocation
	void CopyTextureChannel(UTexture2D* Dest, int32 DestChannel, UTexture2D* Src, int32 SrcChannel);
	void ZeroTextureChannel(UTexture2D* Dest, int32 DestChannel);
	void CopyTextureFromHeightmap(UTexture2D* Dest, int32 DestChannel, ULandscapeComponent* Comp, int32 SrcChannel);

	template<typename TData>
	void SetTextureValueTempl(UTexture2D* Dest, TData Value);
	void ZeroTexture(UTexture2D* Dest);
	void SetTextureValue(UTexture2D* Dest, FColor Value);

	template<typename TData>
	bool EqualTextureValueTempl(UTexture2D* Src, TData Value);
	bool EqualTextureValue(UTexture2D* Src, FColor Value);

private:
	int32 ComponentSizeQuads;
	int32 SubsectionSizeQuads;
	int32 ComponentNumSubsections;
	FVector DrawScale;

	TMap<UTexture2D*, FLandscapeTextureDataInfo*> TextureDataMap;
	ULandscapeInfo* LandscapeInfo;

	// Only for Missing Data interpolation... only internal usage
	template<typename TData, typename TStoreData>
	FORCEINLINE void CalcMissingValues(const int32& X1, const int32& X2, const int32& Y1, const int32& Y2, 
		const int32& ComponentIndexX1, const int32& ComponentIndexX2, const int32& ComponentIndexY1, const int32& ComponentIndexY2, 
		const int32& ComponentSizeX, const int32& ComponentSizeY, TData* CornerValues, 
		TArray<bool>& NoBorderY1, TArray<bool>& NoBorderY2, TArray<bool>& ComponentDataExist, TStoreData& StoreData);
};

#endif

#endif // _LANDSCAPEEDIT_H
