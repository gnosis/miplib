#pragma once
#include <spdlog/spdlog.h>

// for now simply log the error and rethrow

template<typename F, typename... Args>
std::invoke_result_t<F, Args...> call_with_exception_logging(F&& f, Args&&... args)
{
  try {
    return std::invoke(f, std::forward<Args>(args)...);
  }
  catch (GRBException const& e) {
    spdlog::error("Gurobi error [code={}] {}", e.getErrorCode(), e.getMessage());
    throw;
  }
}
