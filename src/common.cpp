#include "common.h"
#include <glob.h>
#include <sstream>
#include <vector>

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

bool strip_extension(std::string& filename, const std::string extension) {
    if(filename.find_last_of(".") != std::string::npos)
        if (filename.substr(filename.find_last_of(".")+1) == extension) {
            filename = filename.substr(0,filename.find_last_of("."));
            return true;
        }
    return false;
}





double listproduct(const vector<double> & l) {
    double p = 1;
    for (vector<double>::const_iterator iter = l.begin(); iter != l.end(); iter++) {
        p = p * *iter;
    }
    return p;
}



double listsum(const vector<double> & l) {
    double p = 0;
    for (vector<double>::const_iterator iter = l.begin(); iter != l.end(); iter++) {
        p += *iter;
    }
    return p;
}

void orderedinsert(list<double> & l, double value) {
	for (list<double>::iterator iter = l.begin(); iter != l.end(); iter++) {
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

