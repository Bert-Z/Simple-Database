#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
//A very simple try

using namespace std;

//typedef long off_t;
constexpr off_t invalid_off = 0xdeadbeef;

//typedef unsigned long size_t;

class file_alloc
{
  private:
    size_t node_size = 4096;
    struct node
    {
        off_t start, end;
        node *prev, *next;
        node(off_t start = 0, off_t end = 0, node *prev = nullptr, node *next = nullptr) : start(start), end(end), prev(prev), next(next) {}
        inline size_t size()
        {
            return end - start;
        }
        inline void print()
        {
            printf("[block start:%d end:%d]", start, end);
        }
    };
    node *head, *tail;
    //char * filename;
    //FILE * file;
    off_t file_end;

    inline void del_node(node *p);
    inline node *insert_before(node *p, off_t start, off_t end);
    inline node *insert_tail(off_t start, off_t end);
    inline void merge(node *p);

  public:
    file_alloc(const char *filename = "");
    ~file_alloc();

    void load(const char *filename = "");
    void dump(const char *filename = "");

    inline bool empty();
    void clear();
    off_t alloc(size_t len);
    void free(off_t pos, size_t len);
    void print()
    {
        node *p = head;
        for (; p; p = p->next)
        {
            p->print();
            printf("\n");
        }
    }
};

//template <class Key, class T>