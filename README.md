# Colibri Core

[![GitHub C++ build](https://github.com/proycon/colibri-core/actions/workflows/colibri-core.yml/badge.svg?branch=master)](https://github.com/proycon/colibri-core/actions/)
[![GitHub Python build](https://github.com/proycon/colibri-core/actions/workflows/colibri-core-python.yml/badge.svg?branch=master)](https://github.com/proycon/colibri-core/actions/)
[![DOI](https://zenodo.org/badge/12996232.svg)](https://zenodo.org/badge/latestdoi/12996232)
[![GitHub release](https://img.shields.io/github/release/proycon/colibri-core.svg)](https://GitHub.com/proycon/colibri-core/releases/)
[![Project Status: Active – The project has reached a stable, usable state and is being actively developed.](https://www.repostatus.org/badges/latest/active.svg)](https://www.repostatus.org/#active)
[![PyPi](https://badge.fury.io/py/colibricore.svg)](https://pypi.org/colibricore)

*by Maarten van Gompel, proycon@anaproy.nl, Radboud University Nijmegen*

*Licensed under GPLv3 (See http://www.gnu.org/licenses/gpl-3.0.html)*

Colibri Core is software to quickly and efficiently count and extract patterns
from large corpus data, to extract various statistics on the extracted
patterns, and to compute relations between the extracted patterns. The employed
notion of pattern or construction encompasses the following categories:

* **n-gram** -- *n* consecutive words
* **skipgram** -- An abstract pattern of predetermined length with one or multiple gaps (of specific size).
* **flexgram** -- An abstract pattern with one or more gaps of variable-size.

N-gram extraction may seem fairly trivial at first, with a few lines in your
favourite scripting language, you can move a simple sliding window of size **n**
over your corpus and store the results in some kind of hashmap. This trivial
approach however makes an unnecessarily high demand on memory resources, this
often becomes prohibitive if unleashed on large corpora. Colibri Core tries to
minimise these space requirements in several ways:

* **Compressed binary representation** -- Each word type is assigned a numeric class, which is encoded in a compact binary format in which highly frequent classes take less space than less frequent classes. Colibri core always uses this representation rather than a full string representation, both on disk and in memory.
* **Informed iterative counting** -- Counting is performed more intelligently by iteratively processing the corpus in several passes and quickly discarding patterns that won't reach the desired occurrence threshold.

Skipgram and flexgram extraction are computationally more demanding but have
been implemented with similar optimisations. Skipgrams are computed by
abstracting over n-grams, and flexgrams in turn are computed either by
abstracting over skipgrams, or directly from n-grams on the basis of
co-occurrence information (mutual pointwise information).

At the heart of the sofware is the notion of pattern models. The core tool, to
be used from the command-line, is ``colibri-patternmodeller`` which enables you
to build pattern models, generate statistical reports, query for specific
patterns and relations, and manipulate models.

A pattern model is simply a collection of extracted patterns (any of the three
categories) and their counts from a specific corpus. Pattern models come in two
varieties:

* **Unindexed Pattern Model** -- The simplest form, which simply stores the patterns and their count.
* **Indexed Pattern Model** -- The more informed form, which retains all indices to the original corpus, at the cost of more memory/diskspace.

The Indexed Pattern Model is much more powerful, and allows more statistics and
relations to be inferred.

The generation of pattern models is optionally parametrised by a minimum
occurrence threshold, a maximum pattern length, and a lower-boundary on the
different types that may instantiate a skipgram (i.e. possible fillings of the
gaps).

## Technical Details

Colibri Core is available as a collection of **standalone command-line tools**,
as a **C++ library**, and as a **Python library**.

Please consult the full documentation at <https://proycon.github.io/colibri-core>

## Installation

### Python binding

For the Colibri Core Python library, just install using:

```
pip install colibricore
```

We strongly recommend you use a Virtual Environment for this. Do note that this
is only available for unix-like systems, Windows is not supported.

### Installation from source

For the command-line tools, check if your distribution has a package available.
There are packages for Alpine Linux (`apk add colibri-core`) and for macOS with
homebrew (`brew tap fbkarsdorp/homebrew-lamachine && brew install
colibri-core`). Note that these do not contain the Python binding!

If no packages are available, you will need to compile from source or use the container build (e.g.
Docker) as explained later on.

In order to do so, you need a sane build environment, install the necessary dependencies for your distribution:

For Debian/Ubuntu::

```
$ sudo apt-get install make gcc g++ pkg-config autoconf-archive libtool autotools-dev libbz2-dev zlib1g-dev libtar-dev python3 python3-dev cython3
```

For RedHat-based systems (run as root)::

```
# yum install pkgconfig libtool autoconf automake autoconf-archive make gcc gcc-c++ libtar libtar-devel python3 python3-devel zlib zlib-devel python3-pip bzip2 bzip2-devel cython3
```

For macOS with homebrew:

```
$ brew install autoconf automake libtool autoconf-archive python3 pkg-config
```

Then clone this repository and install as follows:

```
$ bash bootstrap
$ ./configure
$ make
$ sudo make install
```

### Container usage

The Colibri Core command-line tools are also available as an OCI/Docker container.

A pre-made container image can be obtained from Docker Hub as follows:

``docker pull proycon/colibri-core``

You can also build a container image yourself as follows, make sure you are in the root of this repository:

``docker build -t proycon/colibri-core .``

This builds the latest stable release, if you want to use the latest development version
from the git repository instead, do:

``docker build -t proycon/colibri-core --build-arg VERSION=development .``

Run the frog container interactively as follows, it will dump you into a shell where the various command line tools are available:

``docker run -t -i proycon/colibri-core``

Add the ``-v /path/to/your/data:/data`` parameter if you want to mount your data volume into the container at `/data`.

## Demo

![Colibri Core Demo](https://raw.githubusercontent.com/CLARIAH/wp3-demos/master/colibri-core.gif)


## Publication

This software is extensively described in the following peer-reviewed publication:

    van Gompel, M and van den Bosch, A (2016)
    Efficient n-gram, Skipgram and Flexgram Modelling with Colibri Core.
    *Journal of Open Research Software*
    4: e30, DOI: http://dx.doi.org/10.5334/jors.105

Access the publication [here](http://dx.doi.org/10.5334/jors.105) and please cite it if you make use of
Colibri Core in your work.
