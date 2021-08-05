#include "scale.hpp"
#include <miplib/constr.hpp>
#include <miplib/solver.hpp>

#include <spdlog/spdlog.h>
#include <fmt/ostream.h>

namespace miplib {
namespace detail {

double nearest_power_of_two(double n)
{
  assert(n > 0);
  return std::pow(2, std::round(std::log2(n)));
}

double next_power_of_two(double n)
{
  assert(n > 0);
  return std::pow(2, std::ceil(std::log2(n)));
}

/*
  Rounds n to a nice power of two:
  * If n < 1, rounds to the lowest power of two not smaller than n.
  * If n >= 1, rounds to the nearest power of two.
*/
static double nice_power_of_two(double n)
{
    assert(n > 0);
    if (std::log2(n) < 0)
      return next_power_of_two(n);
    else
      return nearest_power_of_two(n);
}

/*
  Scale constraint using geometric mean such that low_max_abs * high_max_abs = 1.
  Based on [1].

  Args:
      - pyomo_linconstr: Constraint to scale.
      - skip_lb: Skip scaling if lowest max abs value is higher than this parameter.
      - skip_ub: Skip scaling if highest max abs value is lower than this parameter.
      - ignore_inf_var_bounds: If false, then it will throw if there is at least one
      variable with an infinite bound. If true, only the coefficient for that variable
      will be considered.
  [1] J. A. Tomlin. On scaling linear programming problems. In Computational
      practice in mathematical programming, pages 146–166. Springer, 1975.
*/

Constr scale_gm(Constr const& constr, double skip_lb, double skip_ub, bool ignore_inf_var_bounds)
{
  auto [minmaxabs, maxmaxabs] = constr.expr().numerical_range(ignore_inf_var_bounds);

  auto const inf = constr.expr().solver().infinity();
  if (minmaxabs == inf or maxmaxabs == inf)
    throw std::logic_error(
      "All variables must have defined domains for scaling constraint."
    );    

  if (minmaxabs >= skip_lb and maxmaxabs <= skip_ub)
    return constr;

  /*
    Determine "c" such that [minmaxabs/c, maxmaxabs/c] is
    geometrically around 1.

    The above minmaxabs and maxmaxabs are the term amplitudes without
    considering the scaling factor we need to introduce to scale the
    constraint around 1. To make sure that the final bounds we obtain once this
    scaling factor is introduced are still more or less centered around 1,
    we solve the following optimization problem:

    minimize log(new_minmaxabs)^2 + log(new_maxmaxabs)^2
    s.t.
    new_minmaxabs = min(minmaxabs / c, 1 / c)
    new_maxmaxabs = max(maxmaxabs / c, 1 / c)
  */

  double c = 1;
  if (maxmaxabs <= 1)
    c = sqrt(minmaxabs);
  else
  if (minmaxabs <= 1 and maxmaxabs > 1)
    c = sqrt(minmaxabs * maxmaxabs);
  else
  if (minmaxabs >= 1)
    c = sqrt(maxmaxabs);
  
  maxmaxabs = std::max(1 / c, maxmaxabs / c);
  minmaxabs = std::min(1 / c, minmaxabs / c);

  double amplitude = maxmaxabs / minmaxabs;

  if (!ignore_inf_var_bounds and amplitude >= 1e8)
    spdlog::warn(
      "Constraint terms differ more than 1e8 times - expect numerical issues!:\n"
      "| {0} | ∈ [{1}, {2}]",
      constr.expr(), minmaxabs, maxmaxabs
    );

  double scaling_factor = nice_power_of_two(1 / c);

  if (constr.type() == Constr::Equal)
    return scaling_factor * constr.expr() == 0;
  else
  {
    assert(constr.type() == Constr::LessEqual); 
    return scaling_factor * constr.expr() <= 0;
  }
}

}
}
