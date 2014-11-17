// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Determine access modes on image* variables and update the ir
 * @param ir - IR instructions.
 */
void TrackImageAccess(struct exec_list* ir);
