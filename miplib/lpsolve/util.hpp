#pragma once

#include <lpsolve/lp_lib.h>

int get_cur_row_index(lprec* lp, int orig_row_index);
int get_cur_col_index(lprec* lp, int orig_col_index);