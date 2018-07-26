#include <iostream>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <time.h>

using namespace std;


char *rand_str(char *str, const int len)
{
    random_device rd;

    int i;
    for (i = 0; i < len; ++i)
    {
        switch ((rd() % 3))
        {
        case 1:
            str[i] = 'A' + rand() % 26;
            break;
        case 2:
            str[i] = 'a' + rand() % 26;
            break;
        default:
            str[i] = '0' + rand() % 10;
            break;
        }
    }
    str[++i] = '\0';
    return str;
}

int main(void)
{
    srand(time(NULL));

    char name[30];
    for (int i = 0; i < 100; i++)
    {
        // if (i % 10 == 0)
        //     cout << i << endl;

        cout << rand_str(name, 30) << endl;
    }

    system("pause");
    return 0;
}