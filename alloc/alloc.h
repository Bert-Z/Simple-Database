#pragma once
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

constexpr off_t invalid_off = 0xdeadbeef; //之前的无效

class file_alloc
{
  private:
    size_t nodesize = 4096;
    struct node
    {
        off_t start, end;
        node *prev, *next;
        node(off_t start = 0, off_t end = 0, node *prev = nullptr, node *next = nullptr) : start(start), end(end), prev(prev), next(next){};
        size_t size()
        {
            return this->end - this->start;
        }
        void print()
        {
            cout << "Block Start: " << this->start << ", End: " << this->end << endl;
        }
    };
    node *head, *tail;
    off_t fileend;

    void delete_node(node *p);
    node *insert_before(node *p, off_t start, off_t end);
    node *insert_tail(off_t start, off_t end);
    void merge(node *p);

  public:
    file_alloc() : head(nullptr), tail(nullptr), fileend(0){};
    ~file_alloc();

    void load(const string filename);
    void dump(const string filename);

    bool empty();
    void clear();
    off_t alloc(size_t len);
    void free(off_t pos, size_t len);
    void print()
    {
        node *p = this->head;
        for (; p; p = p->next)
        {
            p->print();
        }
    }
};