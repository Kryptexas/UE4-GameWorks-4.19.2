/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     CONST_TOK = 258,
     BOOL_TOK = 259,
     FLOAT_TOK = 260,
     INT_TOK = 261,
     UINT_TOK = 262,
     BREAK = 263,
     CONTINUE = 264,
     DO = 265,
     ELSE = 266,
     FOR = 267,
     IF = 268,
     DISCARD = 269,
     RETURN = 270,
     SWITCH = 271,
     CASE = 272,
     DEFAULT = 273,
     BVEC2 = 274,
     BVEC3 = 275,
     BVEC4 = 276,
     IVEC2 = 277,
     IVEC3 = 278,
     IVEC4 = 279,
     UVEC2 = 280,
     UVEC3 = 281,
     UVEC4 = 282,
     VEC2 = 283,
     VEC3 = 284,
     VEC4 = 285,
     HALF_TOK = 286,
     HVEC2 = 287,
     HVEC3 = 288,
     HVEC4 = 289,
     IN_TOK = 290,
     OUT_TOK = 291,
     INOUT_TOK = 292,
     UNIFORM = 293,
     VARYING = 294,
     GLOBALLYCOHERENT = 295,
     SHARED = 296,
     CENTROID = 297,
     NOPERSPECTIVE = 298,
     NOINTERPOLATION = 299,
     LINEAR = 300,
     MAT2X2 = 301,
     MAT2X3 = 302,
     MAT2X4 = 303,
     MAT3X2 = 304,
     MAT3X3 = 305,
     MAT3X4 = 306,
     MAT4X2 = 307,
     MAT4X3 = 308,
     MAT4X4 = 309,
     HMAT2X2 = 310,
     HMAT2X3 = 311,
     HMAT2X4 = 312,
     HMAT3X2 = 313,
     HMAT3X3 = 314,
     HMAT3X4 = 315,
     HMAT4X2 = 316,
     HMAT4X3 = 317,
     HMAT4X4 = 318,
     FMAT2X2 = 319,
     FMAT2X3 = 320,
     FMAT2X4 = 321,
     FMAT3X2 = 322,
     FMAT3X3 = 323,
     FMAT3X4 = 324,
     FMAT4X2 = 325,
     FMAT4X3 = 326,
     FMAT4X4 = 327,
     SAMPLERSTATE = 328,
     SAMPLERSTATE_CMP = 329,
     BUFFER = 330,
     TEXTURE1D = 331,
     TEXTURE1D_ARRAY = 332,
     TEXTURE2D = 333,
     TEXTURE2D_ARRAY = 334,
     TEXTURE2DMS = 335,
     TEXTURE_EXTERNAL = 336,
     TEXTURE2DMS_ARRAY = 337,
     TEXTURE3D = 338,
     TEXTURECUBE = 339,
     TEXTURECUBE_ARRAY = 340,
     RWBUFFER = 341,
     RWTEXTURE1D = 342,
     RWTEXTURE1D_ARRAY = 343,
     RWTEXTURE2D = 344,
     RWTEXTURE2D_ARRAY = 345,
     RWTEXTURE3D = 346,
     RWSTRUCTUREDBUFFER = 347,
     RWBYTEADDRESSBUFFER = 348,
     POINT_TOK = 349,
     LINE_TOK = 350,
     TRIANGLE_TOK = 351,
     LINEADJ_TOK = 352,
     TRIANGLEADJ_TOK = 353,
     POINTSTREAM = 354,
     LINESTREAM = 355,
     TRIANGLESTREAM = 356,
     INPUTPATCH = 357,
     OUTPUTPATCH = 358,
     STRUCT = 359,
     VOID_TOK = 360,
     WHILE = 361,
     CBUFFER = 362,
     IDENTIFIER = 363,
     TYPE_IDENTIFIER = 364,
     NEW_IDENTIFIER = 365,
     FLOATCONSTANT = 366,
     INTCONSTANT = 367,
     UINTCONSTANT = 368,
     BOOLCONSTANT = 369,
     STRINGCONSTANT = 370,
     FIELD_SELECTION = 371,
     LEFT_OP = 372,
     RIGHT_OP = 373,
     INC_OP = 374,
     DEC_OP = 375,
     LE_OP = 376,
     GE_OP = 377,
     EQ_OP = 378,
     NE_OP = 379,
     AND_OP = 380,
     OR_OP = 381,
     MUL_ASSIGN = 382,
     DIV_ASSIGN = 383,
     ADD_ASSIGN = 384,
     MOD_ASSIGN = 385,
     LEFT_ASSIGN = 386,
     RIGHT_ASSIGN = 387,
     AND_ASSIGN = 388,
     XOR_ASSIGN = 389,
     OR_ASSIGN = 390,
     SUB_ASSIGN = 391,
     INVARIANT = 392,
     VERSION_TOK = 393,
     EXTENSION = 394,
     LINE = 395,
     COLON = 396,
     EOL = 397,
     INTERFACE = 398,
     OUTPUT = 399,
     PRAGMA_DEBUG_ON = 400,
     PRAGMA_DEBUG_OFF = 401,
     PRAGMA_OPTIMIZE_ON = 402,
     PRAGMA_OPTIMIZE_OFF = 403,
     PRAGMA_INVARIANT_ALL = 404,
     ASM = 405,
     CLASS = 406,
     UNION = 407,
     ENUM = 408,
     TYPEDEF = 409,
     TEMPLATE = 410,
     THIS = 411,
     PACKED_TOK = 412,
     GOTO = 413,
     INLINE_TOK = 414,
     NOINLINE = 415,
     VOLATILE = 416,
     PUBLIC_TOK = 417,
     STATIC = 418,
     EXTERN = 419,
     EXTERNAL = 420,
     LONG_TOK = 421,
     SHORT_TOK = 422,
     DOUBLE_TOK = 423,
     HALF = 424,
     FIXED_TOK = 425,
     UNSIGNED = 426,
     DVEC2 = 427,
     DVEC3 = 428,
     DVEC4 = 429,
     FVEC2 = 430,
     FVEC3 = 431,
     FVEC4 = 432,
     SAMPLER2DRECT = 433,
     SAMPLER3DRECT = 434,
     SAMPLER2DRECTSHADOW = 435,
     SIZEOF = 436,
     CAST = 437,
     NAMESPACE = 438,
     USING = 439,
     ERROR_TOK = 440,
     COMMON = 441,
     PARTITION = 442,
     ACTIVE = 443,
     SAMPLERBUFFER = 444,
     FILTER = 445,
     IMAGE1D = 446,
     IMAGE2D = 447,
     IMAGE3D = 448,
     IMAGECUBE = 449,
     IMAGE1DARRAY = 450,
     IMAGE2DARRAY = 451,
     IIMAGE1D = 452,
     IIMAGE2D = 453,
     IIMAGE3D = 454,
     IIMAGECUBE = 455,
     IIMAGE1DARRAY = 456,
     IIMAGE2DARRAY = 457,
     UIMAGE1D = 458,
     UIMAGE2D = 459,
     UIMAGE3D = 460,
     UIMAGECUBE = 461,
     UIMAGE1DARRAY = 462,
     UIMAGE2DARRAY = 463,
     IMAGE1DSHADOW = 464,
     IMAGE2DSHADOW = 465,
     IMAGEBUFFER = 466,
     IIMAGEBUFFER = 467,
     UIMAGEBUFFER = 468,
     IMAGE1DARRAYSHADOW = 469,
     IMAGE2DARRAYSHADOW = 470,
     ROW_MAJOR = 471,
     COLUMN_MAJOR = 472
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 61 "../../../Source/ThirdParty/hlslcc/hlslcc/src/hlslcc_lib/hlsl_parser.yy"

   int n;
   float real;
   const char *identifier;
   const char *string_literal;

   struct ast_type_qualifier type_qualifier;

   ast_node *node;
   ast_type_specifier *type_specifier;
   ast_fully_specified_type *fully_specified_type;
   ast_function *function;
   ast_parameter_declarator *parameter_declarator;
   ast_function_definition *function_definition;
   ast_compound_statement *compound_statement;
   ast_expression *expression;
   ast_declarator_list *declarator_list;
   ast_struct_specifier *struct_specifier;
   ast_declaration *declaration;
   ast_switch_body *switch_body;
   ast_case_label *case_label;
   ast_case_label_list *case_label_list;
   ast_case_statement *case_statement;
   ast_case_statement_list *case_statement_list;

   struct {
	  ast_node *cond;
	  ast_expression *rest;
   } for_rest_statement;

   struct {
	  ast_node *then_statement;
	  ast_node *else_statement;
   } selection_rest_statement;

   ast_attribute* attribute;
   ast_attribute_arg* attribute_arg;



/* Line 2068 of yacc.c  */
#line 308 "../../../Source/ThirdParty/hlslcc/hlslcc/src/hlslcc_lib/hlsl_parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif



#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif



