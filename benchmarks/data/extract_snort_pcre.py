
def extract_patterns(lines, k, out_file):
    for line in lines:
        if k not in line:
            continue
        parts = line.split(k)
        regex = parts[1].split(';')[0][1:-1]
        out_file.write(regex + '\n')

if __name__ == '__main__':
    fname = 'snort3-community-rules/snort3-community.rules'
    with open (fname, 'r') as f:
        extract_patterns(f.readlines(), 'pcre:', open('snort_patterns.txt', 'w'))
