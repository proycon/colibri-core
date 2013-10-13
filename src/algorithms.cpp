#include <vector>
#include <utility>

using namespace std;


vector< pair<int,int> > get_consecutive_gaps(const int n, const int leftmargin=1, const int rightmargin=1) {
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

void compute_multi_skips(vector< vector<pair<int,int> > > & skips, const vector<pair<int,int> > & path, const int n, const int maxskips, const int skipnum, const int leftmargin) {    
    vector< pair<int,int> > currentskips = get_consecutive_gaps(n, leftmargin);
    for (vector< pair<int,int> >::iterator iter = currentskips.begin(); iter != currentskips.end(); iter++) {                
         const int newleftmargin = iter->first + iter->second + 1;   
         vector< pair<int,int> > newpath = path; //copy
         newpath.push_back( *iter );
         if ((skipnum + 1 == maxskips) || ( n - newleftmargin <= 2)) {
            skips.push_back( newpath );
         } else {
            compute_multi_skips(skips, newpath, n, maxskips, skipnum + 1, newleftmargin);
         }
    }
}
