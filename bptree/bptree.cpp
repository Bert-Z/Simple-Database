#include "../alloc/alloc.h"
#include <functional>
#include <vector>

template <typename key_t, typename value_t, size_t node_size = 4096, typename Compare = less<key_t>>
class bptree
{
    typedef pair<key_t, value_t> pair_t;
    typedef vector<pair_t> array_t;

  private:
    typedef char buffer_t[node_size];
    typedef char *buffer_p;

    int sz = 0;
    string filename;
    string index_file;
    file_alloc alloc;
    fstream file;
    const size_t tnode_max;
    const size_t block_max;
    off_t head, tail, root;
    Compare cmp = Compare();
    bool equal(const key_t &a, const key_t &b)
    {
        return !cmp(a, b) && !cmp(b, a);
    }

    struct node
    {
        key_t key;
        off_t pos;
        off_t father;
        off_t prev;
        off_t next;
        size_t sz = 0;
        int type; // 0:treenode; 1:block
        node(key_t key = key_t(), off_t pos = invalid_off, off_t father = invalid_off, off_t prev = invalid_off, off_t next = invalid_off, size_t sz = 0, int type = 0) : key(key), pos(pos), father(father), prev(prev), next(next), sz(sz), type(type){};
    };

    char buffer[node_size];

    void file_reopen()
    {
        if (file)
            file.flush();
    };

    void buf_load_t(char *b, node p)
    {
        file.seekg(p.pos + sizeof(node), ios::beg);
        if (!p.sz)
            return;
        file.read(b, (sizeof(off_t) + sizeof(key_t)) * p.sz);
        return;
    };

    void buf_save_t(char *b, node p)
    {
        file.seekp(p.pos + sizeof(node), ios::beg);
        file.write(b, (sizeof(off_t) + sizeof(key_t)) * p.sz);
        file_reopen();
    };

    void buf_load_b(char *b, node p)
    {
        file.seekg(p.pos + sizeof(node), ios::beg);
        if (!p.sz)
            return;
        file.read(b, (sizeof(key_t) + sizeof(value_t)) * p.sz);
    };

    void buf_save_b(char *b, node p)
    {
        file.seekp(p.pos + sizeof(node), ios::beg);
        file.write(b, (sizeof(key_t) + sizeof(value_t)) * p.sz);
        file_reopen();
    };

    void save_node(const node &p)
    {
        file.seekp(p.pos, ios::beg);
        file.write(reinterpret_cast<char *>(&p), sizeof(node));
        file_reopen();
    };

    void free_node(const node &p)
    {
        alloc.free(p.pos, node_size);
        file_reopen();
    };

    key_t *nthk_b(buffer_p b, size_t n)
    {
        return (key_t *)(b + (sizeof(key_t) + sizeof(value_t)) * n);
    };

    value_t *nthv_b(buffer_p b, size_t n)
    {
        return (value_t *)(b + (sizeof(key_t) + sizeof(value_t)) * n + sizeof(key_t));
    };

    key_t *nthk_t(buffer_p b, size_t n)
    {
        return (key_t *)(b + (sizeof(off_t) + sizeof(key_t)) * n);
    };

    off_t *nthc_t(buffer_p b, size_t n)
    {
        return (off_t *)(b + (sizeof(off_t) + sizeof(key_t)) * n + sizeof(key_t));
    };

    node read_node(off_t p)
    {
        node r;
        file.seekg(p, ios::beg);
        file.read(reinterpret_cast<char *>(&r), sizeof(node));
        return r;
    };

    off_t new_node()
    {
        sz++;
        save_index();
        return alloc.alloc(node_size);
    };

    node new_tnode(key_t key, off_t father = invalid_off, off_t prev = invalid_off, off_t next = invalid_off, size_t sz = 0)
    {
        off_t pos = new_node();
        return node(key, pos, father, prev, next, sz, 0);
    };

    node new_block(key_t key, off_t father = invalid_off, off_t prev = invalid_off, off_t next = invalid_off, size_t sz = 0)
    {
        off_t pos = new_node();
        return node(key, pos, father, prev, next, sz, 1);
    };

    size_t bsearch_t(buffer_p b, key_t k, size_t n)
    {
        size_t l = 0, r = n, mid;
        key_t *t;

        while (l < r)
        {
            mid = (l + r) / 2;
            t = nthk_t(b, mid);
            if (cmp(*t, k))
            {
                l = mid + 1;
            }
            else
            {
                r = mid;
            }
        }
        return l;
    };

    void buf_insert_t(buffer_p b, key_t k, off_t v, node &p)
    {
        size_t i, x = bsearch_t(b, k, p.sz);
        if (x < p.sz && equal(k, *nthk_t(b, x)))
            return;
        for (i = p.sz; i > x; --i)
        {
            *nthk_t(b, i) = *nthk_t(b, i - 1);
            *nthc_t(b, i) = *nthc_t(b, i - 1);
        }
        *nthk_t(b, x) = k;
        *nthc_t(b, x) = v;
        p.sz++;
        p.key = *nthk_t(b, 0);
    };

    size_t bsearch_b(buffer_p b, key_t k, size_t n)
    {
        size_t l = 0, r = n, mid;
        key_t *t;

        while (l < r)
        {
            mid = (l + r) / 2;
            t = nthk_b(b, mid);
            if (cmp(*t, k))
            {
                l = mid + 1;
            }
            else
            {
                r = mid;
            }
        }
        return l;
    };

    void buf_insert_b(buffer_p b, key_t k, off_t v, node &p)
    {
        size_t i, x = bsearch_b(b, k, p.sz);
        if (x < p.sz && equal(k, *nthk_b(b, x)))
            return;
        for (i = p.sz; i > x; --i)
        {
            *nthk_b(b, i) = *nthk_b(b, i - 1);
            *nthv_b(b, i) = *nthv_b(b, i - 1);
        }
        *nthk_b(b, x) = k;
        *nthv_b(b, x) = v;
        p.sz++;
        p.key = *nthk_b(b, 0);
    };

    void buf_remove_b(buffer_p b, key_t k, node &p)
    {
        size_t i, x = bsearch_b(b, k, p.sz);
        if (x == p.sz || !equal(*nthk_b(b, x), k))
        {
            cout << "Can't find!" << endl;
            return;
        }

        for (i = x; i < p.sz - 1; ++i)
        {
            *nthk_b(b, i) = *nthk_b(b, i + 1);
            *nthv_b(b, i) = *nthv_b(b, i + 1);
        }
        --p.sz;
        p.key = *nthk_b(b, 0);
    };

    node buf_split_b(buffer_p b, node &p)
    {
        size_t len1 = p.sz / 2, len2 = p.sz - len1;
        char *nb = (char *)nthk_b(b, len1);
        key_t nk = nthk_b(b, len1);
        node q = new_block(nk, p.father, p.pos, p.next);
        q.sz = len2;
        p.sz = len1;
        p.next = q.pos;
        save_node(q);
        buf_save_b(nb, q);
        save_node(p);
        buf_save_b(b, p);
        if (tail == p.pos)
        {
            tail = q.pos;
            save_info();
        }
        if (q.next != invalid_off)
        {
            node qn = read_node(q.next);
            qn.prev = q.pos;
            save_node(qn);
        }
        return q;
    };

    node buf_split_t(buffer_p b, node &p)
    {
        size_t len1 = p.sz / 2, len2 = p.sz - len1;
        char *nb = (char *)nthk_t(b, len1);
        key_t nk = nthk_t(b, len1);
        node q = new_tnode(nk, p.father);
        q.sz = len2;
        p.sz = len1;
        save_node(q);
        buf_save_t(nb, q);
        save_node(p);
        buf_save_t(b, p);
        return q;
    };

    node _insert_b(node &p, key_t k, value_t v)
    {
        buffer_t b;
        buf_load_b(b, p);
        buf_insert_b(b, k, v, p);
        key_t rk = *nthk_b(b, 0);
        p.key = rk;
        save_node(p);
        if (p.sz > block_max)
        {
            node q = buf_split_b(b, p);
            return q;
        }

        buf_save_b(b, p);
        return p;
    };

    node _insert_t(node &p, key_t k, off_t v)
    {
        buffer_t b;
        buf_load_t(b, p);
        buf_insert_t(b, k, v, p);
        key_t rk = *nthk_t(b, 0);
        p.key = rk;
        save_node(p);
        buf_save_t(b, p);
        return p;
    };

    void save_info()
    {
        file.seekp(0, ios::beg);
        file.write(reinterpret_cast<char *>(&head), sizeof(off_t));
        file.write(reinterpret_cast<char *>(&tail), sizeof(off_t));
        file.write(reinterpret_cast<char *>(&root), sizeof(off_t));
        file.flush();
        save_index();
    };

    void read_info()
    {
        file.seekg(0, ios::beg);
        file.read(reinterpret_cast<char *>(&head), sizeof(off_t));
        file.read(reinterpret_cast<char *>(&tail), sizeof(off_t));
        file.read(reinterpret_cast<char *>(&root), sizeof(off_t));
    };

    node _insert(node &p, const key_t &k, const value_t &v)
    {
        size_t x;
        node result, q;
        if (p.type)
        {
            result = _insert_b(p, k, v);
            return result;
        }

        buffer_t bu;
        buf_load_t(bu, p);
        x = bsearch_t(bu, k, p.sz);

        if (!equal(*nthk_t(bu, x), k) || x >= p.sz)
        {
            if (x > 0)
                --x;
            else
            {
                *nthk_t(bu, 0) = k;
                p.key = k;
                file.seekp(p.pos + sizeof(node), ios::beg);
                file.write(reinterpret_cast<char *>(&k), sizeof(key_t));
                file_reopen();
            }
        }

        q = read_node(*nthc_t(bu, x));
        result = _insert(q, k, v);
        if (result.pos != q.pos)
        {
            buf_insert_t(bu, result.key, result.pos, p);
        }
        save_node(p);
        buf_save_t(bu, p);
        if (p.sz > tnode_max)
        {
            node q = buf_split_t(bu, p);
            if (root == p.pos)
            {
                node new_root = new_tnode(p.key);
                p.father = q.father = new_root.pos;
                root = new_root.pos;
                _insert_t(new_root, p.key, p.pos);
                _insert_t(new_root, q.key, q.pos);
                save_info();
            }
            return q;
        }
        return p;
    };
    
    value_t _find(node &p, const key_t &k, const value_t &d = value_t());

    int _count(node &p, const key_t &k)
    {
        if (cmp(k, p.key))
        {
            return 0;
        }
        if (p.type)
        {
            buffer_t b;
            buf_load_b(b, p);
            size_t x = bsearch_b(b, k, p.sz);
            if (x < p.sz && equal(*nthk_b(b, x), k))
                return 1;
            else
                return 0;
        }

        buffer_t b;
        buf_load_t(b, p);
        size_t x = bsearch_t(b, k, p.sz);
        if (x > p.sz || !equal(k, *nthk_t(b, x)))
            --x;
        node cn = read_node(*nthc_t(b, x));
        return _count(cn, k);
    };

    void _set(node &p, const key_t &k, const value_t &v);
    void _balance_left_b(buffer_p b, node &p, node &l);
    void _balance_right_b(buffer_p b, node &p, node &r);
    void _merge_left_b(buffer_p b, node &p, node &l);
    void _merge_right_b(buffer_p b, node &p, node &r);
    int _balance_b(buffer_p b, node &p, off_t left, off_t right);
    int _balance_t(buffer_p b, node &p, off_t left, off_t right);
    void _balance_left_t(buffer_p b, node &p, node &l);
    void _balance_right_t(buffer_p b, node &p, node &r);
    void _merge_left_t(buffer_p b, node &p, node &l);
    void _merge_right_t(buffer_p b, node &p, node &r);
    int _remove(node &p, const key_t &k, off_t left, off_t right);
    void _search(node &p, array_t &arr, const key_t &key, function<bool(const key_t &, const key_t &)> compar);

  public:
    bptree(const string fname, const string index_fname)
        : tnode_max((node_size - sizeof(node)) / (sizeof(key_t) + sizeof(off_t)) - 1), block_max((node_size - sizeof(node)) / (sizeof(key_t) + sizeof(value_t)) - 1)
    {
        file.open(fname, ios::in);

        filename = fname;
        index_file = index_fname;
        load_index();

        if (!file)
        {
            file.open(fname, ios::out);
            init();
        }
        else
        {
            read_info();
        }
    };

    ~bptree()
    {
        save_index();
        save_info();
        if (file)
            file.close();
    };

    void init()
    {
        alloc.clear();
        save_index();
        new_node();
        head = tail = root = invalid_off;
        file.seekp(0, ios::beg);
        save_info();
    };

    void load_index()
    {
        alloc.load(index_file);
    };

    void save_index()
    {
        alloc.dump(index_file);
    };

    int count(const key_t &key)
    {
        if (root == invalid_off)
            return 0;
        node rn = read_node(root);
        return _count(rn, key);
    };

    value_t find(const key_t &key, const value_t &d = value_t());
    inline bool empty();
    void set(const key_t &key, const value_t &v);
    void insert(const key_t &key, const value_t &v);
    void remove(const key_t &key);
    inline void print_info();
    void search(array_t &arr, const key_t &key, std::function<bool(const key_t &, const key_t &)> compar);
    void traverse(std::function<void(const key_t &, const value_t &)> func);
};