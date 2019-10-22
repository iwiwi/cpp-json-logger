#include <limits>
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
  JLOG_PUT("hoge.true", true);
  JLOG_PUT("hoge.false", false);

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

  // Ignore
  JLOG_OPEN("poyo") {
    JLOG_IGNORE {
      JLOG_PUT("ignored", "sad");
      JLOG_IGNORE {
        JLOG_PUT("nest", "sad");
      }
    }
    JLOG_PUT("notignored", "happy");
  }

  // NaN, Infinity
  if (std::numeric_limits<double>::has_quiet_NaN) {
    JLOG_PUT("nan", std::numeric_limits<double>::quiet_NaN());
  }

  if (std::numeric_limits<double>::has_infinity) {
    JLOG_PUT("inf", std::numeric_limits<double>::infinity());
    JLOG_PUT("neg-inf", -std::numeric_limits<double>::infinity());
  }

  // Add-open
  for (int i = 1; i <= 3; ++i) {
    JLOG_ADD_OPEN("piyo.add_open") {
      f(i);
    }
  }
  
  // No stderr output
  JLOG_PUT("stderr_output", "false", false);
  JLOG_PUT("stderr_output", "true (default)", true);

  return 0;
}
