// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

// ir_track_image_access.cpp: Utility functions

#include "ShaderCompilerCommon.h"
#include "hlslcc.h"
#include "mesa/compiler.h"
#include "mesa/glsl_parser_extras.h"
#include "mesa/hash_table.h"


/**
 * IR visitor used to track how image* variables are accessed.
 * when complete, all image vars are labeled with read/write 
 */
class ir_track_image_access_visitor : public ir_hierarchical_visitor
{
public:

	/**
	 * Only need to hook image dereference, as it is
	 * the only thing with relevant data
	 */
	virtual ir_visitor_status visit_enter(class ir_dereference_image *image)
	{
		ir_variable* var = image->variable_referenced(); 

		if (var)
		{
			if (this->in_assignee)
			{
				var->image_write = 1;
			}
			else
			{
				var->image_read = 1;
			}
		}
		return visit_continue;
	}
};

void TrackImageAccess(struct exec_list* ir)
{
	ir_track_image_access_visitor v;

	v.run(ir);
}
