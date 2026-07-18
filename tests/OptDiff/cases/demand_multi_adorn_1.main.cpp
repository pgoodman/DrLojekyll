// demand_multi_adorn_1 is an expected-diagnostic case under -demand (see the
// .dr header): the compiler must exit 1 with a rendered diagnostic in all 4
// modes, so this driver is inert and never compiled.
int main() {
  return 0;
}
