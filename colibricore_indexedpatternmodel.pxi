cpdef getdata(self, Pattern pattern):
    """Return the indices at which the pattern occurs"""
    cdef cIndexedData cvalue
    if pattern in self:
        cvalue = self.data[pattern.cpattern]
        value = IndexedData()
        value.bind(cvalue)
        return value
    else:
        raise KeyError


cpdef add(self, Pattern pattern, indices):
    """Add a pattern to the indexed model
    :param pattern: The pattern to add
    :type pattern: Pattern
    :param indices: list (or other iterable) of 2-tuples specifying the (sentence,index) of each occurrence of the pattern
    """

    cdef cIndexedData * cvalue
    cvalue = &(self.data[pattern.cpattern])
    cdef cIndexReference ref
    for sentence,token in indices:
        ref.sentence = <int> sentence
        ref.token = <int> token
    self.data.add(pattern.cpattern, cvalue, ref)

def __iter__(self):
    """Iterate over all patterns in this model"""
    it = self.data.begin()
    cdef cPattern cpattern
    while it != self.data.end():
        cpattern = deref(it).first
        pattern = Pattern()
        pattern.bind(cpattern)
        yield pattern
        inc(it)

def items(self):
    """Iterate over all patterns and their index data (IndexedData instances) in this model"""
    it = self.data.begin()
    cdef cPattern cpattern
    cdef cIndexedData cvalue
    while it != self.data.end():
        cpattern = deref(it).first
        cvalue = deref(it).second
        pattern = Pattern()
        pattern.bind(cpattern)
        value = IndexedData()
        value.bind(cvalue)
        yield (pattern,value)
        inc(it)





def covered(self, indexreference):
    if not isinstance(indexreference, tuple) or not len(indexreference) == 2:
        raise ValueError("Expected tuple")
    cdef int sentence = indexreference[0]
    cdef int token = indexreference[1]
    cdef cIndexReference ref = cIndexReference(sentence, token)
    cdef cIndexReference ref2
    cdef unordered_set[cPatternPointer] results = self.data.getreverseindex(ref)
    cdef unordered_set[cPatternPointer] results2
    cdef unordered_set[cPatternPointer].iterator resit
    cdef cPattern cpattern
    if not results.empty():
        return True
    else:
        for i in range(1, token+1):
            if token-i >= 0:
                ref2 = cIndexReference(sentence, token-i)
                results2 = self.data.getreverseindex(ref2)
                resit = results2.begin()
                while resit != results2.end():
                    cpattern = deref(resit).pattern()
                    if cpattern.n() >= token+1:
                        return True
                    inc(resit)
        return False







cpdef outputrelations(self, Pattern pattern, ClassDecoder decoder):
    """Compute and output (to stdout) all relations for the specified pattern:

    :param pattern: The pattern to output relations for
    :type pattern: Pattern
    :param decoder: The class decoder
    :type decoder: ClassDecoder
    """
    self.data.outputrelations(pattern.cpattern,decoder.data,&cout)


def getsubchildren(self, Pattern pattern, int occurrencethreshold = 0, int category = 0, int size = 0):
    """Get subsumption children for the specified pattern
    :param pattern: The pattern
    :type pattern: Pattern
    :param occurrencethreshold: Constrain by patterns occurring at least this many times in this relationship (default: 0, unconstrained)
    :type occurrencethreshold: int
    :param category: Constrain by patterns of the specified category (colibricore.Category.NGRAM,colibricore.Category.SKIPGRAM, colibricore.Category.FLEXGRAM)
    :param size: Constrain by patterns of the specified size
    :type size: int
    :rtype: generator over (Pattern,value) tuples. The values correspond to the number of occurrences for this particularrelationship
    """
    if self.data.reverseindex == NULL:
        raise ValueError("No reverseindex was specified but this method requires it, set reverseindex to an IndexedCorpus instance upon model construction")

    cdef t_relationmap  relations = self.data.getsubchildren(pattern.cpattern, occurrencethreshold, category, size)
    cdef t_relationmap_iterator relit = relations.begin()

    cdef cPattern cpattern
    cdef unsigned int value
    while relit != relations.end():
        cpattern = deref(relit).first
        value = deref(relit).second
        pattern = Pattern()
        pattern.bind(cpattern)
        yield (pattern,value)
        inc(relit)

def getsubparents(self, Pattern pattern, int occurrencethreshold = 0, int category = 0, int size = 0):
    """Get subsumption parents for the specified pattern
    :param pattern: The pattern
    :type pattern: Pattern
    :param occurrencethreshold: Constrain by patterns occurring at least this many times in this relationship (default: 0, unconstrained)
    :type occurrencethreshold: int
    :param category: Constrain by patterns of the specified category (colibricore.Category.NGRAM,colibricore.Category.SKIPGRAM, colibricore.Category.FLEXGRAM)
    :param size: Constrain by patterns of the specified size
    :type size: int
    :rtype: generator over (Pattern,value) tuples. The values correspond to the number of occurrences for this particularrelationship
    """
    if self.data.reverseindex == NULL:
        raise ValueError("No reverseindex was specified but this method requires it, set reverseindex to an IndexedCorpus instance upon model construction")

    cdef t_relationmap  relations = self.data.getsubparents(pattern.cpattern, occurrencethreshold, category, size)
    cdef t_relationmap_iterator relit = relations.begin()
    cdef cPattern cpattern
    cdef unsigned int value
    while relit != relations.end():
        cpattern = deref(relit).first
        value = deref(relit).second
        pattern = Pattern()
        pattern.bind(cpattern)
        yield (pattern,value)
        inc(relit)

def getleftneighbours(self, Pattern pattern, int occurrencethreshold = 0, int category = 0, int size = 0, int cutoff = 0):
    """Get left neighbours for the specified pattern
    :param pattern: The pattern
    :type pattern: Pattern
    :param occurrencethreshold: Constrain by patterns occurring at least this many times in this relationship (default: 0, unconstrained)
    :type occurrencethreshold: int
    :param category: Constrain by patterns of the specified category (colibricore.Category.NGRAM,colibricore.Category.SKIPGRAM, colibricore.Category.FLEXGRAM)
    :param size: Constrain by patterns of the specified size
    :type size: int
    :rtype: generator over (Pattern,value) tuples. The values correspond to the number of occurrences for this particularrelationship
    """
    if self.data.reverseindex == NULL:
        raise ValueError("No reverseindex was specified but this method requires it, set reverseindex to an IndexedCorpus instance upon model construction")

    cdef t_relationmap  relations = self.data.getleftneighbours(pattern.cpattern, occurrencethreshold, category, size,cutoff)
    cdef t_relationmap_iterator relit = relations.begin()
    cdef cPattern cpattern
    cdef unsigned int value
    while relit != relations.end():
        cpattern = deref(relit).first
        value = deref(relit).second
        pattern = Pattern()
        pattern.bind(cpattern)
        yield (pattern,value)
        inc(relit)

def getrightneighbours(self, Pattern pattern, int occurrencethreshold = 0, int category = 0, int size = 0, int cutoff = 0):
    """Get right neighbours for the specified pattern
    :param pattern: The pattern
    :type pattern: Pattern
    :param occurrencethreshold: Constrain by patterns occurring at least this many times in this relationship (default: 0, unconstrained)
    :type occurrencethreshold: int
    :param category: Constrain by patterns of the specified category (colibricore.Category.NGRAM,colibricore.Category.SKIPGRAM, colibricore.Category.FLEXGRAM)
    :param size: Constrain by patterns of the specified size
    :type size: int
    :rtype: generator over (Pattern,value) tuples. The values correspond to the number of occurrences for this particularrelationship
    """
    if self.data.reverseindex == NULL:
        raise ValueError("No reverseindex was specified but this method requires it, set reverseindex to an IndexedCorpus instance upon model construction")

    cdef t_relationmap  relations = self.data.getrightneighbours(pattern.cpattern, occurrencethreshold, category, size,cutoff)
    cdef t_relationmap_iterator relit = relations.begin()
    cdef cPattern cpattern
    cdef unsigned int value
    while relit != relations.end():
        cpattern = deref(relit).first
        value = deref(relit).second
        pattern = Pattern()
        pattern.bind(cpattern)
        yield (pattern,value)
        inc(relit)

def getskipcontent(self, Pattern pattern):
    """Get skip content for the specified pattern
    :param pattern: The pattern
    :type pattern: Pattern
    :rtype: generator over (Pattern,value) tuples. The values correspond to the number of occurrence for this particularrelationship
    """
    if self.data.reverseindex == NULL:
        raise ValueError("No reverseindex was specified but this method requires it, set reverseindex to an IndexedCorpus instance upon model construction")

    cdef t_relationmap  relations = self.data.getskipcontent(pattern.cpattern)
    cdef t_relationmap_iterator relit = relations.begin()
    cdef cPattern cpattern
    cdef unsigned int value
    while relit != relations.end():
        cpattern = deref(relit).first
        value = deref(relit).second
        pattern = Pattern()
        pattern.bind(cpattern)
        yield (pattern,value)
        inc(relit)

def gettemplates(self, Pattern pattern, int occurrencethreshold = 0):
    """Get templates (abstracting skipgrams) for the specified pattern
    :param pattern: The pattern
    :type pattern: Pattern
    :param occurrencethreshold: Constrain by patterns occurring at least this many times in this relationship (default: 0, unconstrained)
    :type occurrencethreshold: int
    :rtype: generator over (Pattern,value) tuples. The values correspond to the number of occurrence for this particularrelationship
    """
    if self.data.reverseindex == NULL:
        raise ValueError("No reverseindex was specified but this method requires it, set reverseindex to an IndexedCorpus instance upon model construction")

    cdef t_relationmap  relations = self.data.gettemplates(pattern.cpattern, occurrencethreshold)
    cdef t_relationmap_iterator relit = relations.begin()
    cdef cPattern cpattern
    cdef unsigned int value
    while relit != relations.end():
        cpattern = deref(relit).first
        value = deref(relit).second
        pattern = Pattern()
        pattern.bind(cpattern)
        yield (pattern,value)
        inc(relit)

def getinstances(self, Pattern pattern, int occurrencethreshold = 0):
    """Get templates (abstracting skipgrams) for the specified pattern
    :param pattern: The pattern
    :type pattern: Pattern
    :param occurrencethreshold: Constrain by patterns occurring at least this many times in this relationship (default: 0, unconstrained)
    :type occurrencethreshold: int
    :rtype: generator over (Pattern,value) tuples. The values correspond to the number of occurrence for this particularrelationship
    """
    if self.data.reverseindex == NULL:
        raise ValueError("No reverseindex was specified but this method requires it, set reverseindex to an IndexedCorpus instance upon model construction")

    cdef t_relationmap  relations = self.data.getinstances(pattern.cpattern, occurrencethreshold)
    cdef t_relationmap_iterator relit = relations.begin()
    cdef cPattern cpattern
    cdef unsigned int value
    while relit != relations.end():
        cpattern = deref(relit).first
        value = deref(relit).second
        pattern = Pattern()
        pattern.bind(cpattern)
        yield (pattern,value)
        inc(relit)

def getcooc(self, Pattern pattern,  int occurrencethreshold = 0, int category = 0, int size = 0):
    """Get left-side co-occurrences for the specified pattern
    :param pattern: The pattern
    :type pattern: Pattern
    :param occurrencethreshold: Constrain by patterns occurring at least this many times in this relationship (default: 0, unconstrained)
    :type occurrencethreshold: int
    :param category: Constrain by patterns of the specified category (colibricore.Category.NGRAM,colibricore.Category.SKIPGRAM, colibricore.Category.FLEXGRAM)
    :param size: Constrain by patterns of the specified size
    :type size: int
    :rtype: generator over (Pattern,value) tuples. The values correspond to the number of occurrence for this particularrelationship
    """
    if self.data.reverseindex == NULL:
        raise ValueError("No reverseindex was specified but this method requires it, set reverseindex to an IndexedCorpus instance upon model construction")

    cdef t_relationmap  relations = self.data.getcooc(pattern.cpattern, occurrencethreshold, category, size)
    cdef t_relationmap_iterator relit = relations.begin()
    cdef cPattern cpattern
    cdef unsigned int value
    while relit != relations.end():
        cpattern = deref(relit).first
        value = deref(relit).second
        pattern = Pattern()
        pattern.bind(cpattern)
        yield (pattern,value)
        inc(relit)

def getleftcooc(self, Pattern pattern, int occurrencethreshold = 0, int category = 0, int size = 0):
    """Get left-side co-occurrences for the specified pattern
    :param pattern: The pattern
    :type pattern: Pattern
    :param occurrencethreshold: Constrain by patterns occurring at least this many times in this relationship (default: 0, unconstrained)
    :type occurrencethreshold: int
    :param category: Constrain by patterns of the specified category (colibricore.Category.NGRAM,colibricore.Category.SKIPGRAM, colibricore.Category.FLEXGRAM)
    :param size: Constrain by patterns of the specified size
    :type size: int
    :rtype: generator over (Pattern,value) tuples. The values correspond to the number of occurrence for this particularrelationship
    """
    if self.data.reverseindex == NULL:
        raise ValueError("No reverseindex was specified but this method requires it, set reverseindex to an IndexedCorpus instance upon model construction")

    cdef t_relationmap  relations = self.data.getleftcooc(pattern.cpattern, occurrencethreshold, category, size)
    cdef t_relationmap_iterator relit = relations.begin()
    cdef cPattern cpattern
    cdef unsigned int value
    while relit != relations.end():
        cpattern = deref(relit).first
        value = deref(relit).second
        pattern = Pattern()
        pattern.bind(cpattern)
        yield (pattern,value)
        inc(relit)

def getrightcooc(self, Pattern pattern, int occurrencethreshold = 0, int category = 0, int size = 0):
    """Get right-side co-occurrences for the specified pattern
    :param pattern: The pattern
    :type pattern: Pattern
    :param occurrencethreshold: Constrain by patterns occurring at least this many times in this relationship (default: 0, unconstrained)
    :type occurrencethreshold: int
    :param category: Constrain by patterns of the specified category (colibricore.Category.NGRAM,colibricore.Category.SKIPGRAM, colibricore.Category.FLEXGRAM)
    :param size: Constrain by patterns of the specified size
    :type size: int
    :rtype: generator over (Pattern,value) tuples. The values correspond to the number of occurrence for this particularrelationship
    """
    if self.data.reverseindex == NULL:
        raise ValueError("No reverseindex was specified but this method requires it, set reverseindex to an IndexedCorpus instance upon model construction")

    cdef t_relationmap  relations = self.data.getrightcooc(pattern.cpattern, occurrencethreshold, category, size)
    cdef t_relationmap_iterator relit = relations.begin()
    cdef cPattern cpattern
    cdef unsigned int value
    while relit != relations.end():
        cpattern = deref(relit).first
        value = deref(relit).second
        pattern = Pattern()
        pattern.bind(cpattern)
        yield (pattern,value)
        inc(relit)

def computeflexgrams_fromskipgrams(self):
    """Compute flexgrams from skipgrams in the model. Returns the number of flexgrams found."""
    return self.data.computeflexgrams_fromskipgrams()

def computeflexgrams_fromcooc(self,double threshold):
    """Compute flexgrams based on  co-occurence. The threshold is expressed in normalised pointwise mutual information. Returns the number of flexgrams found. Flexgrams contain at only one gap using this method."""
    return self.data.computeflexgrams_fromcooc(threshold)
