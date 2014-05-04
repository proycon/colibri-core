#include "datatypes.h"


template<>
unsigned int * converttype<unsigned int, unsigned int>(unsigned int * source) { return source; };

template<>
IndexedData * converttype<IndexedData, IndexedData>(IndexedData * source) { return source; };

template<>
PatternFeatureVector<double> * converttype<  PatternFeatureVector<double>, PatternFeatureVector<double>>(PatternFeatureVector<double> * source) { return source; };

template<>
PatternFeatureVectorMap<double> * converttype<PatternFeatureVectorMap<double>>(PatternFeatureVectorMap<double> * source) { return source; }

template<>
unsigned int * converttype<IndexedData, unsigned int>(IndexedData * source) { unsigned int * t = new unsigned int; *t = source->count(); return t; };

template<>
IndexedData * converttype<unsigned int, IndexedData>(unsigned int * source) { return new IndexedData(); };

template<>
unsigned int* converttype<PatternFeatureVectorMap<double>, unsigned int>(PatternFeatureVectorMap<double>* source) { unsigned int * t = new unsigned int; *t = source->count(); return t; }

template<>
unsigned int* converttype<PatternFeatureVector<double>, unsigned int>(PatternFeatureVector<double>* source) { unsigned int * t = new unsigned int; *t = source->size(); return t; }

template<>
IndexedData * converttype<PatternFeatureVectorMap<double>, IndexedData>(PatternFeatureVectorMap<double>* source) { return new IndexedData(); }
