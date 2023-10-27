# import the relevant file from buildit
# trigger compilation of buildit and retrieve data
# do ml things??
import torch  # for ml things
import subprocess  # for launching buildit
import sqlite3  # for parsing the .db files for example inputs

make_command = "make -f Makefile"
try:
    subprocess.run(make_command, shell=True, check=True)
except subprocess.CalledProcessError:
    print("Makefile compilation failed!")

try:
    result = subprocess.run(
        "./build/sample1",
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        shell=True,
        check=True,
    )
    output = result.stdout
    error_output = result.stderr
except subprocess.CalledProcessError:
    print("Unable to run C++ program!")

# struct RegexOptions {
#     bool ignore_case = false; // same as the PCRE i flag
#     bool dotall = false; // same as the PCRE s flag
#     int interleaving_parts = 1;
#     string flags = "";
#     bool greedy = false;
#     bool binary = true;
#     int block_size = -1; // split the string into blocks
# };
# for the C++ struct, we have to pass in all the attributes then initialize in our main() function
command = [
    "./src/compile.cpp",
    "your_regex_here",
    "your_input_string_here",
    "your_options_here",
]
try:
    result = subprocess.run(
        "./src/match.cpp",  # call match. as seen in Tamara's thesis, match compiles generated code
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        shell=True,
        check=True,
    )
    output = result.stdout
    error_output = result.stderr
except subprocess.CalledProcessError:
    print("Unable to run C++ program!")
