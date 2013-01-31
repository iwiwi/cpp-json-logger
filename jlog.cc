#include "jlog.h"

std::string FLAGS_jlog_out = "./jlog";

namespace jlog_internal {
jlog jlog::instance_;

double get_current_time_sec() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec * 1e-6;
}

long get_memory_usage() {
  //  http://stackoverflow.com/questions/669438/how-to-get-memory-usage-at-run-time-in-c

  std::ifstream stat_stream("/proc/self/stat", std::ios_base::in);
  if (!stat_stream) {
    return 0;
  }

  std::string pid, comm, state, ppid, pgrp, session, tty_nr;
  std::string tpgid, flags, minflt, cminflt, majflt, cmajflt;
  std::string utime, stime, cutime, cstime, priority, nice;
  std::string O, itrealvalue, starttime;
  long vm_usage, rss;

  stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
              >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
              >> utime >> stime >> cutime >> cstime >> priority >> nice
              >> O >> itrealvalue >> starttime >> vm_usage >> rss;
  stat_stream.close();

  // in bytes
  long page_size = sysconf(_SC_PAGE_SIZE);
  long resident_set = rss * page_size;

  return resident_set;
  // return vm_usage;
}
}

void JLOG_INIT(int *argc, char **argv) {
  int k = 0;
  for (int i = 0; i < *argc; ++i) {
    if (strncmp(argv[i], "--jlog_out=", 11) == 0) {
      FLAGS_jlog_out = argv[i] + 11;
    } else {
      argv[k++] = argv[i];
    }
  }
  *argc = k;
  jlog_internal::jlog::init(*argc, argv);
}

