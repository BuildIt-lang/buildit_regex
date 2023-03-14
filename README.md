# BuildIt RegEx

This repo contains an implementation of a regular expression library using [BuildIt](https://buildit.so/).

We currently support the following types of matches:
- full match that checks if the regex exactly matches the text; example code is given in `./samples/sample1.cpp`
- partial match with binary output that tells if there is a match or not (`./samples/sample2.cpp`)
- all partial matches returned as a list of strings (`./samples/sample3.cpp`); the output of all partial matches
is the same as the output of reapeatedly applying the PCRE or RE2 FindAndConsume function that gives non-overlapping
leftmost longest matches

We support the following operators and expressions.

| Expresssion                  | Description                                      |
|------------------------------|--------------------------------------------------|
| `.`                          | any character                                    |
| `[xyz]`, `[^xyz]`            | character class                                  |
| `[a-z]`, `[^a-z]`            | character range                                  |
| `x?`                         | zero or one `x`                                  |
| `x+`                         | one or more `x`                                  |
| `x*`                         | zero or more `x`                                 |
| `(x\|y)`                     | `x` or `y`                                       |
| `x{n}`                       | `x` repeated `n` times                           |
| `x{n,m}`                     | `x` repeated between `n` and `m` times inclusive |
| `\d`, `\w`, `\s`, `\D`, `\W` | escaped character classes                        |

We have a couple of flag options that affect the way the code is generated:
- specifying the number of interleaving parts for partial matches
- splitting the code generation on `|` characters
- grouping multiple consecutive states into one
These options can be set using the `RegexOptions` struct as shown in `./samples/sample2.cpp`.

To compile the code run `make` from the root directory. To run the sample1 code for example, run `./build/sample1`.

### Code structure

- The main code is in `./src` and `./include`.
- Testing code is in `./test`.
- Code for measuring performance is in `./benchmarks`.

## Setting up the benchmarks

### Hyperscan

- To build Hyperscan follow the steps 2 and 3 from [here](https://intel.github.io/hyperscan/dev-reference/getting_started.html).
- Use one of the scripts in `./benchmarks/hyperscan/tools/hsbench/scripts` to create a corpus SQLite database.
- Add the regex patterns to a file following [this format](http://intel.github.io/hyperscan/dev-reference/tools.html#tools-pattern-format).
- From the hyperscan build directory run `bin/hsbench -e <pattern_file> -c <corpus.db>`. More directions are available [here](http://intel.github.io/hyperscan/dev-reference/tools.html#running-hsbench).

### RE2

- To build RE2 run `make` in the `./benchmarks/re2/` directory.

To run the timing experiments on the Twain dataset run `./build/preformance` in the `./benchmarks` directory.

## Datasets

### Twain
- Corpus: [Project Gutenberg: Complete Works of Mark Twain](https://www.gutenberg.org/files/3200/)
- Patterns: available in `./benchmarks/data/twain_patterns.txt`; taken from [this paper](https://www.microsoft.com/en-us/research/uploads/prod/2019/02/SRM_tacas19.pdf)
