// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.


#include "ShaderFormatVectorVM.h"
#include "CoreMinimal.h"
#include "hlslcc.h"
#include "hlslcc_private.h"
#include "VectorVMBackend.h"

PRAGMA_DISABLE_SHADOW_VARIABLE_WARNINGS
#include "glsl_parser_extras.h"
PRAGMA_POP

#include "hash_table.h"
#include "ir_rvalue_visitor.h"
#include "PackUniformBuffers.h"
#include "IRDump.h"
#include "OptValueNumbering.h"
#include "ir_optimization.h"
#include "ir_expression_flattening.h"
#include "ir.h"

#include "VectorVM.h"
#include "INiagaraCompiler.h"


/** Removes any assignments that don't actually map to a VM op but just move some data around. We look for refs and grab the source data direct. */
class ir_propagate_non_expressions_visitor : public ir_rvalue_visitor
{
	void *mem_ctx;
	_mesa_glsl_parse_state *parse_state;

	exec_list non_expr_list;
	int num_expr;
	bool progress;

public:
	ir_propagate_non_expressions_visitor(_mesa_glsl_parse_state *in_state)
	{
		mem_ctx = ralloc_context(0);
		parse_state = in_state;
		num_expr = 0;
		progress = false;
	}

	~ir_propagate_non_expressions_visitor()
	{
		ralloc_free(mem_ctx);
	}

	virtual void handle_rvalue(ir_rvalue **rvalue)
	{
		if (rvalue && *rvalue)
		{
			//Get the dereference for this rvalue if it is one. May need to bypass a swizzle.
			ir_dereference* search_deref = nullptr;
			if (ir_dereference* deref = (*rvalue)->as_dereference())
			{
				search_deref = deref;
			}
			else if (ir_swizzle* swiz = (*rvalue)->as_swizzle())
			{
				search_deref = swiz->val->as_dereference();
			}

			//Search to see if this deref matches any of the non-expression assignments LHS. If so then clone the rhs in it's place.
			if (search_deref)
			{
				foreach_iter(exec_list_iterator, iter, non_expr_list)
				{
					ir_assignment* assign = (ir_assignment*)iter.get();
					if (AreEquivalent(search_deref, assign->lhs))
					{
						ir_rvalue** to_replace = rvalue;
						if (ir_swizzle* swiz = (*rvalue)->as_swizzle())
						{
							check(swiz->mask.num_components == 1);//We do this post scalarize so they're all single component.
							if ((assign->write_mask & 1 << swiz->mask.x) == 0)
							{
								to_replace = nullptr;//this is not the right component assignment to use.
							}
						}
						else if (!(*rvalue)->type->is_scalar())
						{
							//if we're not a swizzle or a direct scalar deref then don't replace anything.
							to_replace = nullptr;
						}

						//If we are a matching scalar or swizzled deref then replace us with the rhs of the non expr assignment.
						if (to_replace)
						{
							ir_rvalue* repl = assign->rhs->clone(ralloc_parent(*to_replace), nullptr);
							(*to_replace)->replace_with(repl);
							*to_replace = repl;
							progress = true;
							break;
						}
					}
				}
			}
		}
	}

	virtual ir_visitor_status visit_enter(ir_expression*)
	{
		num_expr++;
		return visit_continue;
	}

	virtual ir_visitor_status visit_enter(ir_assignment *assign)
	{
		if (assign->condition)
		{
			_mesa_glsl_error(parse_state, "conditional assignment in instruction stream");
			return visit_stop;
		}

		int old_num_expr = num_expr;

		//Handle the rhs. This call will go into HandleRValue and for any rvalues with no expressions, they'll be replaced with an assignment of a previous temporary.
		num_expr = 0;
		assign->rhs->accept(this);

		//Add any new temp or auto assignments. These will be grabbed later to use in replacements in HandleRValue.
		if (num_expr == 0 && assign->lhs->ir_type == ir_type_dereference_variable)
		{
			ir_variable_mode mode = assign->lhs->variable_referenced()->mode;
			if (mode == ir_var_temporary || mode == ir_var_auto)
			{
				assign->remove();
				non_expr_list.push_head(assign);
			}
		}
		num_expr = old_num_expr;
		return visit_continue;
	}

	static void run(exec_list *ir, _mesa_glsl_parse_state *state)
	{
		bool progress = false;
		do
		{
			ir_propagate_non_expressions_visitor propagate_non_expressions_visitor(state);
			visit_list_elements(&propagate_non_expressions_visitor, ir);

			progress = propagate_non_expressions_visitor.progress;

			vm_debug_dump(ir, state);

			progress = do_dead_code(ir, false) || progress;
			progress = do_dead_code_local(ir) || progress;
		} while (progress);
	}
};

void vm_propagate_non_expressions_visitor(exec_list* ir, _mesa_glsl_parse_state* state)
{
	ir_propagate_non_expressions_visitor::run(ir, state);
}