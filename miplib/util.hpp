#pragma once

#include <boost/container_hash/hash.hpp>
#include <unordered_map>

#include "var.hpp"


namespace miplib {

/**
 * @brief A pair of variables.
 */
typedef std::pair<Var, Var> VarPair;

/**
 * @brief Convenience factory for containers involving Var's.
 * 
 * @tparam VarArgs 
 */
template<class ...VarArgs>
struct Vars
{
  Vars(VarArgs const&... var_args):
    m_var_args(std::forward_as_tuple(var_args...)) {}

  std::vector<Var> as_vector(std::size_t s) const
  {
    std::vector<Var> v;
    for (std::size_t i = 0; i < s; ++i)
      v.push_back(std::make_from_tuple<Var>(m_var_args));
    return v;
  }

template<class Container>
auto as_unordered_map_values(Container const& keys) const ->
  std::unordered_map<typename std::decay<decltype(*std::begin(keys))>::type, Var> 
  {
    typedef typename std::decay<decltype(*std::begin(keys))>::type value_type;
    std::unordered_map<value_type, Var> m;
    std::for_each(std::begin(keys), std::end(keys), [&](auto const& k) {
      m.insert({k, std::make_from_tuple<Var>(m_var_args)});
    });
    return m;    
  }

  std::tuple<VarArgs...> m_var_args;
};


}

namespace std {
template<>
struct hash<miplib::VarPair>
{
  std::size_t operator()(miplib::VarPair const& vp) const noexcept
  {
    return boost::hash<miplib::VarPair>()(vp);
  }
};


template<>
struct equal_to<miplib::VarPair>
{
  bool operator()(miplib::VarPair const& vp1, miplib::VarPair const& vp2) const noexcept
  {
    return vp1.first.is_same(vp2.first) and vp1.second.is_same(vp2.second);
  }
};

}  // namespace std
