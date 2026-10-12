#pragma once

#include "search/result.hpp"
#include "search/route_corridor.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace search
{
struct EverywhereSearchParams
{
  using OnResults = std::function<void(Results results)>;

  std::string m_query;
  std::string m_inputLocale;
  std::optional<std::chrono::steady_clock::duration> m_timeout;
  bool m_isCategory = false;

  OnResults m_onResults;

  // When set, results are filtered to the corridor around the active route
  // ("search along route") and annotated with distance from the route.
  // Kept last so existing positional aggregate initializations compile.
  std::shared_ptr<RouteCorridor> m_routeCorridor;
};
}  // namespace search
