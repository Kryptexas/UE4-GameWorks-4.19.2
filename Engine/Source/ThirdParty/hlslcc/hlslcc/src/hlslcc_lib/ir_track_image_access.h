
#pragma once

/**
 * Determine access modes on image* variables and update the ir
 * @param ir - IR instructions.
 */
void TrackImageAccess(struct exec_list* ir);
