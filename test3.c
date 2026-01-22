void main() {
  int x = 1;
  { int x = 2; x = x + 1; }
  x = x + 1;
}
