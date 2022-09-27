# BuildIt Regex

- To compile the code run `make` in the root directory.
- To generate the code from `./samples/sample1.cpp` run `./build/sample1`.

# Setting up the benchmarks

## Hyperscan

- To build Hyperscan follow the steps 2 and 3 from [here](https://intel.github.io/hyperscan/dev-reference/getting_started.html).
- Use one of the scripts in `./benchmarks/hyperscan/tools/hsbench/scripts` to create a corpus SQLite database.
- Add the regex patterns to a file following [this format](http://intel.github.io/hyperscan/dev-reference/tools.html#tools-pattern-format).
- From the hyperscan build directory run `bin/hsbench -e <pattern_file> -c <corpus.db>`. More directions are available [here](http://intel.github.io/hyperscan/dev-reference/tools.html#running-hsbench).

## RE2

- To build RE2 run `make` in the `./benchmarks/re2/` directory.
- To run the timing experiments on the Twain dataset run `bash run.sh` in the `./benchmarks` directory.

# Datasets

## Twain
- Corpus: [Project Gutenberg: Complete Works of Mark Twain](https://www.gutenberg.org/files/3200/)
- Patterns: available in `./benchmarks/data/twain_patterns.txt`; taken from [this paper](https://www.microsoft.com/en-us/research/uploads/prod/2019/02/SRM_tacas19.pdf)

## Snort
