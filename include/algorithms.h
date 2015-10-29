#ifndef COLIBRIALGO_H
#define COLIBRIALGO_H

#include <unordered_map>
#include <vector>
#include <utility>
#include <cstdint>
#include "common.h"

uint32_t vector2mask(const std::vector<std::pair<int,int>> & skips);
std::vector<std::pair<int,int>> mask2vector(const uint32_t mask, const int n);
std::vector< std::pair<int,int> > get_consecutive_gaps(const int n, const int leftmargin=1, const int rightmargin=1);
uint32_t reversemask(uint32_t mask, const unsigned int n);
int maskheadskip(uint32_t mask, const unsigned int n);
int masktailskip(uint32_t mask, const unsigned int n);
std::vector<uint32_t> compute_skip_configurations(const int n, const int maxskips);

#endif
