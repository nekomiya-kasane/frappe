#pragma once

// Range++ (rpp) - Generic Range Adapters Library
// A SQL-expressive, CPO-based range adapter library built on range-v3

// Core infrastructure
#include "concepts.hpp"
#include "pipeable.hpp"
#include "comparator.hpp"

// Filtering
#include "filter.hpp"
#include "filters_ext.hpp"
#include "combine.hpp"

// Sorting
#include "sort.hpp"

// Selection and projection
#include "select.hpp"
#include "distinct.hpp"

// Aggregation
#include "aggregate.hpp"
#include "aggregate_ext.hpp"

// Transformation
#include "transform.hpp"

// Join operations
#include "join.hpp"

// Set operations
#include "set_ops.hpp"

// Window functions
#include "window.hpp"

// Extended features (Phase 1-4)
#include "extras.hpp"

namespace rpp {

// Version info
inline constexpr int version_major = 0;
inline constexpr int version_minor = 6;
inline constexpr int version_patch = 0;

} // namespace rpp
