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

    key_t *nthk_b(buffer_p b, size_t n){
        
    };
    value_t *nthv_b(buffer_p b, size_t n);
    key_t *nthk_t(buffer_p b, size_t n);
    off_t *nthc_t(buffer_p b, size_t n);

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

    node new_tnode();
    node new_block();
    size_t bsearch_t(buffer_p b, key_t k, size_t n);
    void buf_insert_t(buffer_p b, key_t k, off_t v, node &p);
    size_t bsearch_b(buffer_p b, key_t k, size_t n);
    void buf_insert_b(buffer_p b, key_t k, size_t n);
    void buf_remove_b(buffer_p b, key_t k, node &p);
    node buf_split_b(buffer_p b, node &p);
    node _insert_b(node &p, key_t k, value_t v);
    node _insert_t(node &p, key_t k, off_t v);

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

    node _insert(node &p, const key_t &k, const value_t &v);
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