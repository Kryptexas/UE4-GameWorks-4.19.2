#pragma once

struct FSimplygonEnum
{
	static FORCEINLINE void FillEnumOptions(TArray<TSharedPtr<FString> >& OutStrings, UEnum& InEnum, bool UseDiplayText = false)
	{
		//check if enum has dispaly text metadata
		if(!InEnum.HasMetaData(TEXT("DisplayName")))
		{
			//UseDiplayText = false;
		}

		for (int32 EnumIndex = 0; EnumIndex < InEnum.NumEnums() - 1; ++EnumIndex)
		{
			if(!UseDiplayText)
			{
				OutStrings.Add(MakeShareable(new FString(InEnum.GetEnumName(EnumIndex))));
			}
			else
			{
				OutStrings.Add(MakeShareable(new FString(InEnum.GetDisplayNameText(EnumIndex).ToString())));
			}

		}
	}

	static FORCEINLINE void FillEnumOptions(TArray<FString>& OutStrings, UEnum& InEnum, bool UseDiplayText = false)
	{
		//check if enum has dispaly text metadata
		if(!InEnum.HasMetaData(TEXT("DisplayName")))
		{
			//UseDiplayText = false;
		}

		for (int32 EnumIndex = 0; EnumIndex < InEnum.NumEnums() - 1; ++EnumIndex)
		{
			if(!UseDiplayText)
			{
				OutStrings.Add(InEnum.GetEnumName(EnumIndex));
			}
			else
			{
				OutStrings.Add(InEnum.GetDisplayNameText(EnumIndex).ToString());
			}

		}
	}

	static FORCEINLINE UEnum& GetTextureResolutionEnum()
	{ 
		static FName TexResTypeName(TEXT("ESimplygonTextureResolution::TextureResolution_64")); 
		static UEnum* TexResTypeEnum = NULL; 
		if (TexResTypeEnum == NULL) 
		{ 
			UEnum::LookupEnumName(TexResTypeName, &TexResTypeEnum); 
			check(TexResTypeEnum); 
		} 
		return *TexResTypeEnum; 

	}

	static FORCEINLINE UEnum& GetTextureStrechEnum()
	{ 
		static FName TextureStrechName(TEXT("ESimplygonTextureStrech::None")); 
		static UEnum* TextureStrechEnum = NULL; 
		if (TextureStrechEnum == NULL) 
		{ 
			UEnum::LookupEnumName(TextureStrechName, &TextureStrechEnum); 
			check(TextureStrechEnum); 
		} 
		return *TextureStrechEnum; 
	}

	static FORCEINLINE UEnum& GetSamplingQualityEnum()
	{ 
		static FName SamplingQualityName(TEXT("ESimplygonTextureSamplingQuality::Poor")); 
		static UEnum* SamplingQualityEnum = NULL; 
		if (SamplingQualityEnum == NULL) 
		{ 
			UEnum::LookupEnumName(SamplingQualityName, &SamplingQualityEnum); 
			check(SamplingQualityEnum); 
		} 
		return *SamplingQualityEnum; 
	}

	static FORCEINLINE UEnum& GetMaterialChannelEnum()
	{
		static FName MaterialChannelTypeName(TEXT("ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_AMBIENT")); 
		static UEnum* MaterialChannelTypeEnum = NULL; 
		if (MaterialChannelTypeEnum == NULL) 
		{ 
			UEnum::LookupEnumName(MaterialChannelTypeName, &MaterialChannelTypeEnum); 
			check(MaterialChannelTypeEnum); 
		} 
		return *MaterialChannelTypeEnum; 
	}

	static FORCEINLINE UEnum& GetCasterTypeEnum()
	{ 
		static FName CasterTypeName(TEXT("ESimplygonCasterType::None")); 
		static UEnum* CasterTypeEnum = NULL; 
		if (CasterTypeEnum == NULL) 
		{ 
			UEnum::LookupEnumName(CasterTypeName, &CasterTypeEnum); 
			check(CasterTypeEnum); 
		} 
		return *CasterTypeEnum; 

	}

	static FORCEINLINE UEnum& GetColorChannelEnum()
	{
		static FName ColorChannelTypeName(TEXT("ESimplygonColorChannels::RGBA")); 
		static UEnum* ColorChannelTypeEnum = NULL; 
		if (ColorChannelTypeEnum == NULL) 
		{ 
			UEnum::LookupEnumName(ColorChannelTypeName, &ColorChannelTypeEnum); 
			check(ColorChannelTypeEnum); 
		} 
		return *ColorChannelTypeEnum; 

	}

	static FORCEINLINE UEnum& GetFeatureImportanceEnum()
	{ 
		static FName FeatureImportanceName(TEXT("EMeshFeatureImportance::Off")); 
		static UEnum* FeatureImportanceEnum = NULL; 
		if (FeatureImportanceEnum == NULL) 
		{ 
			UEnum::LookupEnumName(FeatureImportanceName, &FeatureImportanceEnum); 
			check(FeatureImportanceEnum); 
		} 
		return *FeatureImportanceEnum; 
	}

	static FORCEINLINE UEnum& GetMaterialLODTypeEnum()
	{ 
		static FName MaterialLODTypeName(TEXT("EMaterialLODType::Off")); 
		static UEnum* MaterialLODTypeEnum = NULL; 
		if (MaterialLODTypeEnum == NULL) 
		{ 
			UEnum::LookupEnumName(MaterialLODTypeName, &MaterialLODTypeEnum); 
			check(MaterialLODTypeEnum); 
		} 
		return *MaterialLODTypeEnum; 

	}
};

enum ESimplygonMassiveLOD
{
	/** ProxyLOD */
	Remeshing UMETA(DisplayName="Remeshing"),
	/** AggregateLOD */
	Aggregate UMETA(DisplayName="Aggregate"),
};

class FSimplygonDummyClass : public UObject
{
public:
	FSimplygonDummyClass();
};