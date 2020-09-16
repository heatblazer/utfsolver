#include <iostream>
#include <vector>
#include <unordered_map>
#include <ctype.h>
#include <cstdio>
#include <cstdlib>
#include <cstring> //memset
using namespace std;


namespace  {
    char* load_data(const char* fname, size_t* outsize)
    {
        FILE* fp = fopen(fname, "rb");
        if (!fp) return NULL;
        fseek(fp, 0, SEEK_END);

        size_t len = ftell(fp), n = 0;
        rewind(fp);
        char* dat = (char*)malloc(sizeof(char) * len);
        *outsize = len;
        if (!dat) { fclose(fp); return  NULL;}
        for(n = fread(dat, sizeof(char),  len, fp); n < len;)
        {
            n += fread(dat, sizeof(char),  len-n, fp);
        }
        fclose(fp);
        return  dat;
    }

}

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

    static inline bool is_starting_utf8(const unsigned char d)
    {
        return (_SUTF1(d) || _SUTF2(d) || _SUTF3(d) || _SUTF4(d)) || _SUTF5(d) || _SUTF6(d);
    }

    static inline bool is_utf8_sequence(const unsigned char d) { return ((d >> 6) == 2); }

    bool valid_sequence(const char* it, utfstate state)
    {
        size_t i = 1;
        for (; i < state && UtfSolver::is_utf8_sequence(it[i]); i++);
        return i + 1 == state;
    }

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
                    for (size_t i = 0; i < idx; i++)
                            m_fixData += it[stridx + i]; //refill
                        stridx += idx; // go to next
                }
                else
                    {
                    encode(it + stridx, s, m_fixData);
                    stridx += s;
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
        else if (_SUTF5(d))
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
    // some seciton in case we need to override the default implementatnion ov pattern validation ...
            /**
         * @brief validate_pattern
         * @param begin of a possible utf sequence
         * @param state of utf sequence 1,2,3,4,5,6 bytes
         * @return index pointer + offset to the end of the valid utf strem or the same pointer if not valid
         */
    virtual size_t validate_pattern_ex(const char* stream, utfstate state, size_t offset)
    {
        bool isOk = true;
        if (offset >= m_size) return 0; //bound check
        for (size_t i = 1; i < state; i++)
            if (!is_utf8_sequence(stream[i])) {
                isOk = false;
                break;
            }

        return  isOk ? (size_t)state : 0;
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
            unsigned int t = (unsigned int)it[i];
            memset(hexbuff, 0, sizeof(hexbuff));
            snprintf(hexbuff, sizeof(hexbuff), "&#x%X;", ((t << 24) & 0xFF000000) >> 24);
            out.append(hexbuff);
        }
    }


public:
    //TODO: add size ctor
    UtfSolver(const std::string& data, const size_t size) : p_data(data.c_str()), m_size(size) {}

    UtfSolver(const char* data, const size_t size) : p_data(data), m_size(size)  {}

    ~UtfSolver() /*noexcept*/ {} // not supported

    const char* data() const { return p_data;  }

    const char* fixed() const { return m_fixData.c_str();  }

    size_t size() const { return m_size;  }

    void resolve()
    {
        utf8heuristics(p_data, m_size);
    }
};


void test(const char* fname)
{
    puts("----- begin test  ------");

    size_t size = 0;
    char* data = load_data(fname, &size);
    if (data) {
        printf("Data ok\r\n");
        UtfSolver solver{data, size};
        solver.resolve();
        std::cout << solver.fixed() << "\r\n";
    } else {
        printf("err in data\r\n");
    }
    free(data);
    puts("----- end test ------");
}


int main(void)
{
    test("data_err.txt");

    return 0;
}
