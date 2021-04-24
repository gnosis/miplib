#pragma once

#include <miplib/constr.hpp>
#include <limits>

namespace miplib {
namespace detail {

Constr scale_gm(Constr const& constr, double skip_lb, double skip_ub);

}
}
