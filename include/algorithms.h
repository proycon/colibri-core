#ifndef COLIBRIALGO_H
#define COLIBRIALGO_H

#include <unordered_map>
#include <vector>
#include <utility>

std::vector< std::pair<int,int> > get_consecutive_gaps(const int n, const int leftmargin=0, const int rightmargin=0);
void compute_multi_skips(std::vector< std::vector<std::pair<int,int> > > & skips, const std::vector<std::pair<int,int> > & path, const int n, const int maxskips=3, const int skipnum = 0, const int leftmargin = 1);
#endif
