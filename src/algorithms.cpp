#include <vector>
#include <utility>
#include "algorithms.h"

using namespace std;


vector< pair<int,int> > get_consecutive_gaps(const int n, const int leftmargin, const int rightmargin) { //obsolete
    vector< pair<int,int> > gaps;
    int begin = leftmargin;
    while (begin < n) {
        int length = (n - rightmargin) - begin;
        while (length > 0) {
            pair<int,int> gap = make_pair(begin, length);
            gaps.push_back(gap);
            length--;
            
        }
        begin++;
    }      
    return gaps;
}

uint32_t vector2mask(const vector<pair<int,int>> & skips) {
    //convert path to mask
    uint32_t mask = 0;
    for (vector<pair<int,int>>::const_iterator iter2 = skips.begin(); iter2 != skips.end(); iter2++) {
        for (unsigned int i = iter2->first; i < (iter2->first + iter2->second) && (i < 31); i++ ) {
            mask |= bitmask[i];
        }
    }
    return mask;
}

vector<pair<int,int>> mask2vector(const uint32_t mask, const int n) {
    //convert mask to (begin, length) vector
    vector<pair<int,int>> gaps;
    int gapbegin = 0;
    int gaplength = 0;
    for (int i = 0; i < n; i++) {
        if (mask & bitmask[i]) {
            if (gaplength == 0) gapbegin = i;
            gaplength++;
        } else {
            if (gaplength > 0) {
                gaps.push_back(pair<int,int>(gapbegin,gaplength));
                gaplength = 0;
            }
        }
    }
    if (gaplength > 0) {
        gaps.push_back(pair<int,int>(gapbegin,gaplength));
        gaplength = 0;
    }
    return gaps;
}

vector<uint32_t> compute_skip_configurations(const int n, const int maxskips) {
    vector<uint32_t> masks;
    if (n < 3) return masks;
    const uint32_t order = pow(2,n-2);
    for (uint32_t i = 1; i < order; i++) {
        const uint32_t mask = i << 1;
        if (n-2 >= maxskips) {
            if (mask2vector(mask,n).size() > maxskips) { //not very efficient but not important here
                continue;
            }
        }
        masks.push_back( mask );
    }
    return masks;
}
