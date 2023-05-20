# About

An exploration into bounded TED as a dynamic graph algorithm.
This repo contains basic dynamic version of Touzet's TED algorithm, and a benchmarking script.
The project requires a copy of the tree similarity repo (https://github.com/DatabaseGroup/tree-similarity/) for reference implementations of TopDiff and Touzet's TED algorithm.
It is included as a git submodule, and you will need to make sure that it has been cloned properly (or otherwise provided at `./external/tree-similarity`).

# Licencing

Source files in this repo contain their own licence information.
If they have any substantial quantity of altered or reused code, the licence information reflects this appropriately.
Any source files not containing licence information should be considered public domain.

The licence information for the reference implementations is available alongside the implementations themselves, wherever you source them from.

# Use

The executable can be built with `make`, and replication benchmarks can be run with `bench.py`.
`bench.py` should be run from inside a copy of the linux git repo (this is its data source) and provided with:
 * the path to the built executable (`./bin/ted` by default)
 * a scratch directory (for changeset caching) (currently unimplemented)
 * an output directory

Running a full replication may take a few days and use up to ~50GB memory.
