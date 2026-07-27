// Copyright 2026, Peter Goodman. All rights reserved.
// Copyright 2019, Trail of Bits, Inc. All rights reserved.

#pragma once

#include <drlojekyll/Parse/Error.h>

namespace hyde {

class DisplayManager;
class DisplayPosition;
class DisplayRange;

// Keeps track of a log of errors.
class ErrorLog {
 public:
  explicit ErrorLog(const DisplayManager &dm_);

  // Add the given error message.
  void Append(Error error) const;

  // Add a new error message constructed from this `ErrorLog`'s `DisplayManager`.
  Error Append(void) const;

  // Add a new error message related to a line:column offset.
  Error Append(const DisplayPosition &pos) const;

  // Add a new error message related to a highlighted range of tokens.
  Error Append(const DisplayRange &range) const;

  // Add a new error message related to a highlighted range of tokens, with one
  // character in particular being referenced.
  Error Append(const DisplayRange &range,
               const DisplayPosition &pos_in_range) const;

  // Add an new error message related to a highlighted range of tokens, with a
  // sub-range in particular being referenced.
  Error Append(const DisplayRange &range, const DisplayRange &sub_range) const;

  // Add a new error message related to a highlighted range of tokens, with a
  // sub-range in particular being referenced, where the error itself is at
  // `pos_in_range`.
  Error Append(const DisplayRange &range, const DisplayRange &sub_range,
               const DisplayPosition &pos_in_range) const;

  // Add a WARNING related to a highlighted range of tokens. Warnings render
  // like errors (with a `warning:` category) but are advisory: they do not
  // count toward `Size()`/`IsEmpty()` and never fail a compile.
  Error AppendWarning(const DisplayRange &range) const;

  // Add a warning related to a highlighted range of tokens, with a sub-range
  // in particular being referenced.
  Error AppendWarning(const DisplayRange &range,
                      const DisplayRange &sub_range) const;

  // Check if the log is empty. Warnings are advisory and do not count.
  bool IsEmpty(void) const;

  // Returns the number of errors in the log. Warnings do not count.
  unsigned Size(void) const;

  // Render the formatted errors to a stream, along with any attached notes,
  // then any warnings.
  void Render(std::ostream &os, const ErrorColorScheme &color_scheme =
                                    Error::kDefaultColorScheme) const;

  // Render only the warnings (the success path: errors are impossible but
  // accumulated warnings still owe the user a report).
  void RenderWarnings(std::ostream &os,
                      const ErrorColorScheme &color_scheme =
                          Error::kDefaultColorScheme) const;

 private:
  class Impl;

  std::shared_ptr<Impl> impl;
};

}  // namespace hyde
