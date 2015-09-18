.. colibri documentation master file, created by
   sphinx-quickstart on Mon Oct  8 11:38:12 2012.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.


****************************
Colibri Documentation 
****************************

.. toctree::
   :maxdepth: 3

Introduction
===================


Colibri Core is software consisting of command line tools as well as programming libraries. to quickly and efficiently count and extract patterns from large corpus data, to extract various statistics on the extracted patterns, and to compute relations between the extracted patterns. The employed notion of pattern or construction encompasses the following categories:

 * **n-gram** -- n consecutive words
 * **skipgram** -- An abstract pattern of predetermined length with one or multiple gaps (of specific size).
 * **flexgram** -- An abstract pattern without predetermined length, with one or more gaps.

N-gram extraction may seem fairly trivial at first, with a few lines in your favourite scripting language, you can move a simple sliding window of size *n* over your corpus and store the results in some kind of hashmap. This trivial approach however makes an unnecessarily high demand on memory resources, this often becomes prohibitive if unleashed on large corpora. Colibri Core tries to minimise these space requirements in several ways:

 * **Binary representation** -- Each word type is assigned a numeric class, which is encoded in a compact binary format in which highly frequent classes take less space than less frequent classes. Colibri core always uses this representation rather than a full string representation, both on disk and in memory.
 * **Informed counting** -- Counting is performed more intelligently by iteratively processing the corpus in several passes and quickly discarding patterns that won't reach the desired occurrence threshold.

Skipgram and flexgram extraction are computationally more demanding but have been implemented with similar optimisations. Skipgrams are computed by abstracting over n-grams, and flexgrams in turn are computed either by abstracting over skipgrams, or directly from n-grams on the basis of co-occurrence information (mutual pointwise information). When patterns have been extracted, along with their counts and or index references to original corpus data, they form a so-called *pattern model*.

At the heart of Colibri Core lies the tool ``colibri-patternmodeller`` which allows you to build, view, manipulate and query pattern models. 

The Colibri software is developed in the scope of the Ph.D. research project **Constructions as Linguistic Bridges**. This research examines the identification and extraction of aligned constructions or patterns across natural languages, and the usage of such constructions in Machine Translation. The aligned constructions are not identified on the basis of an extensive and explicitly defined grammar or expert database of linguistic knowledge, but rather are implicitly distilled from large amounts of example data. Our notion of constructions is broad and transcends the idea of words or variable-length phrases.

This documentation will illustrate how to work with the various tools and the library of colibri, as well as elaborate on the implementation of certain key aspects of the software.

Installation
===============

Installation via LaMachine
------------------------------------------------------------

Colibri Core is included in the `LaMachine <https://proycon.github.io/LaMachine>`_ distribution. This includes all dependencies and other NLP software. LaMachine can also run as a virtual machine on any host OS.

Installing dependencies
-------------------------------

To compile Colibri Core, you need a sane build environment, install the necessary dependencies for your distribution.

For Debian/Ubuntu::

    $ sudo apt-get install make gcc g++ pkg-config autoconf-archive libtool autotools-dev libbz2-dev zlib1g-dev libtar-dev python3 python3-dev cython3" 

For RedHat-based systems (run as root)::

    # yum install pkgconfig libtool autoconf automake autoconf-archive make gcc gcc-c++ libtar libtar-devel python3 python3-devel zlib zlib-devel python3-pip bzip2 bzip2-devel cython3 

For Mac OS X with `homebrew <http://brew.sh>`_::

    $ brew install autoconf automake libtool autoconf-archive python3

Arch Linux users can simply install Colibri Core and all dependencies directly from the `Arch User Repository <https://aur.archlinux.org/packages/colibri-core-git>`_ , no further installation is necessary in this case.

Installation via the Python Package Index
------------------------------------------------------------

Colibri Core can be installed from the `Python Package Index
<https://pypi.python.org/pypi/colibricore>`_ using the ``pip`` tool, often
named ``pip3`` for the Python 3 version, which we recommend. This procedure
will automatically download, compile, and install all of Colibi Core.

First ensure you installed all dependencies from the previous section!

Colibri Core requires an up-to-date version of Cython first (0.23 or above), or the installation will fail, we use ``pip`` to compile it from scratch::

    $ sudo pip3 install cython

Then we can install Colibri Core itself::

    $ sudo pip3 install colibricore

For installation without root privileges we recommend creating a `Python Virtual environment,
<https://virtualenv.pypa.io/en/latest/>`_ , in which case all of Colibri Code will be installed under it::

    $ virtualenv --python=python3 coco
    $ . coco/bin/activate       #you will need to do this each time you want to use Colibri Core
    (coco)$ pip install cython
    (coco)$ pip install colibricore

Alternatively, pass a prefix::

    $ pip3 install --install-option="--prefix=/my/installation/directory" colibricore

**Important Note:** If you install Colibri Core locally (in a Python Virtual Environment
or elsewhere), then you need to set ``LD_LIBRARY_PATH=$VIRTUAL_ENV/lib/`` prior
to running Python for the Python binding to function (replace ``$VIRTUAL_ENV`` with
the directory you used as a prefix if you do not use Python Virtual
Environment). Otherwise, you will be confronted with an error: ``ImportError:
libcolibricore.so.0: cannot open shared object file: No such file or
directory``. If you get this error after a global installaton, run ``sudo
ldconfig``.

Installation from Github
-----------------------------

First ensure you installed all dependencies from the dependencies section!

Colibri Core is hosted on `github <https//github.com/proycon/colibri-core/>`_
and should ideally be retrieved through the versioning control system ``git``.
Provided git is installed on your system, this is done as follows::

	$ git clone https://github.com/proycon/colibri-core.git

Alternatively, you can download and extract release archives from the
aforementioned Github page.

Now we can install Colibri Core itself, the following will install everything globally under ``/usr/`` and hence requires administrative privileges::

    $ sudo python3 setup.py install

If you install from within a `Python Virtual environment,
<https://virtualenv.pypa.io/en/latest/>`_ everything will be
installed under it. This does not require root privileges::

    $ virtualenv --python=python3 coco
    $ . coco/bin/activate       #you will need to do this each time you want to use Colibri Core
    (coco)$ pip install cython
    (coco)$ pip install colibricore

For local installation elsewhere, pass a prefix, for example::

    $ python3 setup.py install --prefix=/home/yourname/local

The note at the end of the previous section applies for any non-global installation!

Manual compilation and installation (advanced)
-------------------------------------------------

Installation via the Python Package Index or Github invokes compilation of the C++ source for you.
You can however to compile everything yourself. This is especially relevant if
you are not only interested in command-line tools or C++ library, and do not
care about the Python library.

Moreover, this route is also needed if you want FoLiA support (optional), for which you
then first to install the following dependency. By default FoLiA support is disabled.

 * **libfolia**; obtainable from `the FoLiA website <http://proycon.github.com/folia>`_,  follow the instructions included with libfolia to install it.

Including pulling the sources from github, Colibri Core can be compiled and
installed as follows::

  $ git clone https://github.com/proycon/colibri-core.git
  $ cd colibri-core
  $ bash bootstrap
  $ ./configure [--prefix=/usr] [--with-folia --with-folia-path=/path/to/libfolia]
  $ make
  $ sudo make install

If not prefix is set, installation will be under ``/usr/local/`` by default. 

To compile the Python binding manually, we first create an empty file
``manual`` which signals the build process to not attempt to recompile the C++
library itself::

  $ touch manual

Then build as follows, provided Colibri Core is in a globally accessible
location already::

  $ sudo python3 ./setup.py install

If you used a prefix and want to install in a customised non-global location,
again use ``--prefix`` along with ``--include-dirs`` and ``--library-dirs`` to
point to where the C++ headers and the library are installed::

  $ python3 ./setup.py build_ext --include-dirs=/path/to/include/colibri-core  --library-dirs=/path/to/lib/  install --prefix=/path/to/somewhere/

Update ``$LD_LIBRARY_PATH`` and ``$PYTHONPATH`` where necessary.
 
Keeping colibri up to date
-----------------------------

It is recommended to often check back for new versions of Colibri Core.

If you used, ``pip`` just run::

 $ pip3 install -U colibricore

For the git version::

 $ git pull
 
And then recompile as per the above instructions.

General usage instructions
---------------------------------

Colibri consist of various programs and scripts, each of which will output an
extensive overview of available parameters if the parameter ``-h`` is passed.
Each program is designed for a specialised purpose, with specific input and
output formats. It is often needed to call multiple programs in succession to
obtain the final analysis or model you desire. 

Quick start: High-level scripts
=================================

Introduction
---------------

Colibri Core comes with a set of scripts that provide simpler access to the
underlying tools and can be used from the command line by end-users to get
quick results. Input to these tools is always one or more plain text files, in
tokenised form, with one sentence per line.

Tokenisation
--------------

If your corpus is not tokenised yet, you can consider using the tokeniser `ucto
<http://ilk.uvt.nl/ucto>`_ , which is not part of Colibri Core, Debian/Ubuntu
users may it in the repository (``sudo apt-get install ucto``), Mac OS X users
can find it in homebrew (``brew install naiaden/lama/ucto``). This will also do
sentence detection and, with the ``-n`` flag output one line per sentence, as
Colibri prefers::

	$ ucto -L en -n untokenisedcorpus.txt > tokenisedcorpus.txt

The ``-L`` specifies the language of your corpus (English in this case), several others are
available as well. 

Of course, you can use any other tokeniser of your choice.


Scripts
-----------

In addition to the core tools, described in the remainder of this
documentation, Colibri Core offers the following scripts:

 * ``colibri-ngrams`` - Extracts n-grams of a particular size from the corpus
   text, in the order they occur, i.e. by moving a sliding window over the text.
 * ``colibri-freqlist`` - Extracts all n-grams from one or more corpus text files and
    outputs a frequency list. Also allows for the extraction of skipgrams. By
    default all n-grams are extract, but an occurrence threshold can be set with the ``-t`` flag.
 * ``colibri-ngramstats`` - Prints a summary report on the ngrams in one or
    more corpus text files. To get the full details on interpreting the output report,
    read the section *Statistical Reports and Histograms*.
 * ``colibri-histogram`` - Prints a histogram of ngram/skipgram occurrence count
 * ``colibri-queryngrams`` - Interactive tool allowing you to query ngrams
   from standard input, various statistics and relations can be outputted.
 * ``colibri-reverseindex`` - Computes and prints a reverse index for the
   specified corpus text file. For each token position in the corpus, it will
   output what patterns are found there (i.e start at that very same position)
 * ``colibri-loglikelihood`` - Computes the log-likelihood between patterns in
   two or more corpus text files, which allows users to determine what words or
   patterns are significantly more frequent in one corpus than the other.
 * ``colibri-coverage`` - Computes overlap between a training corpus and a test
   corpus, produces coverage metrics.

Users have to be aware, however, that these script only expose a
limited amount of the functionality of Colibri Core.


Corpus Class Encoding
================================

Introduction
----------------------

Computation on large datasets begs for solutions to keep memory consumption
manageable. Colibri requires that input corpora are converted into a compressed
binary form. The vocabulary of the corpus is converted to integer form, i.e.
each word-type in the corpus is represented by a numeric class. Highly frequent
word-types get assigned low class numbers and less frequent word-types get
higher class numbers. The class is represented in a dynamic-width byte-array,
rather than a fixed-width integer. Patterns are encoded per word, each word
starts with a size marker of one byte indicating the number of bytes are used
for that word. The specified number of bytes that follow encode the word class.
Instead of a size marker, byte values of 128 and above are reserved for special
markers, such as encoding gaps and structural data. Finally, the pattern as a
whole is ended by a null byte.

All internal computations of all tools in colibri proceed on this internal
representation rather than actual textual strings, keeping running time shorter
and memory footprint significantly smaller.

Class-encoding your corpus
-----------------------------------

When working with colibri, you first want to **class encode** your corpus. This is done by the program ``colibri-classencode``. It takes as input a *tokenised* monolingual corpus in plain text format, containing *one sentence per line*, as a line is the only structural unit Colibri works with, extracted patterns will never cross line boundaries. Each line should be delimited by a single newline character (unix line endings). If you desire another structural unit (such as for example a tweet, or a paragraph), simply make sure each is on one line. 

Colibri is completely agnostic when it comes to the character encoding of the input. Given a corpus file ``yourcorpus``, class encoding is done as follows::

	$ colibri-classencode yourcorpus

This results in two files:

 * ``yourcorpus.colibri.cls`` - This is the class file; it lists all word-types and class numbers.  
 * ``yourcorpus.colibri.dat`` - This is the corpus is encoded binary form. It is a lossless compression that is roughly half the size of the original  

If your corpus is not tokenised yet, you can consider using the tokeniser `ucto <http://ilk.uvt.nl/ucto>`_ (not part of colibri), this will also do sentence detection and output one line per sentence::

	$ ucto -L en -n untokenisedcorpus.txt > tokenisedcorpus.txt
	
The above sample is for English (``-L en``), several other languages are also supported.

In addition to this plain text input. The class encoder also supports *FoLiA XML* (`folia website <http://proycon.github.com/folia>`_) if you compiled with FoLiA support, make sure such files end with the extension ``xml`` and they will be automatically interpreted as FoLiA XML::

	$ colibri-classencode yourcorpus.xml
	

The class file is the vocabulary of your corpus, it simply maps word strings to integer. You must always ensure that whenever you are working with multiple models, and you want to compare them, to use the exact same class file. It is possible to encode multiple corpus files similtaneously,  generating a joined class file::

	$ colibri-classencode yourcorpus1.txt yourcorpus2.txt
	
This results in ``yourcorpus1.colibri.cls`` and ``yourcorpus1.colibri.dat`` and ``yourcorpus2.colibri.dat``. The class file spans both despite the name. An explicit name can be passed with the ``-o`` flag. It is also possible to encode multiple corpora in a single unified file by passing the ``-u`` flag. This is often desired if you want to train a pattern model on all the joined data::

	$ colibri-classencode -o out -u yourcorpus1.txt yourcorpus2.txt

This will produce ``out.colibri.dat`` and ``out.colibri.cls``. You can use the ``-l`` option to read input filenames from file instead of command line arguments (one filename per line).

If you have a pre-existing class file you can load it with the ``-c`` flag, and use it to encode new data::

    $ colibri-classencode -c yourcorpus1.colibri.cls yourcorpus2.txt

This will produce a ``yourcorpus2.colibri.dat``, provided that all of the word types already existed in ``yourcorpus1.colibri.cls`` (which usually is not the case, in which case an error will be shown. 
To circumvent this error you have to specify how to deal with unknown words. There are two ways; the ``-U`` flag will encode all unknown word as a single word class dedicated to the task, whereas the ``-e`` flag will *extend* the specified class file with any new classes found. It has to be noted that this extension method spoils the optimal compression as classes are no longer strictly sorted by frequency. If you can all needed data in one go, then that is always preferred.

This setup, however, is often seen in a train/test paradigm::

   $ colibri-classencode -f testset.txt -c trainset.colibri.cls -e 

This will result in an encoded corpus ``testset.colibri.dat`` and an *extended* class file ``testset.colibri.cls``, which is a superset of the original ``trainset.cls``, adding only those classes that did not yet exist in the training data.

Class-decoding your corpus
------------------------------

Given an encoded corpus and a class file, the original corpus can always be reconstructed (unless the ``-U`` option was used in encoding to allow unknown words). This we call *class decoding* and is done using the ``colibri-classdecode`` program::
   
 $ colibri-classdecode -f yourcorpus.colibri.dat -c yourcorpus.colibri.cls

Partial decoding can be done by specifying start and end line numbers using the
flags ``-s`` and ``-e`` respectively.

Output will be to ``stdout``, you can redirect it to a file as follows::

 $ colibri-classdecode -f yourcorpus.colibri.dat -c yourcorpus.colibri.cls > yourcorpus.txt








Pattern Modeller
===============================

Introduction
-----------------------

The ``colibri-patternmodeller`` program is used to create pattern models capturing recurring patterns from a monolingual corpus. The extracted patterns are n-grams or skip-grams, where a skip-gram is an n-gram with one or more gaps of either a predefined size, thus containing unspecified or wildcard tokens, or of dynamic width.

In the internal pattern representation, in the place of the size marker, byte
value 128 is used for a fixed gap of a single token, and can be repeated for
gaps of longer length, byte value 129 is used for a gap of unspecified dynamic
width. 

The pattern finding algorithm is iterative in nature and is guaranteed to find
all n-grams above a specified occurrence threshold and optionally given a
maximum size for n. It does so by iterating over the corpus n times, iterating
over all possible values for n in ascending order. At each iteration, a sliding
window extracts all n-grams in the corpus for the size is question. An n-gram
is counted in a hashmap data structure only if both n-1-grams it by definition
contains are found during the previous iteration with an occurrence above the
set threshold.  The exception are unigrams, which are all by definition counted
if they reach the threshold, as they are already atomic in nature. At the end
of each iteration, n-grams not making the occurrence threshold are
pruned. This simple iterative technique reduces the memory footprint compared
to the more naive approach of immediately storing all in a hashmap, as it
prevents the storing of lots of patterns not making the threshold by discarding
them at an earlier stage. 

At the beginning of each iteration of n, all possible ways in which any n-gram
of size *n* can contain gaps is computed. When an n-gram is found, various
skip-grams are tried in accordance with these gap configurations. This is
accomplished by 'punching holes' in the n-gram, resulting in a skip-gram. If
all consecutive parts of this skip-gram were counted during previous iterations
and thus made the threshold, then the skip-gram as a whole is counted,
otherwise it is discarded. After each iteration, pruning again takes places to
prune skip-grams that are not frequent enough.

The pattern finder can create either indexed or unindexed models. For indexed
models, the precise location of where an n-gram or skipgram instance was found
in the corpus is recorded. This comes at the cost of much higher memory usage,
but is necessary for more strongly constrained skip extraction, as well as for
extracting relations between patterns at a later stage. Indexed models by
default also maintain a reverse index allowing, and even unindexed models do so
during building.

Note that for fixed-size skipgrams in indexed models, the various fillings
for the gaps can be reconstructed precisely.

If you are only interested in simple n-gram or simple skip-gram counts, then an
unindexed model may suffice. 

Creating a pattern model
----------------------------

First make sure to have class-encoded your corpus. Given this encoded corpus,
``colibri-patternmodeller`` can be invoked to produce an indexed pattern model.
Always specify the output file using the ``-o`` flag. The occurrence threshold
is specified with parameter ``-t``, patterns occuring less will not be counted.
The default value is two.  The maximum value for n, i.e. the maximum
n-gram/skipgram size, can be restricted using the parameter ``-l``.:: 

	$ colibri-patternmodeller -f yourcorpus.dat -t 10 -o yourcorpus.colibri.indexedpatternmodel
	
This outputted model ``yourcorpus.colibri.indexedpatternmodel`` is stored in a
binary format. To print it into a human readable presentation it needs to be
decoded. The ``colibri-patternmodeller`` program can do this by specifying an
input model using the ``-i`` flag, the class file using the ``-c`` parameter,
and the desired action to print it all using ``-P``::

	$ colibri-patternmodeller -i yourcorpus.colibri.indexedpatternmodel -c yourcorpus.colibri.cls -P


Optionally, instead of or in addition to outputting a model to file using
``-o``, you can also print it directly with ``-P``.
	
Output will be to ``stdout`` in a tab delimited format, with the first line
reserved for the header. This facilitates easy parsing as you can just load it
into any software accepting CSV files, such as spreadsheets. An excerpt
follows::

    PATTERN	COUNT	TOKENS	COVERAGE	CATEGORY	SIZE	FREQUENCY	REFERENCES
    For	2	2	0.0059	ngram	1	0.0121	11:0 15:0
    death	2	2	0.0059	ngram	1	0.0121	11:5 23:7
    who	2	2	0.0059	ngram	1	0.0121	15:1 21:5
    .	4	4	0.0118	ngram	1	0.0242	5:6 9:4 10:6 13:4
    be	4	4	0.0118	ngram	1	0.0242	1:1 1:5 9:2 35:3
    flee	2	2	0.0059	ngram	1	0.0121	36:1 36:5
    not to	4	8	0.0235	ngram	2	0.1538	1:3 36:3 37:3 38:3


The various columns are:

* **Pattern** - The actual pattern. Gaps in skipgrams are represented as ``{*x*}`` where x is a number representing the size of the skip. Variable-width skipgrams are just ``{*}``. 
* **Occurrence count** - The absolute number of times this pattern occurs
* **Tokens** - The absolute number of tokens in the corpus that this pattern covers. Longer patterns by definition cover more tokens. This value's maximum is ``occurrencecount * n``, the value will be smaller if a pattern overlaps itself.
* **Coverage** - The number of covered tokens, as a fraction of the total number of tokens.
* **Category** - The type of pattern (ngram, skipgram or flexgram).
* **Size** - The length of the n-gram or skipgram in words/tokens.
* **Frequency** - The frequency of the pattern *within its category and
  size class*, so for an ngram of size two, the frequency indicates the
  frequency amongst all bigrams.
* **References** - A space-delimited list of indices in the corpus that correspond to a occurrence of this pattern. Indices are in the form ``sentence:token`` where sentence starts at one and token starts at zero. This column is only available for indexed models.
 
Creating a pattern model with skipgrams and/or flexgrams
----------------------------------------------------------

The pattern model created in the previous example did not yet include skip-grams, these have to be explicitly enabled with the ``-s`` flag. When this is used, another options becomes available for consideration:

* ``-T [value]`` - Only skipgrams that have at least this many different types
  as skip content, i.e. possible options filling the gaps, will be considered.
  The default is set to two.
  
Here is an example of generating an indexed pattern model including skipgrams::

	$ colibri-patternmodeller -f yourcorpus.colibri.dat -t 10 -s -T 3 -o yourcorpus.colibri.indexedpatternmodel

If you want to generate unindexed models, simply add the flag ``-u``. Do note
that for unindexed models the parameter ``-T`` has no effect, it will extract
all skipgrams it can find as if ``-T`` were set to one! If you want decent
skpigrams, you're best off with an indexed model. Note that indexed models can
always be read and printed in an unindexed way (with the ``-u`` flag); but
unindexed models can not be read in an indexed way, as they simply lack
indices::

	$ colibri-patternmodeller -i yourcorpus.colibri.indexedpatternmodel -c yourcorpus.colibri.cls -u -P
	$ colibri-patternmodeller -i yourcorpus.colibri.unindexedpatternmodel -c yourcorpus.colibri.cls -u -P

Flexgrams, non-consecutive patterns in which the gap (only one in the current implementation) is of dynamic width, can be generated in one of two ways:

 * Extract flexgrams by abstracting from skipgrams: use the ``-S S`` flag.
 * Extract flexgrams directly from n-gram co-occurence: use the ``-S
   [threshold]`` flag, where the threshold is expressed as normalised pointwise mutual
   information [-1,1]. 

The skipgram approach has the advantage of allowing you to rely on the ``-T``
threshold, but comes with the disadvantage of having a maximum span. The
co-occurrence approach allows for flexgrams over larger distances. Both methods
come at the cost of more memory, especially the former method.

Neither skipgrams nor flexgrams will cross the line boundary of the original
corpus data, so ensure your data is segmented into lines suitable for your
purposes in the encoding stage.

Two-stage building
-----------------------

Generating an indexed pattern model takes considerably more memory than an
unindexed model, as instead of mere counts, all indices have to be retained. 

The creation of a pattern models progresses through stages of counting and
pruning. When construction of an indexed model with an occurrence threshold of
2 or higher reaches the limits of your system's memory capacity, then two-stage building
*may* offer a solution. 

Two-stage building first constructs an unindexed model (demanding less memory),
and subsequently loads this model and searches the corpus for indices for all
patterns in the model. Whilst this method is more time-consuming, it prevents
the memory bump (after counting, prior to pruning) that normal one-stage
building of indexed models have. Two-stage building is enabled using the ``-2`` flag::

	$ colibri-patternmodeller -2 -f yourcorpus.colibri.dat -t 10 -s -T 3 -o yourcorpus.colibri.indexedpatternmodel


Statistical reports and histograms
----------------------------------

If you have a pattern model, you can generate a statistical report which includes information on the number of occurrences and number of types for patterns, grouped for n-grams or skipgrams for a specific value of *n*. A report is generated using the ``-R`` flag, the input model is specified using ``-i``::

	   $ colibri-patternmodeller -i yourcorpus.colibri.indexedpatternmodel -R

Example output::

 REPORT
 ----------------------------------
                             PATTERNS    TOKENS  COVERAGE     TYPES
 Total:                             -       340         -       177
 Uncovered:                         -       175    0.5147       136
 Covered:                          69       165    0.4853        41
 
   CATEGORY N (SIZE)   PATTERNS    TOKENS  COVERAGE     TYPES OCCURRENCES
        all       all        69       165    0.4853        41         243
        all         1        40       165    0.4853        40         165
        all         2        11        26    0.0765        13          26
        all         3         7        17    0.0500         9          19
        all         4         5        10    0.0294         9          14
        all         5         5         9    0.0265         9          17
        all         6         1         2    0.0059         6           2
     n-gram       all        62       165    0.4853        40         215
     n-gram         1        40       165    0.4853        40         165
     n-gram         2        11        26    0.0765        13          26
     n-gram         3         5        12    0.0353         8          12
     n-gram         4         3         6    0.0176         6           6
     n-gram         5         2         4    0.0118         6           4
     n-gram         6         1         2    0.0059         6           2
   skipgram       all         7         7    0.0206         6          28
   skipgram         3         2         7    0.0206         4           7
   skipgram         4         2         4    0.0118         4           8
   skipgram         5         3         5    0.0147         5          13

Some explanation is in order to correctly interpret this data.  First of all
patterns are grouped by category (ngram,skipgram, flexgram) and size. There are various metrics:

    * **Pattern** - The number of distinct patterns in this group, so for
      category n-gram of 2, this reflects the number of distinct bigrams.
    * **Tokens** - The number of tokens that is covered by the patterns in the
      group. Longer patterns by definition cover more tokens.  
      . This is only available for indexed models, for unindexed models it is either omitted or the
      number shown is maximum projection ``occurrencecount * size`` .
    * **Coverage** - The number of tokens covered as a fraction of the total number of tokens. Only for indexed models.
    * **Types** - The number of unique **word** types covered, i.e the number
      of distinct unigrams.
    * **Occurrences** - Cumulative occurrence count of all the patterns in
      the group. Used as a basis for computing frequency. Occurrence count
      differs from **tokens**, the former expresses the number of times a
      pattern occurs in the corpus, the latter expresses how many tokens are
      part of the pattern
      

To better understand these metrics, let's perceive them in the following test
sentence::

    to be or not to be , that is the question

If we generate an indexed pattern model purely on this sentence, **with threshold two**. We find the following three patterns::

    PATTERN COUNT   TOKENS  COVERAGE        CATEGORY        SIZE    FREQUENCY REFERENCES
    to      2       2       0.181818        ngram            1       0.5     1:0 1:4
    be      2       2       0.181818        ngram            1       0.5     1:1 1:5
    to be   2       4       0.363636        ngram            2       1       1:0 1:4

The report then looks as follows::

    REPORT
    ----------------------------------
                                PATTERNS    TOKENS  COVERAGE     TYPES
    Total:                             -        11         -         9
    Uncovered:                         -         7    0.6364         7
    Covered:                           3         4    0.3636         2

    CATEGORY N (SIZE)   PATTERNS    TOKENS  COVERAGE     TYPES OCCURRENCES
        all       all         3         4    0.3636         2           6
        all         1         2         4    0.3636         2           4
        all         2         1         4    0.3636         2           2
     n-gram       all         3         4    0.3636         2           6
     n-gram         1         2         4    0.3636         2           4
     n-gram         2         1         4    0.3636         2           2


Our sentence has 11 tokens, 7 of which are not covered by the patterns found, 4
of which are.  Since we have only n-grams and no skipgrams or flexgrams in this
simple example, the data for *all* and *n-gram* is the same. The **coverage**
metric expresses this in a normalised fashion. 

In our data we have two unigrams *(to, be)* and one bigram *(to be)*, this is
expressed by the **patterns** metric. Both the unigrams and the bigrams cover
the exact same four tokens in our sentence, i.e  0, 1, 4, and 5, so the TOKENS
column reports four for all. If we look at the **types** column, we notice we
only have two word types: *to* and *be*. The unigrams occur in four different
instances and the bigrams occur in two different instances. This is expressed
in the **occurrences** column. Combined that makes six occurrences.

We have 9 types in total, of which only 2 (to, be) are covered, the remaining 7
*(or not , that is the question)* remain uncovered as we set our occurrence threshold for
this model to two. 

Pattern models store how many of the tokens and types in the original corpus
were covered. Tokens and types not covered did not make the set thresholds.
Make sure to use indexed models if you want accurate coverage data.

A histogram can also be generated, using the ``-H`` flag::
	   
        $ colibri-patternmodeller -i yourcorpus.colibri.indexedpatternmodel -H

Example output::

        OCCURRENCES   PATTERNS
        2   39
        3   5
        4   13
        5   5
        6   1
        7   1
        8   1
        10  1
        13  1
        14  1
        15  1


Filtering models
--------------------------------

Patterns models can be read with ``-i`` and filtered by setting stricter thresholds prior to printing, reporting or outputting to file. An example::

    $ colibri-patternmodeller -i yourcorpus.colibri.indexedpatternmodel -t 20 -T 10 -o yourcorpus_filtered.colibri.indexedpatternmodel -P

You can also filter pattern models by intersecting with another pattern model
using the ``-j`` option. This only works when both are built on
the same class file::

    $ colibri-patternmodeller -i yourcorpus.colibri.indexedpatternmodel -j yourcorpus2.colibri.indexedpatternmodel -o yourcorpus_filtered.colibri.indexedpatternmodel

The output pattern model will contain only those patterns that were present in
both the input model (``-i``) as well as the constraining model (``-j``), which may be
either indexed or unindexed regardless of the input model; it will
always contain the counts/indices from the input model.


Training and testing coverage
--------------------------------

An important quality of pattern models lies in the fact that pattern models can
be compared, provided they use comparable vocabulary, i.e. are based on the same
class file.  More specifically, you can train a pattern model on a corpus and
test it on another corpus, which yields another pattern model containing only
those patterns that occur in both training and test data. The difference in
count, frequency and coverage can then be easily be compared. You build such a
model by taking the intersection with a training model using the ``-j`` flag.
Make sure to always use the same class file for all datasets you are comparing.
Instructions for this were given in :ref:`classencodetraintest`.

Training::
   $ colibri-patternmodeller -f trainset.colibri.dat -o trainset.colibri.indexedpatternmodel

This results in a model ``trainset.colibri.indexedpatternmodel``. Now proceed
with testing on another corpus:

Testing::
   $ colibri-patternmodeller -f testset.colibri.dat -j trainset.indexedpatternmodel.colibri -o testset.colibri.indexedpatternmodel

or (more memory efficient)::

   $ colibri-patternmodeller -f testset.colibri.dat -I trainset.indexedpatternmodel.colibri -o testset.colibri.indexedpatternmodel

This results in a model ``testset.colibri.indexedpatternmodel`` that only contains patterns that also occur in the specified training model. 

Such an intersection of models can also be created at any later stage using
``-i`` and ``-j``, as shown in the previous section.


Reverse index
-----------------

Indexed pattern models have a what is called a *forward index*. For each
pattern, all of the positions, ``(sentence,token)``, at which a token of the
pattern can be found, is held. In Colibri-core, sentences always start at 1, whereas
tokens start at 0. 

A reverse index is a mapping of references of the type ``(sentence,token)`` to a set of all the
patterns that *begin* at that location. Such a reverse index can be constructed
from the forward index of an indexed pattern model, or it can be explicitly
given by simply passing the original corpus data to the model, which makes
reverse indices available even for unindexed models. Explicitly providing a
reverse index makes loading a model faster (especially on larger models), but at
the cost of higher memory usage, especially in case of sparse models.

Passing corpus data for the reverse index to colibri-patternmodeller is done
using the ``-r`` flag, and the full reverse index can be displayed using the ``-Z`` flag::

   $ colibri-patternmodeller  -i yourcorpus.indexedpatternmodel.colibri -r yourcorpus.colibri.dat -c yourcorpus.colibri.cls -Z


Indexes and/or reverse indexes are required for various purposes, one of which
is the extraction of relations and co-occurrence information.


Query mode
--------------

The pattern modeller has query mode which allows you to quickly extract patterns from test sentences or fragments thereof. The query mode is invoked by loading a pattern model (``-i``), a class file (``-c``) and the ``-Q`` flag. The query mode can be run interactively as it takes input from ``stdin``, one *tokenised* sentence per line. The following example illustrates this, the sentence *"To be or not to be"* was typed as input::


    $ colibri-patternmodeller -i /tmp/data.colibri.patternmodel -c /tmp/hamlet.colibri.cls -Q
    Loading class decoder from file /tmp/hamlet.colibri.cls
    Loading class encoder from file /tmp/hamlet.colibri.cls
    Loading indexed pattern model /tmp/data.colibri.patternmodel as input model...
    Colibri Patternmodeller -- Interactive query mode.
    Type ctrl-D to quit, type X to switch between exact mode and extensive mode (default: extensive mode).
    1>> To be or not to be
    1:0	To	8		8	0.0235294	ngram	1	0.0484848   1:0 5:7 9:5 10:0 22:0 36:0 37:0 38:0
    1:1	be	4		4	0.0117647	ngram	1	0.0242424   1:1 1:5 9:2 35:3
    1:2	or	4		4	0.0117647	ngram	1	0.0242424   1:2 36:2 37:2 38:2
    1:3	not	5		5	0.0147059	ngram	1	0.030303    1:3 27:7 36:3 37:3 38:3
    1:4	to	13		13	0.0382353	ngram	1	0.0787879   1:4 2:6 4:1 5:10 6:7 8:4 9:1 9:8 10:4 27:2 36:4 37:4 38:4
    1:2	or not	4		8	0.0235294	ngram	2	0.153846    1:2 36:2 37:2 38:2
    1:3	not to	4		8	0.0235294	ngram	2	0.153846    1:3 36:3 37:3 38:3
    1:2	or not to	4		12	0.0352941	ngram	3	0.333333    1:2 36:2 37:2 38:2

The output starts with an index in the format ``sentence:token``, specifying
where the pattern found was found in your input. The next columns are the same
as the print output.The interactive query mode distinguishes two modes,
extensive mode and exact mode. In extensive mode, your input string will be
scanned for all patterns occurring in it. In exact mode, the input you
specified needs to match exactly and as a whole. Type ``X`` to switch between
the modes.

In addition to interactive query mode, there is also a command line query mode
``-q`` in which you specify the pattern you want to query as argument on the command
line. Multiple patterns can be specified by repeating the ``-q`` flag. This
mode always behaves according to exact mode::

    $ colibri-patternmodeller -i /tmp/data.colibri.patternmodel -c /tmp/hamlet.colibri.cls -q "to be"
    Loading class decoder from file /tmp/hamlet.colibri.cls
    Loading class encoder from file /tmp/hamlet.colibri.cls
    Loading indexed pattern model /tmp/data.colibri.patternmodel as input model...
    to be	2		4	0.0117647	ngram	2	0.0769231   1:4 9:1


Pattern Relations
---------------------

A pattern model contains a wide variety of patterns; the relationships between those can be made explicit. These relationships can be imagined as a directed graph, in which the nodes represent the various patterns (n-grams and skipgrams), and the edges represent the relations. The following relations are distinguished; note that as the graph is directed relations often come in pairs; one relationship for each direction: 

* **Subsumption relations** - Patterns that are subsumed by larger patterns are called *subsumption children*, the larger patterns are called *subsumption parents*. These are the two subsumption relations that can be extracted from an indexed pattern model.
* **Successor relations**  - Patterns that follow eachother are in a left-of/right-of relation.
* **Instantiation relations** - There is a relation between skipgrams and
  patterns that instantiate them ``to be {*1*} not {*1*} be`` is instantiated
  by ``to {*1*} or``, also referred to as the skip content.

You can all of these extract relations using the ``-g`` flag, which is to be
used in combination with the query mode ``-Q`` or ``-q``. Consider the
following sample::

    $ colibri-patternmodeller -i /tmp/data.colibri.patternmodel -c /tmp/hamlet.colibri.cls -q "to be" -g  
    Loading class decoder from file /tmp/hamlet.colibri.cls
    Loading class encoder from file /tmp/hamlet.colibri.cls
    Loading indexed pattern model /tmp/data.colibri.patternmodel as input model...
    Post-read processing (indexedmodel)
    to be	2		4	0.0117647	ngram	2	0.0769231	1:4 9:1
    #	PATTERN1	RELATION	PATTERN2	REL.COUNT	REL.FREQUENCY	COUNT2
        to be	SUBSUMES	to	2	0.5	13
        to be	SUBSUMES	be	2	0.5	4
        to be	RIGHT-NEIGHBOUR-OF	To {*1*} or not	1	0.25	4
        to be	RIGHT-NEIGHBOUR-OF	To {*2*} not	1	0.25	4
        to be	RIGHT-NEIGHBOUR-OF	not	1	0.25	5
        to be	RIGHT-NEIGHBOUR-OF	or not	1	0.25	4
        
The following columns are reported, all are indented with a single tab so
possible parsers can distinguish the numbers for the queried pattern itself from the relationships with other patterns.

    * **Pattern 1** -- The pattern you queried
    * **Relation** -- The nature of the relationship between pattern 1 and pattern 2
    * **Pattern 2** -- The pattern that is related to the queried pattern
    * **Relation Count** -- The number of times pattern 1 and pattern 2 occur in this relation 
    * **Relation Frequency** -- The number of times pattern 1 and pattern 2 occur in this relationas a fraction of all relations of this type
    * **Count 2** -- The absolute number of occurrences of pattern 2 in the model

Co-occurrence 
--------------------

Co-occurrence in Colibri-Core measures which patterns co-occur on the same line
(i.e. usually corresponding to a sentence or whatever structural unit you
decided upon when encoding your corpus). Co-occurrence is another relation in
addition to the ones described in the previous section.

The degree of co-occurrence can be expressed as either an absolute occurrence
number (``-C``), for a normalised mutual pointwise information (``-Y``). Both
flags take a threshold, setting the threshold too low, specially for npmi, may
cause very high memory usage. The following syntax would show all patterns that
occur at least five times in the same sentence. Note that the order of the
pattern pairs does not matter; if there are two patterns X and Y then result X
Y or Y X would be the same, yet only one of them is included in the output to
prevent duplicating information::

  $ colibri-patternmodeller -i /tmp/data.colibri.patternmodel -c /tmp/hamlet.colibri.cls -C 5 




Architecture Overview
=======================

.. image:: arch.png


Python Tutorial
=======================

Colibri Core offers both a C++ API as well as a Python API. It exposes all of
the functionality, and beyond, of the tools outlined above. The Python API
binds with the C++ code, and although it is more limited than the C++ API, it
still offers most higher-level functionality. The Colibri Core binding between C++ and
Python is written in Cython.

A Python tutorial for Colibri Core is available in the form of an IPython
Notebook, meaning that you can interactively run it and play with. You can go
to the static, read-only, version `by clicking here <http://proycon.github.io/colibri-core/doc/colibricore-python-tutorial.html>`_ 


Python API Reference
=======================



.. automodule:: colibricore
   :members:
   :undoc-members:
   :special-members:
   :show-inheritance:



















