// Correction: if (true) { x = x + 1; }
void test1() {
  int x = 3;
  if (true) { x = x + 1; }
}

// Correction: int x = 1;
void test2() {
  if (true) { int x = 1; }
  x = 2;
}

// Correction: bool x = true;
void test3() {
  int x = 1;
  bool b = true;
  x = b;
}

// Correction: if (true) { b = true; }
void test4() {
  bool b = false;
  if (1) { b = true; }
}

// Correction: x = x + 1;
void test5() {
  int x = 1;
  bool b = true;
  x = x + b;
}

void main() {
  test1();
  test2();
  test3();
  test4();
  test5();
}
