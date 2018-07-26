#include "database.h"

database::database(){};

database::~database()
{
    delete dataRecord;
};

void database::open(const string file)
{
    string index_filename = "index_" + file;
    dataRecord = new bptree<int, svalue>(file.c_str(), index_filename.c_str());
}

svalue database::getValue(const int key)
{
    // for (auto it : memory)
    // {
    //     if (it.first == key)
    //     {
    //         return it.second;
    //     }
    // }

    pair<int, svalue> newone(key, dataRecord->find(key));
    // memory.push_back(newone);

    // if (memory.size() >= 20)
    // {
    //     memory.erase(memory.begin());
    // }

    return newone.second;
};

bool func(const int key1, const int key2)
{
    if (key1 < key2)
        return true;
    else
        return false;
}

void database::getRange(const int key1, const int key2)
{
    vector<pair<int, svalue>> rangeBox;
    dataRecord->search(rangeBox, key1, key2, func);

    // cout << rangeBox.size() << endl;
    for (auto it : rangeBox)
        cout << it.first << "\t" << it.second << endl;
};

void database::insert(const int key, const string value)
{

    dataRecord->insert(key, svalue(value.c_str()));
    return;
};

void database::set(const int key, const string value)
{
    // for (auto it = memory.begin(); it != memory.end(); it++)
    // {
    //     if (it->first == key)
    //     {
    //         it->second = svalue(value.c_str());
    //     }
    // }

    dataRecord->set(key, svalue(value.c_str()));
    return;
};
void database::remove(const int key)
{
    // auto it = memory.begin();
    // for (; it != memory.end(); it++)
    // {
    //     if (it->first == key)
    //         break;
    // }
    // memory.erase(it);
    dataRecord->remove(key);
    return;
};

void database::printAll()
{
    dataRecord->traverse();
    // dataRecord->printAll();
};