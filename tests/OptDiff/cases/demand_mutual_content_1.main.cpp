// Copyright 2026, Peter Goodman. All rights reserved.
//
// demand_mutual_content_1 is an expected-diagnostic case under bare -demand
// (see the .dr header: the reject fires in the DataFlow demand body-walk with
// "Unsupported rule-body shape under -demand" -- mutual recursion inside a
// demanded body, the shadowed recursive-content belt): the compiler must exit
// 1 with a rendered diagnostic in all 4 modes, so this driver is inert /
// never compiled.
int main() {
  return 0;
}
