
#include "util.hpp"
#include <stdexcept>

#include <lpsolve/lp_lib.h>

int get_cur_row_index(lprec* lp, int orig_row_index)
{
  int r = get_lp_index(lp, orig_row_index);
  if (r == 0)
    throw std::logic_error("lpsolve error retrieving current row index");
  return r;
}

int get_cur_col_index(lprec* lp, int orig_col_index)
{
  int nr_orig_rows = get_Norig_rows(lp);
  int r = get_lp_index(lp, orig_col_index + nr_orig_rows);
  if (r == 0)
    throw std::logic_error("lpsolve error retrieving current column index");
  return r;
}
