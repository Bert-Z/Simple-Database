#include "alloc.h"

void file_alloc::delete_node(node *p)
{
    if (p->next)
        p->next->prev = p->prev;
    else
        this->tail = p->prev;

    if (p->prev)
        p->prev->next = p->next;
    else
        this->head = p->next;

    delete p;
    return;
}

file_alloc::node *file_alloc::insert_before(node *p, off_t start, off_t end)
{
    node *newone = new node(start, end, p->prev, p);
    if (p->prev)
        p->prev->next = newone;
    else
        head = newone;

    p->prev = newone;

    return newone;
}

file_alloc::node *file_alloc::insert_tail(off_t start, off_t end)
{
    if (this->empty())
    {
        node *newone = new node(start, end);
        head = tail = newone;
        return newone;
    }

    node *newone = new node(start, end, tail);
    tail->next = newone;
    tail = newone;

    return newone;
}

void file_alloc::merge(node *p)
{
    node *q;
    if (p->next && p->next->start == p->end)
    {
        q = p->next;
        p->end = q->end;
        delete_node(q);
    }
    if (p->prev && p->prev->end == p->start)
    {
        q = p->prev;
        q->end = p->end;
        delete_node(p);
    }
}

file_alloc::~file_alloc()
{
    this->clear();
}

void file_alloc::load(const string filename)
{
    clear();

    fstream file(filename, ios::in);
    if (!file)
    {
        cout << "File open wrong!" << endl;
        return;
    }

    off_t nstart, nend;

    file.read(reinterpret_cast<char *>(&fileend), sizeof(off_t));

    while (!file.eof())
    {
        file.read(reinterpret_cast<char *>(&nstart), sizeof(off_t));

        if (file.gcount() == 0) //得到实际读取的值，判断是否到结尾
            break;

        file.read(reinterpret_cast<char *>(&nend), sizeof(off_t));

        insert_tail(nstart, nend);
    }

    file.close();
}

void file_alloc::dump(const string filename)
{
    fstream file;
    file.open(filename, ios::out);

    file.write(reinterpret_cast<char *>(&fileend), sizeof(off_t));

    node *p = this->head;
    for (; p; p = p->next)
    {
        off_t nstart, nend;
        nstart = p->start;
        nend = p->end;
        file.write(reinterpret_cast<char *>(&nstart), sizeof(off_t));
        file.write(reinterpret_cast<char *>(&nend), sizeof(off_t));
    }

    file.close();
}

bool file_alloc::empty()
{
    return this->head == nullptr;
}

void file_alloc::clear()
{
    fileend = 0;

    node *p, *q;
    p = head;

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
    node *p = this->head;
    off_t pos;

    for (; p && (p->size()) < len; p = p->next)
        ;

    if (p)
    {
        pos = p->start;
        p->start += len;
        if (p->start == p->end)
            delete_node(p);
        return pos;
    }
    else
    {
        pos = fileend;
        fileend += len;
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
            cout << "Haven't alloc yet!" << endl;
            return;
        }
        q = insert_before(p, pos, pos + len);
    }
    else
    {
        q = insert_tail(pos, pos + len);
    }

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

//     cout << "second" << endl;
//     newtry.dump("data");
//     newtry.print();
//     system("pause");

//     return 0;
// }