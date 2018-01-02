// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

class UControlRigSequence;
class UAnimSequence;
class USkeletalMesh;

namespace ControlRigSequenceConverter
{

/** Convert a control rig sequence to an anim sequence using a specific skeletal mesh */
extern void Convert(UControlRigSequence* Sequence, UAnimSequence* AnimSequence, USkeletalMesh* SkeletalMesh, bool bShowDialog = true);

}