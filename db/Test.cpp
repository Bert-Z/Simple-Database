#include "database.h"
#include <stdlib.h>
#include <time.h>

database db;

void read_command()
{
    string command;
    cin >> command;

    while (command != "CLOSE")
    {
        if (command == "GET")
        {
            int key;

            cout << "Enter key: ";
            cin >> key;
            cout << db.getValue(key) << endl;
            cout << endl;
        }
        else if (command == "GETRANGE")
        {
            int key1, key2;
            string value;
            vector<pair<int, string>> rangeBox;

            cout << "Enter key1: ";
            cin >> key1;
            cout << "Enter key2: ";
            cin >> key2;

            db.getRange(key1, key2);
            cout << endl;
        }
        else if (command == "INSERT")
        {
            int key;
            string value;

            cout << "Enter key: ";
            cin >> key;
            cout << "Enter value: ";
            cin >> value;

            db.insert(key, value);
            cout << endl;
        }
        else if (command == "SET")
        {
            int key;
            string value;

            cout << "Enter key: ";
            cin >> key;
            cout << "Enter value: ";
            cin >> value;

            db.set(key, value);
            cout << endl;
        }
        else if (command == "REMOVE")
        {
            int key;

            cout << "Enter key: ";
            cin >> key;

            db.remove(key);
            cout << endl;
        }
        else if (command == "PRINTALL")
        {
            db.printAll();
            cout << endl;
        }
        else
        {
            cout << "Wrong command!" << endl;
            cout << endl;
        }

        cin >> command;
    }

    cout << "Thank you for using this database!" << endl;
    cout << endl;
    system("pause");
}

char *rand_str(char *str, const int len)
{

    int i;
    for (i = 0; i < len; ++i)
    {
        switch ((rand() % 3))
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

    // string filepath;
    string file;
    cout << "Welcome to my simple database!" << endl;
    cout << "(Attention: this is an key-Value(int-string) database )!" << endl;
    cout << "*****************************************************************" << endl;
    cout << endl;

    // cout << "Please enter the dbfile path: ";
    // cin >> filepath;

    cout << "Please enter the dbfile you need to open: ";
    cin >> file;
    db.open(file);

    cout << "Now you can do these options on the database: " << endl;
    cout << "GET        --get the key's value" << endl;
    cout << "GETRANGE   --print the key-value of [key1,key2]" << endl;
    cout << "INSERT     --insert one key-value" << endl;
    cout << "SET        --set one key-value" << endl;
    cout << "REMOVE     --remove one key and its value" << endl;
    cout << "PRINTALL   --print all the key-value in dbfile" << endl;
    cout << "CLOSE      --close the database file" << endl;
    cout << "*****************************************************************" << endl;

    clock_t start = clock();
    for (int i = 0; i < 100000; i++)
    {
        if (i % 10000 == 0)
            cout << i << endl;
        char name[30];
        string newstring = rand_str(name, 30);
        db.insert(i, newstring);
    }
    clock_t finish = clock();
    cout << (double)(finish - start) / 1000 << "s" << endl;
    cout << endl;

    cout << "RANDOM GET 2000 DATA" << endl;
    start = clock();
    for (int i = 0; i < 2000; i++)
        db.getValue(rand() % 100000);
    finish = clock();
    cout << (double)(finish - start) / 1000 << "s" << endl;
    cout << endl;

    cout << "RANDOM RESET 2000 DATA" << endl;
    start = clock();
    for (int i = 0; i < 2000; i++)
        db.set(rand() % 100000,"bibqwf");
    finish = clock();
    cout << (double)(finish - start) / 1000 << "s" << endl;
    cout << endl;

    cout << "GET 50" << endl;
    start = clock();
    cout << db.getValue(50) << endl;
    finish = clock();
    cout << (double)(finish - start) / 1000 << "s" << endl;
    cout << endl;

    cout << "RESET 50  =>  ASD" << endl;
    start = clock();
    db.set(50, "ASD");
    cout << db.getValue(50) << endl;
    finish = clock();
    cout << (double)(finish - start) / 1000 << "s" << endl;
    cout << endl;

    cout << "GETRANGE 40=>60" << endl;
    start = clock();
    db.getRange(40, 60);
    finish = clock();
    cout << (double)(finish - start) / 1000 << "s" << endl;
    cout << endl;

    cout << "REMOVE 50" << endl;
    start = clock();
    db.remove(50);
    finish = clock();
    cout << (double)(finish - start) / 1000 << "s" << endl;
    cout << endl;

    cout << "GETRANGE 40=>60" << endl;
    start = clock();
    db.getRange(40, 60);
    finish = clock();
    cout << (double)(finish - start) / 1000 << "s" << endl;
    cout << endl;

    cout << "INSERT 50 => FEQ" << endl;
    start = clock();
    db.insert(50, "FEQ");
    finish = clock();
    cout << (double)(finish - start) / 1000 << "s" << endl;
    cout << endl;

    cout << "GETRANGE 40=>60" << endl;
    start = clock();
    db.getRange(40, 60);
    finish = clock();
    cout << (double)(finish - start) / 1000 << "s" << endl;
    cout << endl;

    read_command();

    return 0;
}