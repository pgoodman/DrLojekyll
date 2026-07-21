// Copyright 2026, Peter Goodman. All rights reserved.
//
// demand_recursive_content_1 is an expected-diagnostic case under bare
// -demand (see the .dr header: the reject fires upstream in the DataFlow
// demand body-walk, so a -demand-instance token would be inert): the compiler
// must exit 1 with a rendered diagnostic in all 4 modes, so this driver is
// inert / never compiled.
int main() {
  return 0;
}
