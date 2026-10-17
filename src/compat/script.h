#pragma once
#include "compat/field_tasks.h"
/* Original context layout uses native pointers; never pass it to guest code. */
#define POKEHEARTGOLD_SCRIPT_H
#define SCRIPT_MODE_STOPPED 0
#define SCRIPT_MODE_BYTECODE 1
#define SCRIPT_MODE_NATIVE 2
#define ScriptReadByte(ctx) *(ctx->script_ptr++)
#include "script_types.inc"
