#include <vector>
#include <utility>
#include "algorithms.h"

using namespace std;


vector< pair<int,int> > get_consecutive_gaps(const int n, const int leftmargin, const int rightmargin) {
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

void compute_skip_configurations(vector< vector<pair<int,int> > > & skips, vector<uint32_t> & masks,const vector<pair<int,int> > & path, const int n, const int maxskips, const int skipnum, const int leftmargin) {    
    vector< pair<int,int> > currentskips = get_consecutive_gaps(n, leftmargin);
    for (vector< pair<int,int> >::iterator iter = currentskips.begin(); iter != currentskips.end(); iter++) {                
         const int newleftmargin = iter->first + iter->second + 1;   
         vector< pair<int,int> > newpath = path; //copy
         newpath.push_back( *iter );
         if ((skipnum + 1 == maxskips) || ( n - newleftmargin <= 2)) {
            skips.push_back( newpath );
            //convert path to mask
            uint32_t mask = 0;
            for (vector<pair<int,int>>::iterator iter2 = newpath.begin(); iter2 != newpath.end(); iter2++) {
                for (unsigned int i = iter2->first; i < (iter2->first + iter2->second) && (i < 31); i++ ) {
                    mask |= bitmask[i];
                }
            }
            masks.push_back(mask);
         } else {
            compute_skip_configurations(skips, masks, newpath, n, maxskips, skipnum + 1, newleftmargin);
         }
    }
}
