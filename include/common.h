#ifndef COLIBRICOMMON_H
#define COLIBRICOMMON_H

#include <string>
#include <list>
#include <vector>
#include <exception>

/**
 * @file common.h
 * \brief Basic largely trivial functions for the common good.
 *
 * @author Maarten van Gompel (proycon) <proycon@anaproy.nl>
 * 
 * @section LICENSE
 * Licensed under GPLv3
 *
 * @section DESCRIPTION
 * Basic largely trivial functions for the common good.
 */

std::string trim(const std::string &t, const std::string &ws);
std::string get_extension(const std::string& filename);
bool strip_extension(std::string& filename, const std::string extension);
double listproduct(const std::vector<double> & l);
double listsum(const std::vector<double> & l);
void orderedinsert(std::list<double> & l, double value);
std::vector<std::string> & split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);

class InternalError: public std::exception {
  virtual const char* what() const throw()
  {
    return "Colibri internal error";
  }
};

class KeyError: public std::exception {
  virtual const char* what() const throw()
  {
    return "Colibri KeyError";
  }
};
#endif
