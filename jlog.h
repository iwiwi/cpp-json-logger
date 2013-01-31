#ifndef JLOG_H_
#define JLOG_H_

#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/resource.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <map>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdlib>
#include <stdint.h>
#include <vector>

extern std::string FLAGS_jlog_out;

namespace jlog_internal {
double get_current_time_sec();
long get_memory_usage();

struct json_node {
  virtual void print(std::ostream &os, int depth, bool last) = 0;
};

struct json_parent : json_node {
  std::vector<std::string> keys;
  std::map<std::string, json_node*> children;

  virtual void print(std::ostream &os, int depth, bool last) {
    os << "{" << std::endl;
    for (size_t i = 0; i < keys.size(); ++i) {
      os << std::string(depth + 2, ' ') << "\"" << keys[i] << "\": ";
      children[keys[i]]->print(os, depth + 2, i + 1 == keys.size());
    }
    os << std::string(depth, ' ')
       << (last ? "}" : "},")
       << std::endl;
  }
};

struct json_leaf : json_node {
  std::string value;

  virtual void print(std::ostream &os, int, bool last) {
    os << "\"" << value << (last ? "\"" : "\",") << std::endl;
  }
};

struct json_array : json_node {
  std::vector<json_node*> children;

  virtual void print(std::ostream &os, int depth, bool last) {
    os << "[" << std::endl;
    for (size_t i = 0; i < children.size(); ++i) {
      os << std::string(depth + 2, ' ');
      children[i]->print(os, depth + 2, i + 1 == children.size());
    }
    os << std::string(depth, ' ')
       << (last ? "]" : "],")
       << std::endl;
  }
};

class jlog {
 public:
  template<typename value_t>
  static void jlog_put(const char *path, value_t value,
                       bool glog) {
    if (instance_.filename_.empty() && !instance_.already_warned_) {
      std::cerr << "WARNING: Logging without calling JLOG_INIT is written to STDOUT" << std::endl;
      instance_.already_warned_ = true;
    }

    if (instance_.nested_glog_flag_ && glog) {
      LOG() << path << " = " << value << std::endl;
    }

    json_node *&jn = instance_.reach_path(path);
    json_leaf *jl = new json_leaf;
    to_string<value_t>(value, &jl->value);
    jn = jl;
  }

  template<typename value_t>
  static void jlog_add(const char *path, value_t value,
                       bool glog) {
    if (instance_.filename_.empty() && !instance_.already_warned_) {
      std::cerr << "WARNING: Logging without calling JLOG_INIT is written to STDOUT" << std::endl;
      instance_.already_warned_ = true;
    }

    if (instance_.nested_glog_flag_ && glog) {
      LOG() << path << " = " << value << std::endl;
    }

    json_node *&jn = instance_.reach_path(path);
    if (jn == NULL) jn = new json_array;
    json_array *ja = dynamic_cast<json_array*>(jn);
    assert(ja);
    json_leaf *jl = new json_leaf;
    to_string<value_t>(value, &jl->value);
    ja->children.push_back(jl);
  }

  static void init(int argc, char **argv) {
    const char *slash = strrchr(argv[0], '/');
    instance_.program_name_ = slash ? slash + 1 : argv[0];

    struct utsname u;
    if (0 != uname(&u)) {
      *u.nodename = '\0';
    }

    time_t timestamp = time(NULL);
    struct ::tm tm_time;
    localtime_r(&timestamp, &tm_time);

    std::ostringstream os;
    os.fill('0');
    os << instance_.program_name_ << "."
       << u.nodename << "."
       // << getenv("USER") << "."
       << "jlog."
       << std::setw(2) << (1900 + tm_time.tm_year) % 100
       << std::setw(2) << 1+tm_time.tm_mon
       << std::setw(2) << tm_time.tm_mday
       << '-'
       << std::setw(2) << tm_time.tm_hour
       << std::setw(2) << tm_time.tm_min
       << std::setw(2) << tm_time.tm_sec
       << '.'
       << getpid()
       << '\0';

    instance_.filename_ = os.str();
    LOG() << "JLOG: " << instance_.filename_ << std::endl;

    jlog_put("run.program", instance_.program_name_, true);
    for (int i = 0; i < argc; ++i) {
      jlog_add("run.args", argv[i], true);
    }
    jlog_put("run.machine", u.nodename, true);
    {
      char buf[256];
      strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", &tm_time);
      jlog_put("run.date", buf, true);
    }
    jlog_put("run.user", getenv("USER"), true);
    jlog_put("run.pid", getpid(), true);

    instance_.start_time_ = get_current_time_sec();
  }

 private:
  static std::ostream &LOG() {
    time_t timestamp = time(NULL);
    struct ::tm tm_time;
    localtime_r(&timestamp, &tm_time);

    std::cerr.fill('0');
    std::cerr
        << '['
        << std::setw(2) << 1+tm_time.tm_mon
        << std::setw(2) << tm_time.tm_mday
        << ' '
        << std::setw(2) << tm_time.tm_hour << ':'
        << std::setw(2) << tm_time.tm_min << ':'
        << std::setw(2) << tm_time.tm_sec << "] ";
    return std::cerr;
  }

  template<typename value_t>
  static void to_string(value_t value, std::string *s) {
    std::ostringstream oss;
    oss << value;
    *s = oss.str();
  }

  jlog() : already_warned_(false), nested_glog_flag_(true) {
    current_ = &root_;
  }

  ~jlog() {
    if (filename_.empty()) {
      if (!root_.children.empty()) {
        root_.print(std::cout, 0, true);
      }
    } else {
      jlog_put("run.time", get_current_time_sec() - start_time_, true);
      jlog_put("run.memory", get_memory_usage(), true);

      if (0 != system(("mkdir -p " + FLAGS_jlog_out).c_str())) {
        perror("mkdir");
      }
      std::ofstream ofs((FLAGS_jlog_out + "/" + filename_).c_str());
      if (!ofs) {
        perror("ofstream");
        LOG() << "JLOG: Failed to open output file, write to STDERR" << std::endl;
        root_.print(std::cout, 0, true);
      }
      root_.print(ofs, 0, true);
      LOG() << "JLOG: " << FLAGS_jlog_out + "/" + filename_ << std::endl;

      std::ostringstream os;
      std::string linkname = FLAGS_jlog_out + "/" + std::string(program_name_);

      if (0 != system(("rm -rf " + linkname).c_str())) {
        perror("rm");
      }
      if (0 != symlink(filename_.c_str(), linkname.c_str())) {
        perror("symlink");
      }
    }
  }

  json_node *&reach_path(const char *path) {
    std::string s(path);
    assert(!s.empty());
    for (size_t i = 0; i < s.length(); ++i) {
      if (s[i] == '.') s[i] = ' ';
    }
    std::istringstream iss(s);

    json_node **jn = &current_;
    for (std::string t; iss >> t; ) {
      if (*jn == NULL) *jn = new json_parent();
      json_parent *jp = dynamic_cast<json_parent*>(*jn);
      assert(jp != NULL);
      if (jp->children[t] == NULL) jp->keys.push_back(t);
      jn = &jp->children[t];
      assert(jn);
    }

    return *jn;
  }

  json_parent root_;
  json_node *current_;
  bool already_warned_;
  bool nested_glog_flag_;

  const char *program_name_;
  std::string filename_;
  double start_time_;

  static jlog instance_;

  friend class jlog_opener;
  friend class jlog_add_opener;
};

class jlog_opener {
 public:
  jlog_opener(bool add, const char *path, bool glog = true) {
    if (add == false) {
      prev_ = jlog::instance_.current_;
      json_node *& jn = jlog::instance_.reach_path(path);
      if (jn == NULL) jn = new json_parent;
      jlog::instance_.current_ = jn;
    } else {
      prev_ = jlog::instance_.current_;
      json_node *& jn = jlog::instance_.reach_path(path);
      if (jn == NULL) jn = new json_array;
      json_array *ja = dynamic_cast<json_array*>(jn);
      assert(ja);
      ja->children.push_back(new json_parent);
      jlog::instance_.current_ = ja->children.back();
    }

    prev_glog_flag_ = jlog::instance_.nested_glog_flag_;
    jlog::instance_.nested_glog_flag_ = glog;
  }

  ~jlog_opener() {
    jlog::instance_.current_ = prev_;
    jlog::instance_.nested_glog_flag_ = prev_glog_flag_;
  }

  operator bool() {
    return false;
  }

 private:
  json_node *prev_;
  bool prev_glog_flag_;
};

class jlog_benchmarker {
 public:
  jlog_benchmarker(bool add, const char *path, bool glog = true)
      : path_(path), add_(add), glog_(glog) {
    start_ = get_current_time_sec();
  }

  ~jlog_benchmarker() {
    double r = get_current_time_sec() - start_;
    if (add_) {
      jlog::jlog_add(path_, r, glog_);
    } else {
      jlog::jlog_put(path_, r, glog_);
    }
  }

  operator bool() {
    return false;
  }

 private:
  const char *path_;
  bool add_;
  double start_;
  bool glog_;
};
}  // namespace jlog_internal

#define JLOG_OPEN(...)                                              \
  if (jlog_internal::jlog_opener o__                                \
      = jlog_internal::jlog_opener(false, __VA_ARGS__)); else

#define JLOG_ADD_OPEN(...)                                          \
  if (jlog_internal::jlog_opener o__                                \
      = jlog_internal::jlog_opener(true, __VA_ARGS__)); else

#define JLOG_PUT_BENCHMARK(...)                                     \
  if (jlog_internal::jlog_benchmarker o__                           \
      = jlog_internal::jlog_benchmarker(false, __VA_ARGS__)); else

#define JLOG_ADD_BENCHMARK(...)                                     \
  if (jlog_internal::jlog_benchmarker o__                           \
      = jlog_internal::jlog_benchmarker(true, __VA_ARGS__)); else

template<typename value_t>
inline void JLOG_PUT(const char *path, value_t value, bool glog = true) {
  jlog_internal::jlog::jlog_put(path, value, glog);
}

template<typename value_t>
inline void JLOG_ADD(const char *path, value_t value, bool glog = true) {
  jlog_internal::jlog::jlog_add(path, value, glog);
}

void JLOG_INIT(int *argc, char **argv);

#endif  // JLOG_H_
