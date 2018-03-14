// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "FbxAnimUtils.h"
#include "Misc/Paths.h"
#include "EditorDirectories.h"
#include "FbxExporter.h"
#include "Animation/AnimTypes.h"
#include "Curves/RichCurve.h"
#include "Engine/CurveTable.h"

namespace FbxAnimUtils
{
	void ExportAnimFbx( const FString& ExportFilename, UAnimSequence* AnimSequence, USkeletalMesh* Mesh, bool bSaveSkeletalMesh, bool BatchMode, bool &OutExportAll, bool &OutCancelExport )
	{
		if( !ExportFilename.IsEmpty() && AnimSequence && Mesh )
		{
			FString FileName = ExportFilename;
			FEditorDirectories::Get().SetLastDirectory(ELastDirectory::FBX_ANIM, FPaths::GetPath(FileName)); // Save path as default for next time.

			UnFbx::FFbxExporter* Exporter = UnFbx::FFbxExporter::GetInstance();
			//Show the fbx export dialog options
			Exporter->FillExportOptions(BatchMode, (!BatchMode || !OutExportAll), ExportFilename, OutCancelExport, OutExportAll);
			if (!OutCancelExport)
			{
				Exporter->CreateDocument();

				Exporter->ExportAnimSequence(AnimSequence, Mesh, bSaveSkeletalMesh);

				// Save to disk
				Exporter->WriteToFile(*ExportFilename);
			}
		}
	}

	static FbxNode* FindCurveNodeRecursive(FbxNode* NodeToQuery, const FString& InCurveNodeName, FbxNodeAttribute::EType InNodeType)
	{
		if (InCurveNodeName == UTF8_TO_TCHAR(NodeToQuery->GetName()) && NodeToQuery->GetNodeAttribute() && NodeToQuery->GetNodeAttribute()->GetAttributeType() == InNodeType)
		{
			return NodeToQuery;
		}

		const int32 NodeCount = NodeToQuery->GetChildCount();
		for (int32 NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex)
		{
			FbxNode* FoundNode = FindCurveNodeRecursive(NodeToQuery->GetChild(NodeIndex), InCurveNodeName, InNodeType);
			if (FoundNode != nullptr)
			{
				return FoundNode;
			}
		}

		return nullptr;
	}

	static FbxNode* FindCurveNode(UnFbx::FFbxImporter* FbxImporter, const FString& InCurveNodeName, FbxNodeAttribute::EType InNodeType)
	{
		FbxNode *RootNode = FbxImporter->Scene->GetRootNode();
		return FindCurveNodeRecursive(RootNode, InCurveNodeName, InNodeType);
	}

	bool IsSupportedCurveDataType(int32 InDataType)
	{
		switch (InDataType)
		{
		case eFbxShort:		//!< 16 bit signed integer.
		case eFbxUShort:	//!< 16 bit unsigned integer.
		case eFbxUInt:		//!< 32 bit unsigned integer.
		case eFbxHalfFloat:	//!< 16 bit floating point.
		case eFbxInt:		//!< 32 bit signed integer.
		case eFbxFloat:		//!< Floating point value.
		case eFbxDouble:	//!< Double width floating point value.
		case eFbxDouble2:	//!< Vector of two double values.
		case eFbxDouble3:	//!< Vector of three double values.
		case eFbxDouble4:	//!< Vector of four double values.
		case eFbxDouble4x4:	//!< Four vectors of four double values.
			return true;
		}

		return false;
	}

	bool ShouldImportCurve(FbxAnimCurve* Curve, bool bDoNotImportWithZeroValues)
	{
		if (Curve && Curve->KeyGetCount() > 0)
		{
			if (bDoNotImportWithZeroValues)
			{
				for (int32 KeyIndex = 0; KeyIndex < Curve->KeyGetCount(); ++KeyIndex)
				{
					if (!FMath::IsNearlyZero(Curve->KeyGetValue(KeyIndex)))
					{
						return true;
					}
				}
			}
			else
			{
				return true;
			}
		}

		return false;
	}

	bool ImportCurveTableFromNode(const FString& InFbxFilename, const FString& InCurveNodeName, UCurveTable* InOutCurveTable, float& OutPreRoll)
	{
		UnFbx::FFbxImporter* FbxImporter = UnFbx::FFbxImporter::GetInstance();

		const FString FileExtension = FPaths::GetExtension(InFbxFilename);
		if (FbxImporter->ImportFromFile(*InFbxFilename, FileExtension, true))
		{
			if (FbxImporter->Scene != nullptr)
			{
				// merge animation layer at first (@TODO: do I need to do this?)
				FbxAnimStack* AnimStack = FbxImporter->Scene->GetMember<FbxAnimStack>(0);
				if (AnimStack != nullptr)
				{
					FbxImporter->MergeAllLayerAnimation(AnimStack, FbxTime::GetFrameRate(FbxImporter->Scene->GetGlobalSettings().GetTimeMode()));

					FbxTimeSpan AnimTimeSpan = FbxImporter->GetAnimationTimeSpan(FbxImporter->Scene->GetRootNode(), AnimStack, DEFAULT_SAMPLERATE);

					// Grab the start time, as we might have a preroll
					OutPreRoll = FMath::Abs((float)AnimTimeSpan.GetStart().GetSecondDouble());

					// @TODO: do I need this check?
					FbxAnimLayer* AnimLayer = AnimStack->GetMember<FbxAnimLayer>(0);
					if (AnimLayer != nullptr)
					{
						// Try the legacy path with curves in blendshapes on a dummy mesh
						if (FbxNode* MeshNode = FindCurveNode(FbxImporter, InCurveNodeName, FbxNodeAttribute::eMesh))
						{
							// We have a node, so clear the curve table
							InOutCurveTable->RowMap.Empty();

							FbxGeometry* Geometry = (FbxGeometry*)MeshNode->GetNodeAttribute();
							int32 BlendShapeDeformerCount = Geometry->GetDeformerCount(FbxDeformer::eBlendShape);
							for (int32 BlendShapeIndex = 0; BlendShapeIndex < BlendShapeDeformerCount; ++BlendShapeIndex)
							{
								FbxBlendShape* BlendShape = (FbxBlendShape*)Geometry->GetDeformer(BlendShapeIndex, FbxDeformer::eBlendShape);

								const int32 BlendShapeChannelCount = BlendShape->GetBlendShapeChannelCount();

								FString BlendShapeName = UTF8_TO_TCHAR(FbxImporter->MakeName(BlendShape->GetName()));

								for (int32 ChannelIndex = 0; ChannelIndex < BlendShapeChannelCount; ++ChannelIndex)
								{
									FbxBlendShapeChannel* Channel = BlendShape->GetBlendShapeChannel(ChannelIndex);

									if (Channel)
									{
										FString ChannelName = UTF8_TO_TCHAR(FbxImporter->MakeName(Channel->GetName()));

										// Maya adds the name of the blendshape and an underscore to the front of the channel name, so remove it
										if (ChannelName.StartsWith(BlendShapeName))
										{
											ChannelName = ChannelName.Right(ChannelName.Len() - (BlendShapeName.Len() + 1));
										}

										FbxAnimCurve* Curve = Geometry->GetShapeChannel(BlendShapeIndex, ChannelIndex, (FbxAnimLayer*)AnimStack->GetMember(0));
										if (Curve)
										{
											FRichCurve& RichCurve = *InOutCurveTable->RowMap.Add(*ChannelName, new FRichCurve());
											RichCurve.Reset();

											FbxImporter->ImportCurve(Curve, RichCurve, AnimTimeSpan, 0.01f);
										}
									}
								}
							}
							return true;
						}
						// Now look for attribute curves on a skeleton node
						else if (FbxNode* SkeletonNode = FindCurveNode(FbxImporter, InCurveNodeName, FbxNodeAttribute::eSkeleton))
						{
							// We have a node, so clear the curve table
							InOutCurveTable->RowMap.Empty();

							ExtractAttributeCurves(SkeletonNode, false, [&InOutCurveTable, &AnimTimeSpan, &FbxImporter](FbxAnimCurve* InCurve, const FString& InCurveName, const FString& InChannelName, int32 InChannelIndex, int32 InNumChannels)
							{
								FString FinalCurveName;
								if (InNumChannels == 1)
								{
									FinalCurveName = InCurveName;
								}
								else
								{
									FinalCurveName = InCurveName + "_" + InChannelName;
								}

								FRichCurve& RichCurve = *InOutCurveTable->RowMap.Add(*FinalCurveName, new FRichCurve());
								RichCurve.Reset();

								FbxImporter->ImportCurve(InCurve, RichCurve, AnimTimeSpan, 1.0f);
							});
							return true;
						}
					}
				}
			}
		}

		FbxImporter->ReleaseScene();

		return false;
	}

	void ExtractAttributeCurves(FbxNode* InNode, bool bInDoNotImportCurveWithZero, TFunctionRef<void(FbxAnimCurve* /*InCurve*/, const FString& /*InCurveName*/, const FString& /*InChannelName*/, int32 /*InChannelIndex*/, int32 /*InNumChannels*/)> ImportFunction)
	{
		FbxProperty Property = InNode->GetFirstProperty();
		while (Property.IsValid())
		{
			FbxAnimCurveNode* CurveNode = Property.GetCurveNode();
			// do this if user defined and animated and leaf node
			if( CurveNode && Property.GetFlag(FbxPropertyFlags::eUserDefined) &&
				CurveNode->IsAnimated() && FbxAnimUtils::IsSupportedCurveDataType(Property.GetPropertyDataType().GetType()) )
			{
				FString CurveName = UTF8_TO_TCHAR(CurveNode->GetName());
				UE_LOG(LogFbx, Log, TEXT("CurveName : %s"), *CurveName );

				int32 TotalCount = CurveNode->GetChannelsCount();
				for (int32 ChannelIndex=0; ChannelIndex<TotalCount; ++ChannelIndex)
				{
					FbxAnimCurve * AnimCurve = CurveNode->GetCurve(ChannelIndex);
					FString ChannelName = CurveNode->GetChannelName(ChannelIndex).Buffer();

					if (FbxAnimUtils::ShouldImportCurve(AnimCurve, bInDoNotImportCurveWithZero))
					{
						ImportFunction(AnimCurve, CurveName, ChannelName, ChannelIndex, TotalCount);
					}
					else
					{
						UE_LOG(LogFbx, Log, TEXT("CurveName(%s) is skipped because it only contains invalid values."), *CurveName);
					}
				}
			}

			Property = InNode->GetNextProperty(Property); 
		}
	}
}
