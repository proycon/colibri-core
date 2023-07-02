#include "common.h"
#include <glob.h>
#include <sstream>
#include <iostream>
#include <vector>
#include <numeric>

using namespace std;

std::string trim(const std::string &t, const std::string &ws) {
    std::string str = t;
    size_t found;
    found = str.find_last_not_of(ws);
    if (found != std::string::npos)
        str.erase(found+1);
    else
        str.clear();            // str is all whitespace
    return str;
}

std::string get_extension(const std::string& filename) {
    if(filename.find_last_of(".") != std::string::npos)
        return filename.substr(filename.find_last_of(".")+1);
    return "";
}

bool strip_extension(std::string& filename, const std::string& extension) {
    if(filename.find_last_of(".") != std::string::npos)
        if (filename.substr(filename.find_last_of(".")+1) == extension) {
            filename = filename.substr(0,filename.find_last_of("."));
            return true;
        }
    return false;
}

double listproduct(const vector<double> & l) {
  return std::accumulate( l.begin(), l.end(), 1, std::multiplies<double>() );
}

double listsum(const vector<double> & l) {
  return std::accumulate( l.begin(), l.end(), 0 );
}

void orderedinsert(list<double> & l, double value) {
  for (list<double>::iterator iter = l.begin(); iter != l.end(); ++iter ) {
    if (value < *iter) {
      l.insert(iter, value);
      return;
    }
  }
  l.push_back(value);
}


vector<string> & split(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while(getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


vector<string> split(const string &s, char delim) {
    vector<string> elems;
    return split(s, delim, elems);
}

bool test_common(){
  int err_cnt = 0;
  vector<double> v { 1.0, 2.0, 3.0, 4.0 };
  if ( listproduct( v ) != 24.0 ) {
    cerr << "listproduct problem: " << listproduct( v ) << " != 24.0" << endl;
    ++err_cnt;
  }
  if ( listsum( v ) != 10.0 ){
    cerr << "listproduct problem: " << listsum( v ) << " != 10" << endl;
    ++err_cnt;
  }
  list<double> l { 1.0, 2.0, 3.0, 4.0 };
  orderedinsert( l, 3.14 );
  if ( l != list<double>({ 1.0, 2.0, 3.0, 3.14, 4.0 }) ){
    cerr << "orderedinsert problem: { ";
    for ( const auto& it : l ){
      cerr << it;
      if ( &it != &l.back() ){
	cerr << ", ";
      }
    }
    cerr << " } != { 1.0, 2.0, 3.0, 3.14, 4.0 }" << endl;
    ++err_cnt;
  }
  return err_cnt == 0;
}
