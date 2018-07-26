#include "bptree.hpp"

typedef bptree<int, string> bpRecord;

int main()
{
    try
    {
        string filename;
        cin >> filename;

        string index_filename = string("index_") + filename;

        bpRecord newRecord(filename, index_filename);

        newRecord.insert(1, "hello1");
        newRecord.insert(2, "hello2");
        newRecord.insert(3, "hello3");
        newRecord.insert(4, "hello4");
        newRecord.insert(5, "hello5");
        newRecord.insert(6, "hello6");
        newRecord.insert(7, "hello7");
        newRecord.insert(8, "hello8");

        cout << newRecord.find(5) << endl;
        newRecord.set(5, "new");

        cout << newRecord.find(5) << endl;
    }
    catch (exception &e)
    {
        cerr << e.what() << endl; //输出错误
    }
    catch (...)
    {
        cerr << "exception\n";
    }

    system("pause");
    return 0;
}