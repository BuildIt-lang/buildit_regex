#include "test.h"

void test_simple(MatchType type) {
    compare_result("abcdef", "abcdef", "......", type);
    compare_result("abcdef", "abcdef", ".ggg..", type);
    compare_result("abcdef", "abcdef", "gg....", type);
    compare_result("abcdef", "abcdef", "...ggg", type);
    compare_result("abcdef", "11abcdef111", ".ggg..", type);
    compare_result("abcdef", "xyzabcdefgh", "gg....", type);
    compare_result("abcdef", "xyzabcdefghi", "...ggg", type);
}

void test_star(MatchType type) {
    compare_result("a*bc", "aaaabc", "gg..", type);
    compare_result("a*bc", "aaaabc", "ggg.", type);
    compare_result("a*bc", "dddaaaabc", "gg..", type);
    compare_result("a*bc", "ccccaaaabcd", "ggg.", type);
}

void test_brackets(MatchType type) {
    compare_result("a[abc]d", "abd", ".gggggg", type);
    compare_result("[a-d]ef", "bef", "ggggg..", type);
    compare_result("[^a-d]ef", "bef", "gggggg..", type);
    compare_result("[^a-d]ef", "kef", "gggggg..", type);
    compare_result("a[abc]d", "555abd1", ".gggggg", type);
    compare_result("[a-d]ef", "44bef3", "ggggg..", type);
    compare_result("[^a-d]ef", "33bef65", "gggggg..", type);
    compare_result("[^a-d]ef", "421kef43", "gggggg..", type);
}

void test_repetition(MatchType type) {
    compare_result("abaaacde", "abaaacde", "ggggggg.", type);
    compare_result("aba{3}cde", "abaaacde", "gggggggg.", type);
    compare_result("aba{2,4}cde", "abaaacde", "gggggggggg.", type);
    compare_result("aba{2,4}cde", "abaaacde", "gggggggg...", type);
    compare_result("aba{2,4}cde", "abaaacde", "..ggggggg..", type);
    compare_result("aba{2,4}cde", "abaaacde", "..ggggggg..", type);
    compare_result("aba{2,4}cde", "abaaacde", "..ggggggggg", type);
    compare_result("aba{2,4}cde", "dsafdabaaacde", "gggggggggg.", type);
    compare_result("ab.{2,4}cde", "abaaacde", "gggggggggg.", type);
    compare_result("ab.{2,4}cde", "abaaacdefdsa", "gggggggggg.", type);
    compare_result("aba{2,4}cde", "fdasfabaaacdegf", "gggggggggg.", type);
    compare_result("aba{2,4}cde", "asfdsabaaacdegf", "gggggggg...", type);
    compare_result("aba{2,4}cde", "fasdabaaacdefds", "..gggggg...", type);
    compare_result("aba{2,4}cde", "asdabaaacdefd", "..ggggggg..", type);
    compare_result("aba{2,4}cde", "abaaacde", "..ggggggggg", type);
}

void test_or_groups(MatchType type) {
    compare_result("(ab|cd)ef", "abef", "ggggggg..", type);
    compare_result("(ab|cd)ef", "cdef", "ggggggg..", type);
    compare_result("(ab|cd)ef", "ef", "ggggggg..", type);
    compare_result("111(ab|cd)ef", "111abef", "...ggggggg..", type);
    compare_result("111(ab|cd)ef", "111abef", "..gggggggg..", type);
    compare_result("111(ab|cd)ef", "aa111abef", "..gggggggg..", type);
    compare_result("(ab|cd)ef", "aaaabefaa", "ggggggg..", type);
    compare_result("(ab|cd)ef", "aacdef55", "ggggggg..", type);
    compare_result("(ab|cd)ef", "555efa", "ggggggg..", type);
    compare_result("111(ab|cd)ef", "111abefaa", "...ggggggg..", type);
}

void test_combined(MatchType type) {
    // brackets and star
    compare_result("[bcd]*", "bbcdd", "gggggg", type);    
    compare_result("aa5[bcd]*", "aa5bbcdd", "..ggggggg", type);    
    compare_result("[bcd]5*", "d555", "ggggggg", type);    
    compare_result("[bcd]5*", "bbcdd555", "ggggggg", type);    
    compare_result("[bcd]*", "aaaabbcdd", "gggggg", type);    
    compare_result("aa5[bcd]*", "113aa5bbcdd11", "..ggggggg", type);    
    compare_result("a(abcd)*", "aabcdabcd", ".ggggggg", type);
    compare_result("(abcd)*", "abcdabcd", "ggggggg", type);
    compare_result("12(abcd)*", "12abcdabcd", "ggggggggg", type);
    compare_result("12(abcd)*", "55512abcdabcd", "ggggggggg", type);
    compare_result("(bdc)*", "bdcbdc", "ggggg.", type);
    // ? and +
    compare_result("ab(cd)?", "abcd", "..gggg.", type); 
    compare_result("ab(cd)?", "ab", "..gggg.", type); 
    compare_result("ab(cd)?", "abcd", ".gggggg", type); 
    compare_result("ab(cd)?", "ab", ".gggggg", type); 
    compare_result("ab(cd)+", "abcdcd", "..gggg.", type); 
    compare_result("ab(cd)+", "abcd", "..gggg.", type); 
    compare_result("ab(cd)?", "iafdsabcd", "..gggg.", type); 
    compare_result("ab(cd)?", "111abppp", "..gggg.", type); 
    compare_result("ab(cd)?", "123abcd123", ".gggggg", type); 
    compare_result("ab(cd)?", "a22ab22", ".gggggg", type); 
    compare_result("ab(cd)+", "21abcdcd21", "..gggg.", type); 
    compare_result("ab(cd)+", "333abcd", "..gggg.", type); 
}

void test_or_split(MatchType type) {
    // full
    compare_result("ab(c|56|de)", "ab56", "...s.s..s..", type);
    compare_result("ab(c|56|de)+", "ab56ab56", "...s.s..s...", type);
    compare_result("ab(c|56|de)k", "abck", "...s.s..s...", type);
    compare_result("ab(c|56|de)k", "ab56k", "...s.s..s...", type);
    compare_result("ab(c|56|de)k", "abdek", "...s.s..s...", type);
    compare_result("ab(c|56|de)k", "ab56dek", "...s.s..s...", type);
    compare_result("ab((c|56|de)k|ab)", "ab56abab", "....s.s..s....s..", type);
    compare_result("a(cbd|45)*", "acbd45cbd", "..s...s...", type);
    compare_result("([abc]|dc|4)2", "b2", ".sssss.s..s..", type);
    compare_result("([abc]|dc|4)2", "b2", ".sssss.s..s..", type);
    compare_result("(dc|[abc]|4)2", "b2", ".s..sssss.s..", type);
    compare_result("(dc|4|[abc])2", "b2", ".s..s.sssss..", type);
    compare_result("([^23]|2)abc", "1abc", ".sssss.s....", type);
    compare_result("([^23]|2)abc", "3abc", ".sssss.s....", type);
    compare_result("(a|[abc]|4)2", "ac42", ".s.sssss.s..", type);
    compare_result("(b|a*|d)aa", "aaaa", ".s.s..s...", type);
    compare_result("(bc|de|[23]*)", "22323", ".s..s..ssss..", type);
    compare_result("(bc|de|[23]*)", "bc", ".s..s..ssss..", type);
    compare_result("(bc|de|[23]*)", "bcbc", ".s..s..ssss..", type);
    compare_result("(bc|de|[23]*)", "bc23", ".s..s..ssss..", type);
    compare_result("a(bc|de|[23]*)", "a", "..s..s..ssss..", type);
    compare_result("a(bc|de|[23]+)", "a", "..s..s..ssss..", type);
    compare_result("a(bc|de|[23]+)", "a2332", "..s..s..ssss..", type);
    compare_result("a(bc|de|[23]?)", "a23", "..s..s..ssss..", type);
    compare_result("(ab|(cd|ef)|dd)", "efff", ".....s..s......", type);
    compare_result("(ab|cd|ef)11(f|d|b)", "ab11d", ".s..s..s...........", type);
    compare_result("(ab|cd|ef)11(f|d|b)", "cd11f", ".............s.s.s.", type);
    compare_result("(ab|cd|ef)11(f|d|b)", "ef11b", ".s..s..s.....s.s.s.", type);
    // partial
    compare_result("ab(c|56|de)", "11ab5611", "...s.s..s..", type);
    compare_result("ab(c|56|de)+", "11ab56ab5611", "...s.s..s...", type);
    compare_result("ab(c|56|de)k", "aaabckdd", "...s.s..s...", type);
    compare_result("ab(c|56|de)k", "ssab56kdd", "...s.s..s...", type);
    compare_result("ab(c|56|de)k", "aaasabdekaa", "...s.s..s...", type);
    compare_result("ab(c|56|de)k", "22ab56dek222", "...s.s..s...", type);
    compare_result("ab((c|56|de)k|ab)", "111ab56abab222", "....s.s..s....s..", type);
    compare_result("a(cbd|45)*", "acbd45cbd222", "..s...s...", type);
    compare_result("([abc]|dc|4)2", "1b222", ".sssss.s..s..", type);
    compare_result("([abc]|dc|4)2", "3b324", ".sssss.s..s..", type);
    compare_result("(dc|[abc]|4)2", "ddb22", ".s..sssss.s..", type);
    compare_result("(dc|4|[abc])2", "11b2", ".s..s.sssss..", type);
    compare_result("([^23]|2)abc", "11abc44", ".sssss.s....", type);
    compare_result("([^23]|2)abc", "113abc33", ".sssss.s....", type);
    compare_result("(a|[abc]|4)2", "ddac42aa", ".s.sssss.s..", type);
    compare_result("(b|a*|d)aa", "1aaaadd", ".s.s..s...", type);
    compare_result("(bc|de|[23]*)", "dd22323aa", ".s..s..ssss..", type);
    compare_result("(bc|de|[23]*)", "ssbc", ".s..s..ssss..", type);
    compare_result("(bc|de|[23]*)", "aabcbc", ".s..s..ssss..", type);
    compare_result("(bc|de|[23]*)", "abac23a", ".s..s..ssss..", type);
    compare_result("a(bc|de|[23]*)", "a111", "..s..s..ssss..", type);
    compare_result("a(bc|de|[23]+)", "abcd", "..s..s..ssss..", type);
    compare_result("a(bc|de|[\\dab]+)", "abc34", "..s..s..ssssss..", type);
    compare_result("a(bc|de|[23]+)", "a2332bc", "..s..s..ssss..", type);
    compare_result("a(bc|de|[23]?)", "a2323", "..s..s..ssss..", type);
    compare_result("(ab|(cd|ef)|dd)", "efffdd", ".....s..s......", type);
    compare_result("(ab|cd|ef)11(f|d|b)", "aaaacd11fg", ".s..s..s...........", type);
    compare_result("(ab|cd|ef)11(f|d|b)", "aaaaef11fg", ".............s.s.s.", type);
    compare_result("(ab|cd|ef)11(f|d|b)", "aaaa11fg", ".s..s..s.....s.s.s.", type);
}

void test_join(MatchType match_type) {
    compare_result("1abd", "1abd", "..j.", match_type);
    compare_result("1abd", "1abd", ".jj.", match_type);
    compare_result("12abdd", "12abdd", "..jj..", match_type);
    compare_result("abc", "1abcd", "jjj", match_type);
    compare_result("abcd", "1abcd", "jjjj", match_type);
    compare_result("abca*", "abcaa", "jjj..", match_type);
    compare_result("123abcddd", "123abcddd", "...jjjj..", match_type);
    compare_result("(abcd)(efgh)(ijk)", "abcdefghijklmn", ".jjjj..jjjj..jjj.", match_type);
    compare_result("a{3}b{2}", "caaabbd", "jjjjjjjj", match_type);
}



int main() {
    auto start = high_resolution_clock::now();
    cout << "--- FULL MATCHES ---" << endl;
    test_join(MatchType::FULL);
    test_simple(MatchType::FULL);
    test_star(MatchType::FULL);
    test_brackets(MatchType::FULL);
    test_or_groups(MatchType::FULL);
    test_repetition(MatchType::FULL);
    test_combined(MatchType::FULL);
    test_or_split(MatchType::FULL);
    
    cout << "--- PARTIAL MATCHES ---" << endl;
    test_join(MatchType::PARTIAL_SINGLE);
    test_simple(MatchType::PARTIAL_SINGLE);
    test_star(MatchType::PARTIAL_SINGLE);
    test_brackets(MatchType::PARTIAL_SINGLE);
    test_or_groups(MatchType::PARTIAL_SINGLE);    
    test_repetition(MatchType::PARTIAL_SINGLE);
    test_combined(MatchType::PARTIAL_SINGLE);
    test_or_split(MatchType::PARTIAL_SINGLE);
    
    auto end = high_resolution_clock::now();
    auto dur = (duration_cast<seconds>(end - start)).count();
    cout << "time: " << dur << "s" << endl;
}
