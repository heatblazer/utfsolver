#include <iostream>
#include <vector>
#include <ctype.h>
#include <cstdio>
#include <cstdlib>
#include <cstring> //memset
#include <fstream>

using namespace std;

namespace  {

    struct datachunk // POD type
    {
        char* data;
        size_t size;
    };

    void wb(const char* data, size_t len)
    {
        const char* pbegin = data;
        FILE* fp = fopen("out.txt", "wb");
        if (fp) {
            fwrite(pbegin, 1,len , fp);
            fclose(fp);
        }
    }

    char* load_data(const char* fname, size_t* outsize)
    {
        FILE* fp = fopen(fname, "rb");
        if (!fp) return NULL;
        fseek(fp, 0, SEEK_END);

        size_t len = ftell(fp), n = 0;
        rewind(fp);
        char* dat = (char*)malloc(sizeof(char) * len);
        char* pDat = dat;
        *outsize = len;
        if (!dat) { fclose(fp); return  NULL;}
        for(n = fread(dat, sizeof(char),  len, fp); n < len;)
        {
            n += fread(dat, sizeof(char),  len-n, fp);
        }
        fclose(fp);
        return  pDat;
    }

    struct datachunk load_data_ex(const char* fname)
    {
        struct datachunk d;
        d.data = load_data(fname, &d.size);
        return d;
    }
} //ns

//https://en.wikipedia.org/wiki/UTF-8

#define _SUTF1(d) (d >> 7) == 0
#define _SUTF2(d) (d >> 5) == 0x6
#define _SUTF3(d) (d >> 4) == 0xE
#define _SUTF4(d) (d >> 3) == 0x1E
#define _SUTF5(d) (d >> 2) == 0x3E
#define _SUTF6(d) (d >> 1) == 0x7E


class UtfSolver
{
    enum utfstate
    {
        UNKNOWN,
        UTF1,
        UTF2,
        UTF3,
        UTF4,
        UTF5,
        UTF6,
        SIZE = UNKNOWN + UTF6
    };
private:
    const char* p_data; // aggregation

    std::string m_fixData;

    size_t m_size;


    static inline bool is_utf8_sequence(const unsigned char d) { return !(~(d >> 6) & 0x2); }


    /**
     * @brief utf8heuristics
     * @param predicts the broken UTF stream
     * @return - no return
     */
    void utf8heuristics(const char* it, size_t len)
    {
        size_t ncount=0, tmpidx = 0, stridx = 0;
        //will increment h with correct offset always
        if (len == 0 || len > m_size)
            return;

        while (stridx < len)
        {
            unsigned char d = it[stridx];
            utfstate s = state(d);

            if (s != UNKNOWN)
            {
                size_t idx = validate_pattern_ex(it + stridx, s, stridx);

                if (idx == s)
                {

                    /* all ok increment index for the next utf8 */
                    for (size_t i = 0; i < idx; i++) {
                        m_fixData += it[stridx + i];//refill
                    }
                    stridx += idx; // go to next
                }
                else
                {
                    //size_t n = stridx + s > m_size ? m_size - stridx : idx; //defensive check
                    encode(it + stridx, idx, m_fixData);
                    stridx += idx;
                }
            }
            else
            {
                // case 1:
                //if begin is broken [x,1,2,3,4...... n] then h+x will have to encode n seqence till it's an utf sequence
                // case 2:
                // if somewhere is broken [1...... x,1,2,3....n] then x+j will point to that part and i count will encode n bytes
                // case 3: at end - shall be the same as 2
                tmpidx = stridx; //begin of a broken pattern
                for (ncount =0; stridx < len && UtfSolver::is_utf8_sequence(it[stridx]); stridx++, ncount++); //just find the end of the broken pattern
                encode(it + tmpidx, ncount, m_fixData); //total size - n broken strings
                ncount = 0; // reset n
            }
        }
    }


    utfstate state(unsigned char d)
    {
        utfstate state = UTF1;
        if (_SUTF1(d))
        {
            return  state;
        }
        if (_SUTF2(d))
        {
            state = UTF2;
        }
        else if (_SUTF3(d))
        {
            state = UTF3;
        }
        else if (_SUTF4(d))
        {
            state = UTF4;
        }
        else if (_SUTF5(d))
        {
            state = UTF5;
        }
        else if (_SUTF6(d))
        {
            state = UTF6;
        }
        else
        {
            state = UNKNOWN;
        }
        //        printf("State is [%s]\r\n", gStates[(unsigned)state]);
        return state;
    }

protected:

//    virtual void separate(const char* stream, )


    // some seciton in case we need to override the default implementatnion ov pattern validation ...
    /**
     * @brief validate_pattern
     * @param begin of a possible utf sequence
     * @param state of utf sequence 1,2,3,4,5,6 bytes
     * @return index pointer + offset to the end of the valid utf strem or the same pointer if not valid
     */
    virtual size_t validate_pattern_ex(const char* stream, utfstate state, size_t offset)
    {
        size_t i = 1;
        for (i = 1; i < state && (i + offset) < m_size && is_utf8_sequence(stream[i]); i++) ;
        return  i;
    }

    /**
     * @brief encoder - you can override if you need other escape pattern
     * @param it - iterator to a nonprintable chars
     * @param inout - ref to the buffer of fixed characters
    */
    virtual void encode(const char* it, size_t n, std::string& out)
    {
        for (size_t i = 0; i < n; i++) {
            char hexbuff[16]; /* lower size */
            unsigned char t = (unsigned char)it[i];
            memset(hexbuff, 0, sizeof(hexbuff));
            snprintf(hexbuff, sizeof(hexbuff), "&#x%X;", t);
            out.append(hexbuff);
        }
    }


public:
    //TODO: add size ctor
    UtfSolver(const std::string& data, const size_t size) : p_data(data.c_str()), m_size(size)
    {
        m_fixData.reserve(1024);
    }

    UtfSolver(const char* data, const size_t size) : p_data(data), m_size(size)  {}

    ~UtfSolver() /*noexcept*/ {} // not supported

    const char* data() const { return p_data;  }

    const char* fixed() const { return m_fixData.c_str();  }

    size_t size2() const {return  m_fixData.size();}

    size_t size() const { return m_size;  }

    void solve()
    {
        utf8heuristics(p_data, m_size);
    }
};


void test(const char* fname)
{
    //    puts("----- begin test  ------");
    struct datachunk d = load_data_ex(fname);
    if (d.data) {
        printf("Data ok\r\n");


        UtfSolver solver{d.data, d.size};

        std::cout << "[broken:]" << solver.data() << "\r\n";

        solver.solve();
        std::cout << "[fixed:]" << solver.fixed() << "\r\n";


        UtfSolver validator {solver.fixed(), solver.size2()};

        validator.solve();

        std::cout << "[check:]" << validator.fixed() << "\r\n";


    } else {
        printf("err in data\r\n");
    }
    free(d.data);
    //    puts("----- end test ------");
}


void test_from_bin(const char* fname)
{
    struct UtfNoEnc : public UtfSolver
    {
        UtfNoEnc(const char* d, const size_t len) : UtfSolver{d, len} { }
        UtfNoEnc(const std::string& d, const size_t len ) : UtfSolver{d, len} { }
        // override
    protected:
        virtual void encode(const char* it, size_t n, std::string& out)
        {
            (void) it; (void) n;
        }

    };

    size_t size;
    char* data = load_data(fname, &size);
    UtfNoEnc utf{data, size};
    utf.solve();
    wb(utf.fixed(), utf.size2());
}


/**
 * @brief test_raw: manual synthesis test
 */
void test_raw()
{
    char data[17] = {0};
    data[0] = 'v';
    data[1] = 'l';
    data[2] = 'a';
    data[3] = 'd';
    data[4] = 'i';
    data[5] |= (1 << 7);
    data[6] |= (1 << 7);
    data[7] |= (7 << 5);
    data[8] |= (1 << 7);// = 'p';
    data[9] |= (1 << 7);// = 'p';
    data[10] = 'i';
    data[11] = 't';
    data[12] = 's';
    data[13] = 't';
    data[14] |= (1 << 7);
    data[15] |= (1 << 7);
    data[16] |= (1 << 7);


    UtfSolver solver{data, sizeof(data)};
    solver.solve();

    UtfSolver sanity {solver.fixed(), solver.size2()};
    sanity.solve();

    std::cout << "[broken:]" << solver.data() << "\r\n";
    std::cout << "[fixed:]" << solver.fixed() << "\r\n";
    std::cout << "[check:]" << sanity.fixed() << "\r\n";
}


int main(void)
{
#if 0

    test("D:\\Dev\\git\\build-utfsolver-Desktop_Qt_5_12_2_MinGW_32_bit-Debug\\debug\\test100.txt");
    puts("---------------------------------------------------------");
    test("D:\\Dev\\git\\build-utfsolver-Desktop_Qt_5_12_2_MinGW_32_bit-Debug\\debug\\test99.txt");
    puts("---------------------------------------------------------");
    test("D:\\Dev\\git\\build-utfsolver-Desktop_Qt_5_12_2_MinGW_32_bit-Debug\\debug\\data_bigmix.txt");
    puts("---------------------------------------------------------");

    test("D:\\Dev\\git\\build-utfsolver-Desktop_Qt_5_12_2_MinGW_32_bit-Debug\\debug\\testcase.txt");
    puts("---------------------------------------------------------");

    test("D:\\Dev\\git\\build-utfsolver-Desktop_Qt_5_12_2_MinGW_32_bit-Debug\\debug\\testcase_output.txt");
    puts("---------------------------------------------------------");

#endif
//    test("paltalk.txt");
//    puts("---------------------------------------------------------");
     //test("data_bigmix.txt");
    //    puts("---------------------------------------------------------");
    //    test("D:\\Dev\\git\\build-utfsolver-Desktop_Qt_5_12_2_MinGW_32_bit-Debug\\debug\\data_err.txt");
    //    puts("---------------------------------------------------------");

//    test_raw();
//    test_from_bin("/media/storage/Builds/build-utfsolver-CLang-Debug/utfsolve/textdata/bindata");
    return 0;
}
