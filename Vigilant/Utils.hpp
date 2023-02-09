#pragma once
#include <string>
#include <vector>

void trim_all(std::string &strInOut);
void trim_right(std::string &strInOut);
void trim_left(std::string &strInOut);
void split(const std::string &strIn, std::vector<std::string> &vectWords);
