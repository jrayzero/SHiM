// -*-c++-*-

#pragma once

#include <vector>

// https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
std::vector<std::string> split_on(std::string s, std::string delim) {
  int pos = 0;
  std::string token;
  std::vector<std::string> tokens;
  while ((pos = s.find(delim)) != std::string::npos) {
    token = s.substr(0, pos);
    tokens.push_back(token);
    s.erase(0, pos + delim.length());
  }
  if (!s.empty()) {
    tokens.push_back(s);
  }
  return tokens;
}
