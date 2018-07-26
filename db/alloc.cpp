#include "alloc.h"
#include <iostream>

void file_alloc::del_node(node *p)
{
    if (p->prev)
        p->prev->next = p->next;
    else
        head = p->next;
    if (p->next)
        p->next->prev = p->prev;
    else
        tail = p->prev;
    delete p;
}

inline file_alloc::node *file_alloc::insert_before(node *p, off_t start, off_t end)
{
    node *q = new node(start, end, p->prev, p);
    if (p->prev)
        p->prev->next = q;
    else
        head = q;
    p->prev = q;
    return q;
}

inline file_alloc::node *file_alloc::insert_tail(off_t start, off_t end)
{
    if (empty())
    {
        head = tail = new node(start, end);
        return head;
    }
    node *q = new node(start, end, tail, nullptr);
    tail->next = q;
    tail = q;
    return q;
}

inline void file_alloc::merge(node *p)
{
    node *q;
    if (p->next && p->next->start == p->end)
    {
        q = p->next;
        p->end = q->end;
        del_node(q);
    }
    if (p->prev && p->prev->end == p->start)
    {
        q = p->prev;
        q->end = p->end;
        del_node(p);
    }
}

file_alloc::file_alloc(const char *fname)
{
    //filename = (char *)malloc(sizeof(fname));
    //strcpy(filename, fname);
    head = tail = nullptr;
    //file = fopen(fname,"rb+");
    file_end = 0;
}

file_alloc::~file_alloc()
{
    clear();
}

void file_alloc::load(const char *filename)
{
    clear();
    FILE *fp = fopen(filename, "rb");
    if (!fp)
        return;
    //throw non_existent_file();
    off_t s[2];
    int read_cnt;
    fread(&file_end, sizeof(off_t), 1, fp);
    while (!feof(fp))
    {
        read_cnt = fread(s, sizeof(off_t), 2, fp);
        if (read_cnt == 0)
            break;
        if (read_cnt == 1)
            throw runtime_error("invalid format");
        insert_tail(s[0], s[1]);
    }
    fclose(fp);
}

void file_alloc::dump(const char *filename)
{
    FILE *fp = fopen(filename, "wb");
    node *p = head;
    off_t s[2];
    fwrite(&file_end, sizeof(off_t), 1, fp);
    for (; p; p = p->next)
    {
        //printf("write\n");
        s[0] = p->start;
        s[1] = p->end;
        fwrite(s, sizeof(off_t), 2, fp);
    }
    if (fp)
        fclose(fp);
}

inline bool file_alloc::empty()
{
    return head == nullptr;
}

void file_alloc::clear()
{
    file_end = 0;
    node *p = head, *q;
    while (p)
    {
        q = p->next;
        delete p;
        p = q;
    }
    head = tail = nullptr;
}

off_t file_alloc::alloc(size_t len)
{
    node *p = head;
    off_t pos;
    for (; p != nullptr && p->size() < len; p = p->next)
        ;
    if (p)
    {
        pos = p->start;
        p->start += len;
        if (p->start == p->end)
            del_node(p);
        return pos;
    }
    else
    {
        pos = file_end;
        file_end += len;
        return pos;
    }
}

void file_alloc::free(off_t pos, size_t len)
{
    node *p = head, *q;
    for (; p && p->start < pos; p = p->next)
        ;

    if (p)
    {
        if (p->start < pos + len)
        {
            //printf("invalid address: %d %d %d\n", p->start, pos, len);
            throw runtime_error("unallocated_space");
            // throw unallocated_space();
        }
        q = insert_before(p, pos, pos + len);
    }
    else
    {
        q = insert_tail(pos, pos + len);
    }
    //printf("free succeed\n");
    merge(q);
    return;
}

// int main()
// {
//     file_alloc newtry;
//     newtry.load("data");

//     cout << "first" << endl;
//     newtry.print();

//     newtry.clear();
//     cout << newtry.empty() << endl;

//     cout << "second" << endl;
//     newtry.dump("data");
//     newtry.print();
//     system("pause");

//     return 0;
// }
