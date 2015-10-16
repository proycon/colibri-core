#ifndef COLIBRIALGO_H
#define COLIBRIALGO_H

#include <unordered_map>
#include <vector>
#include <utility>
#include <cstdint>
#include "common.h"

std::vector< std::pair<int,int> > get_consecutive_gaps(const int n, const int leftmargin=1, const int rightmargin=1);
void compute_skip_configurations(std::vector< std::vector<std::pair<int,int> > > & skips,std::vector<uint32_t> & masks, const std::vector<std::pair<int,int> > & path, const int n, const int maxskips=3, const int skipnum = 0, const int leftmargin = 1);
#endif
