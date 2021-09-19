#pragma once

#include <fmt/ostream.h>

// This checks for return values of scip calls and
// throws an exception for bad ones.

#define SCIP_CALL_EXC(x) {                      \
  SCIP_RETCODE retcode;			                \
  if( (retcode = (x)) != SCIP_OKAY)	            \
  {						                        \
	throw std::logic_error(                     \
      fmt::format("SCIP error {}", retcode)	    \
    );                                          \
  }						                        \
}

// This checks for return values of scip calls and
// terminates program for bad ones.

#define SCIP_CALL_TERM(x) {                      \
  SCIP_RETCODE retcode;			                \
  if( (retcode = (x)) != SCIP_OKAY)	            \
  {						                        \
	std::terminate();  \
  }						                        \
}
