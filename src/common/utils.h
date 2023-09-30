#ifndef UTILS_H
#define UTILS_H

#include <deque>
#include <string>

namespace cpot {

std::deque<std::string> split(const std::string& text, char delimiter) {
  std::deque<std::string> r;
  size_t pos = 0;
  size_t i = -1;
  while (++i < text.size()) {
    if (text[i] == delimiter) {
      r.push_back(text.substr(pos, i - pos));
      pos = ++i;
    }
  }
  r.push_back(text.substr(pos, text.size() - pos));
  return r;
}

void process_std_lines(std::function<bool(const std::string&)> f) {
  while (true) {
    if (std::cin.eof()) {
      break;
    }
    std::string line;
    getline(std::cin, line);

    // Skip empty lines.
    if(line.find_first_not_of(' ') == std::string::npos) {
      continue;
    }

    if (f(line)) {
      break;
    }
  }
}

template<class T>
std::ostream& operator<<(std::ostream& s, const std::vector<T>& A) {
  if (A.size() == 0) {
    return s << "{}";
  }
  s << "{" << A[0];
  for (size_t i = 1; i < A.size(); ++i) {
    s << ", " << A[i];
  }
  return s << "}";
}

template<>
std::ostream& operator<<(std::ostream& s, const std::vector<std::string>& A) {
  if (A.size() == 0) {
    return s << "{}";
  }
  s << "{\"" << A[0] << "\"";
  for (size_t i = 1; i < A.size(); ++i) {
    s << ", \"" << A[i] << "\"";
  }
  return s << "}";
}

}  // namespace cpot

#endif  // UTILS_H
