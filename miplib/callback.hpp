#pragma once

namespace miplib {
namespace detail {
struct ICallback
{
    virtual void operator()() = 0;
};

}
}
