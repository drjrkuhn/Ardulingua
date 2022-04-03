#include "hrslip.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <slipinplace.h>
#include <sstream>

#include <sliputils.h>

#if 0
using test_encoder = slip::encoder<char>;
using test_decoder = slip::decoder<char>;
#else
using test_encoder = slip::encoder_hr;
using test_decoder = slip::decoder_hr;
#endif

inline void print_encode_results(char* buf, size_t bufsize, const char* src,
                                 size_t srcsize, bool inplace) {
    using namespace std;
    // recode the human-readable encoded input into test_decoder format
    string srcstr = slip::recode<slip::encoder_hr, test_encoder>(src, srcsize);
    src           = srcstr.c_str();
    srcsize       = srcstr.length();
    memset(buf, '.', bufsize);
    if (inplace) {
        memcpy(buf, src, srcsize);
        src = buf;
    }
    cout << "src:  " << slip::escaped(srcstr);
    size_t est_nencoded = test_encoder::encoded_size(src, srcsize);
    if (est_nencoded > bufsize) {
        cout << "<<< !!Warning!! dsize not big enough to hold encoded string";
    }
    cout << endl;
    size_t ndecoded, nencoded;

    // encode

    nencoded = test_encoder::encode(buf, bufsize, src, srcsize);
    string encstr(buf, nencoded);
    if (nencoded < bufsize) buf[nencoded] = '\0';
    cout << "encs: " << slip::escaped(encstr) << endl;
    cout << "ebuf: " << slip::escaped(buf, bufsize, "[]")  << endl;

    // decode
    size_t est_ndecoded = test_decoder::decoded_size(buf, nencoded);

    if (inplace) {
        ndecoded = test_decoder::decode(buf, bufsize, buf, nencoded);
    } else {
        ndecoded = test_decoder::decode(buf, bufsize, encstr.c_str(), encstr.length());
    }

    string decstr(buf, ndecoded);
    cout << "decs: " << slip::escaped(decstr) << endl;
    cout << "dbuf: " << slip::escaped(buf, bufsize, "[]") << endl;

    if (est_nencoded != nencoded)
        cout << "!!Warning!! encoded size estimate:" << est_nencoded
             << " did not match encoded size:" << nencoded << endl;
    if (est_ndecoded != srcsize)
        cout << "!!Warning!! decoded size estimate:" << est_ndecoded
             << " did not match source size:" << srcsize << endl;
    if (decstr != srcstr)
        cout << "!!Warning!! decoded string did not match source string" << endl;
    cout << "      bufsize:" << bufsize;
    cout << " srcsize:" << srcsize;
    cout << " est_nencoded:" << est_nencoded;
    cout << " nencoded:" << nencoded;
    cout << " est_ndecoded:" << est_ndecoded;
    cout << " ndecoded:" << ndecoded;
    cout << endl
         << endl;
}

int main() {
    using namespace std;
    const size_t bsize = 32;
    char buf[bsize];

    // inputs are in hr_encoder format

    char out1[]    = "Lorus";    //"Lorus Ipsum";
    char out2[]    = "Lo^#r#us"; // Favius## Rex\\ \\\\#\\##Aeturnum padre##";
    char out3[]    = "Lorus##";
    size_t out1len = strlen(out1);
    size_t out2len = strlen(out2);
    size_t out3len = strlen(out3);

    cout << "===== slip out-of-place =====" << endl;
    if (out1len < bsize + 1)
        print_encode_results(buf, bsize, out1, out1len, false);
    if (out1len < bsize + 1)
        print_encode_results(buf, bsize, out2, out2len, false);
    if (out1len < bsize + 1)
        print_encode_results(buf, bsize, out3, out3len, false);

    cout << "===== slip in-place =====" << endl;
    if (out1len < bsize + 1)
        print_encode_results(buf, bsize, out1, out1len, true);
    if (out2len < bsize + 1)
        print_encode_results(buf, bsize, out2, out2len, true);
    if (out3len < bsize + 1)
        print_encode_results(buf, bsize, out3, out3len, true);

    return 0;
}
