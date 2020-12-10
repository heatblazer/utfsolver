#include <iostream>
#include <fstream>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <cstring>

void gen_utf_random(int, std::string&);

void gen_broken_utf(int, std::string&);

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "useage:\r\n  \t-n <count> \r\nto generate normal files or\r\n"
                  << "\t-n <count> -b\r\nto generate broken\r\n"
                  <<"--------------------------" << std::endl;
        return  0;
    }

    int cnt;
    std::string out;
    std::ofstream outfile;
    bool breakit = false;
    for (int i=1; i < argc; ++i) {
        if (!strcmp(argv[i], "-n") && i <= argc) {
            sscanf(argv[i+1], "%d", &cnt);
        } else if (!strcmp(argv[i], "-b")) {
            breakit = true;
        }
    }

    if (breakit) {
        outfile.open ("utfgenbroken.txt");
        gen_broken_utf(cnt, out);
    } else {
        outfile.open ("utfgen.txt");
        gen_utf_random(cnt, out);
    }

    outfile << out;
    outfile.close();

    std::cout << "Finished generating " << (breakit ? "broken" : "normal") <<
        " pattern\r\n";
    return 0;
}

void gen_utf_random(int count, std::string& out)
{
    srand(time(NULL));
    for(int i=0; i < count; ++i) {
        int r = rand() % 4;
        switch (r) {
        case 0:
            out += '1';
            break;
        case 1:
            out += char(3 << 6);
            out += char(1 << 7);
            out += '2';
            break;
        case 2:
            out += char(7 << 5);
            out += char(1 << 7);
            out += char(1 << 7);
            out += char('3');
            break;
        case 3:
            out += char(0xF << 4);
            out += char(1 << 7);
            out += char(1 << 7);
            out += char(1 << 7);
            out += char('4');
            break;
        default:
            break;
        }
    }
}



void gen_broken_utf(int cnt, std::string& out)
{
    std::string s;
    gen_utf_random(cnt, s);

    for(size_t i=0; i < s.size(); ++i) {
        int r = rand() % 4;
        switch (r) {
        case 0:
            s[i] = 'X';
            break;
        default:
            break;
        }
    }
    out = s;
}
