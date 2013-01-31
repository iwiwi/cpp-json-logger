#include "jlog.h"

void f(int i) {
  JLOG_PUT("raw", i);
  JLOG_PUT("square", i * i);
}

int main(int argc, char **argv) {
  // Init
  JLOG_INIT(&argc, argv);

  // Put
  JLOG_PUT("hoge.ten", 10);
  JLOG_PUT("hoge.pi", 3.14);
  JLOG_PUT("hoge.hello.helloooo", "Hello");

  // Add
  for (int i = 0; i < 3; ++i) {
    JLOG_ADD("hoge.add", i);
  }

  // Open
  JLOG_OPEN("piyo") {
    JLOG_PUT("opened", "oh yeah");
  }

  // Add-open
  for (int i = 1; i <= 3; ++i) {
    JLOG_ADD_OPEN("piyo.add_open") {
      f(i);
    }
  }

  // Benchmark
  JLOG_PUT_BENCHMARK("bench.put") {
    usleep(100000);
  }
  for (int i = 0; i < 3; ++i) {
    JLOG_ADD_BENCHMARK("bench.add") {
      usleep((i + 1) * 100000);
    }
  }

  return 0;
}
