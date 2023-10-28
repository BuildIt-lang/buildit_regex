# import the relevant file from buildit
# trigger compilation of buildit and retrieve data
# do ml things??
import torch  # for ml things
import subprocess  # for launching buildit
import sqlite3  # for parsing the .db files for example inputs


def launch(regex_string, input, regex_options, match_type, submatch):
    command = [
        "./src/ml.cpp",
        regex_string,
        input,
        str(regex_options),
        match_type,
        submatch,
    ]

    try:
        result = subprocess.run(
            command,
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


def main():
    make_command = "make -f Makefile"
    try:
        subprocess.run(make_command, shell=True, check=True)
    except subprocess.CalledProcessError:
        print("Makefile compilation failed!")

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
    regex_options = {
        "ignore_case": False,
        "dotall": False,
        "interleaving_parts": 1,
        "flags": "",
        "greedy": False,
        "binary": True,
        "block_size": -1,
    }
    launch("sample", "sample", regex_options, "1", "submatch_sample")


if __name__ == "__main__":
    main()
