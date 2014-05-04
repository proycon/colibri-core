#include "patternstore.h"

template<>
PatternMap<unsigned int> * converttype<PatternMap<unsigned int>, PatternMap<unsigned int>>(PatternMap<unsigned int> * source) { return source; };
