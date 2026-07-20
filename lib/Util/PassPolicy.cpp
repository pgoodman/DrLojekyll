// Copyright 2026, Peter Goodman. All rights reserved.

#include <drlojekyll/Util/PassPolicy.h>

#include <cstdio>

namespace hyde {
namespace {

// Prefix-star + exact matching only (the P1 pinned contract §5): `df.*`
// matches `df` and every `df.<x>`; `*` matches everything; anything else is
// an exact comparison. Deliberately NOT fnmatch.
static bool GlobMatches(std::string_view glob, std::string_view name) {
  if (glob == "*") {
    return true;
  }
  const auto size = glob.size();
  if (size >= 2u && glob[size - 1u] == '*' && glob[size - 2u] == '.') {
    const auto stem = glob.substr(0u, size - 2u);  // "df"
    if (name == stem) {
      return true;
    }
    const auto prefix = glob.substr(0u, size - 1u);  // "df."
    return name.size() > prefix.size() &&
           name.substr(0u, prefix.size()) == prefix;
  }
  return name == glob;
}

// The body-resident optional passes per level: exactly what the legacy
// wholesale-skip guards skip today. df.simplify / df.demand run OUTSIDE the
// DataFlow optimize guard and are deliberately absent.
static const char *const kDataFlowBody[] = {"df.cse", "df.canon", "df.dfe",
                                            "df.sink"};
static const char *const kControlFlowBody[] = {"cf.regionopt", "cf.procdedup"};

// EVERY registered pass name (the parse-time reachability authority).
// df.demand is deliberately ABSENT: -demand is a semantic flag the policy
// never gates in P1 (see DataFlow/Build.cpp), so naming it here would
// re-open the silent-neuter trap.
static const char *const kAllPasses[] = {"df.simplify", "df.cse", "df.canon",
                                         "df.dfe", "df.sink", "cf.regionopt",
                                         "cf.procdedup"};

}  // namespace

bool PassPolicy::Enabled(std::string_view name) const {
  auto enabled = only_globs.empty();
  for (const auto &glob : only_globs) {
    if (GlobMatches(glob, name)) {
      enabled = true;
      break;
    }
  }
  for (const auto &glob : disabled_globs) {
    if (GlobMatches(glob, name)) {
      enabled = false;
      break;
    }
  }
  return enabled;
}

bool PassPolicy::AnyBodyOptionalEnabled(PassLevel level) const {
  if (level == PassLevel::kDataFlow) {
    for (auto name : kDataFlowBody) {
      if (Enabled(name)) {
        return true;
      }
    }
    return false;
  }
  for (auto name : kControlFlowBody) {
    if (Enabled(name)) {
      return true;
    }
  }
  return false;
}

bool PassPolicy::Gate(std::string_view name, std::string_view desc) const {
  if (!Enabled(name)) {
    return false;  // Filtered out: no tick — index space is per-config.
  }
  const auto index = bisect_counter++;
  if (bisect_limit == -1) {
    fprintf(stderr, "BISECT: running pass (%llu) %.*s%s%.*s\n",
            static_cast<unsigned long long>(index),
            static_cast<int>(name.size()), name.data(),
            desc.empty() ? "" : " on ",
            static_cast<int>(desc.size()), desc.data());
    return true;
  }
  if (bisect_limit >= 0 && static_cast<int64_t>(index) > bisect_limit) {
    return false;  // Skipped by the limit; the tick already happened.
  }
  return true;
}

PassPolicy PassPolicy::DisableDataFlowOpt(void) {
  PassPolicy policy;
  for (auto name : kDataFlowBody) {
    policy.disabled_globs.emplace_back(name);
  }
  return policy;
}

PassPolicy PassPolicy::DisableControlFlowOpt(void) {
  PassPolicy policy;
  policy.disabled_globs.emplace_back("cf.*");
  return policy;
}

bool PassPolicy::MatchesAnyKnownPass(std::string_view glob) {
  for (auto name : kAllPasses) {
    if (GlobMatches(glob, name)) {
      return true;
    }
  }
  return false;
}

bool PassPolicy::IsValidGlob(std::string_view glob) {
  if (glob.empty()) {
    return false;
  }
  if (glob == "*") {
    return true;
  }
  for (auto i = 0u; i < glob.size(); ++i) {
    const auto ch = glob[i];
    if (ch == '?' || ch == '[' || ch == ']') {
      return false;
    }
    if (ch == '*' &&
        !(i + 1u == glob.size() && i >= 1u && glob[i - 1u] == '.')) {
      return false;  // '*' only as the tail of a dotted prefix.
    }
  }
  return true;
}

}  // namespace hyde
