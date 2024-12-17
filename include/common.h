#ifndef COLIBRICOMMON_H
#define COLIBRICOMMON_H

#include <string>
#include <list>
#include <vector>
#include <exception>
#include <stdexcept>
#include <cmath>
#include <cstdint>

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

typedef unsigned long bsize_t;
const bsize_t         B32       = pow(2, 32);
const uint32_t        bitmask[] = {1,       1 << 1,  1 << 2,  1 << 3,  1 << 4,  1 << 5,  1 << 6,  1 << 7,  1 << 8,  1 << 9,  1 << 10, 1 << 11, 1 << 12, 1 << 13, 1 << 14, 1 << 15,
                                   1 << 16, 1 << 17, 1 << 18, 1 << 19, 1 << 20, 1 << 21, 1 << 22, 1 << 23, 1 << 24, 1 << 25, 1 << 26, 1 << 27, 1 << 28, 1 << 29, 1 << 30};

std::string           trim(const std::string& t, const std::string& ws);
std::string           get_extension(const std::string& filename);
bool                  strip_extension(std::string& filename, const std::string& extension);
double                listproduct(const std::vector<double>& l);
double                listsum(const std::vector<double>& l);
void                  orderedinsert(std::list<double>& l, double value);
std::vector<std::string>& split(const std::string& s, char delim, std::vector<std::string>& elems);
std::vector<std::string>  split(const std::string& s, char delim);

bool                      test_common();

class InternalError : public std::runtime_error {
  public:
    explicit InternalError() : std::runtime_error("Colibri internal error") {}
};

class KeyError : public std::runtime_error {
  public:
    explicit KeyError() : std::runtime_error("Colibri KeyError") {}
};

class UnknownTokenError : public std::runtime_error {
  public:
    explicit UnknownTokenError() : std::runtime_error("Colibri TokenError: the input contained an unknown token") {}
};
#endif
