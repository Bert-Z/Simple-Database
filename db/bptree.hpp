
#include "alloc.h"
#include <functional>
#include <iostream>
#include <vector>
//typedef int off_t;
template <class key_t, class value_t, size_t node_size = 4096, class Compare = std::less<key_t>>
class bptree
{
	typedef pair<key_t, value_t> pair_t;
	typedef vector<pair_t> array_t;

  private:
	typedef char buffer_t[node_size];
	typedef char *buffer_p;

	int sz = 0;
	char *filename;
	char *index_file;
	FILE *file;
	file_alloc alloc;
	const size_t tnode_max;
	const size_t block_max;
	off_t head, tail, root;
	Compare cmp = Compare();
	bool equal(const key_t &a, const key_t &b)
	{
		return !cmp(a, b) && !cmp(b, a);
	}
	/*
	[Tree node information] | a0 | [a0, a1) | a1 | [a1, a2) | a2 | [a2, a3) | a3 | ... | an-1 | [an-1, +inf)
	*/

	struct node
	{
		key_t key;
		off_t pos;
		off_t father;
		off_t prev;
		off_t next;
		size_t sz = 0;
		int type; //0 tree node 1 block
		node(key_t key = key_t(),
			 off_t pos = invalid_off,
			 off_t father = invalid_off,
			 off_t prev = invalid_off,
			 off_t next = invalid_off,
			 size_t sz = 0,
			 int type = 0)
			: key(key), pos(pos), father(father), prev(prev), next(next), sz(sz), type(type) {}
	};

	char buffer[node_size];
	inline void file_reopen()
	{
		if (file)
			fflush(file);
	}
	inline void move_to_data(const node &p)
	{
		fseek(file, p.pos + sizeof(node), SEEK_SET);
	}

	inline void buf_load_t(char *b, node p)
	{
		move_to_data(p);
		if (!p.sz)
			return;
		fread(b, 1, (sizeof(off_t) + sizeof(key_t)) * p.sz, file);
		//file_reopen();
	}
	inline void buf_save_t(char *b, node p)
	{
		int i;
		move_to_data(p);
		fwrite(b, 1, (sizeof(key_t) + sizeof(off_t)) * p.sz, file);
		file_reopen();
	}

	inline void buf_load_b(char *b, node p)
	{
		move_to_data(p);
		if (!p.sz)
			return;
		fread(b, 1, (sizeof(key_t) + sizeof(value_t)) * p.sz, file);
	}

	inline void buf_save_b(char *b, node p)
	{
		move_to_data(p);
		fwrite(b, 1, (sizeof(key_t) + sizeof(value_t)) * p.sz, file);
		file_reopen();
	}

	inline void save_node(const node &p)
	{
		fseek(file, p.pos, SEEK_SET);
		fwrite(&p, sizeof(node), 1, file);
		file_reopen();
	}

	inline void free_node(const node &p)
	{
		alloc.free(p.pos, node_size);
		save_index();
	}

	key_t *nthk_b(buffer_p b, size_t n)
	{
		return (key_t *)(b + (sizeof(key_t) + sizeof(value_t)) * n);
	}
	value_t *nthv_b(buffer_p b, size_t n)
	{
		return (value_t *)(b + (sizeof(key_t) + sizeof(value_t)) * n + sizeof(key_t));
	}

	key_t *nthk_t(buffer_p b, size_t n)
	{
		return (key_t *)(b + (sizeof(off_t) + sizeof(key_t)) * n);
	}
	off_t *nthc_t(buffer_p b, size_t n)
	{
		return (off_t *)(b + (sizeof(key_t)) * (n + 1) + sizeof(off_t) * n);
	}

	node read_node(off_t p)
	{
		node r;
		fseek(file, p, SEEK_SET);
		fread(&r, sizeof(node), 1, file);
		return r;
	}
	inline off_t new_node()
	{
		sz++;
		save_index();
		return alloc.alloc(node_size);
	}
	inline node new_tnode(
		key_t key,
		off_t father = invalid_off,
		off_t prev = invalid_off,
		off_t next = invalid_off,
		size_t sz = 0)
	{
		off_t pos = new_node();
		return node(key, pos, father, prev, next, sz, 0);
	}
	inline node new_block(
		key_t key,
		off_t father = invalid_off,
		off_t prev = invalid_off,
		off_t next = invalid_off,
		size_t sz = 0)
	{
		off_t pos = new_node();
		return node(key, pos, father, prev, next, sz, 1);
	}

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
	}
	void buf_insert_t(buffer_p b, key_t k, off_t v, node &p)
	{
		int i, x = bsearch_t(b, k, p.sz);
		if (x < p.sz && equal(k, *nthk_t(b, x)))
			return;
		for (i = p.sz; i > x; --i)
		{
			*nthk_t(b, i) = *nthk_t(b, i - 1);
			*nthc_t(b, i) = *nthc_t(b, i - 1);
		}
		p.sz++;
		*nthk_t(b, x) = k;
		*nthc_t(b, x) = v;
		p.key = *nthk_t(b, 0);
	}

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
	}
	void buf_insert_b(buffer_p b, key_t k, value_t v, node &p)
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
	}
	void buf_remove_b(buffer_p b, key_t k, node &p)
	{
		size_t i, x = bsearch_b(b, k, p.sz);
		if (x == p.sz || !equal(*nthk_b(b, x), k))
		{
			//puts("here");
			throw runtime_error("not_found");
			// throw not_found();
		}
		//if (x == p.sz || !equal(*nthk_b(b, x), k)) return;
		for (i = x; i < p.sz - 1; ++i)
		{
			*nthk_b(b, i) = *nthk_b(b, i + 1);
			*nthv_b(b, i) = *nthv_b(b, i + 1);
		}
		--p.sz;
		p.key = *nthk_b(b, 0);
		//printf("!%d\n", p.key);
	}

	node buf_split_b(buffer_p b, node &p)
	{
		size_t len1 = p.sz / 2, len2 = p.sz - len1;
		char *nb = b + (sizeof(key_t) + sizeof(value_t)) * len1;
		key_t nk = *nthk_b(b, len1);
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
	}

	node buf_split_t(buffer_p b, node &p)
	{
		size_t len1 = p.sz / 2, len2 = p.sz - len1;
		char *nb = (char *)nthk_t(b, len1);
		key_t nk = *nthk_t(b, len1);
		node q = new_tnode(nk, p.father);
		q.sz = len2;
		p.sz = len1;
		save_node(q);
		buf_save_t(nb, q);
		save_node(p);
		buf_save_t(b, p);
		return q;
	}

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
	}

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
	}

	inline void save_info()
	{
		fseek(file, 0, SEEK_SET);
		fwrite(&head, sizeof(off_t), 1, file);
		fwrite(&tail, sizeof(off_t), 1, file);
		fwrite(&root, sizeof(off_t), 1, file);
		fflush(file);
		save_index();
	}
	inline void read_info()
	{
		fseek(file, 0, SEEK_SET);
		fread(&head, sizeof(off_t), 1, file);
		fread(&tail, sizeof(off_t), 1, file);
		fread(&root, sizeof(off_t), 1, file);
	}

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
				move_to_data(p);
				fwrite(&k, sizeof(key_t), 1, file);
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
				//printf("new root!\n");
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
	}
	value_t _find(node &p, const key_t &key, const value_t &d = value_t())
	{
		if (cmp(key, p.key))
		{
			return d;
		}
		if (p.type)
		{
			buffer_t b;
			buf_load_b(b, p);
			size_t x = bsearch_b(b, key, p.sz);
			if (x < p.sz && equal(*nthk_b(b, x), key))
				return *nthv_b(b, x);
			else
			{
				return d;
			}
		}
		buffer_t b;
		buf_load_t(b, p);
		size_t x = bsearch_t(b, key, p.sz);
		if (x >= p.sz || !equal(key, *nthk_t(b, x)))
			--x;
		node cn = read_node(*nthc_t(b, x));
		return _find(cn, key, d);
	}

	int _count(node &p, const key_t &key)
	{
		if (cmp(key, p.key))
		{
			return 0;
		}
		if (p.type)
		{
			buffer_t b;
			buf_load_b(b, p);
			size_t x = bsearch_b(b, key, p.sz);
			if (x < p.sz && equal(*nthk_b(b, x), key))
				return 1;
			else
			{
				return 0;
			}
		}
		buffer_t b;
		buf_load_t(b, p);
		size_t x = bsearch_t(b, key, p.sz);
		if (x >= p.sz || !equal(key, *nthk_t(b, x)))
			--x;
		node cn = read_node(*nthc_t(b, x));
		return _count(cn, key);
	}
	void _set(node &p, const key_t &key, const value_t &v)
	{
		if (cmp(key, p.key))
		{
			return;
		}
		if (p.type)
		{
			buffer_t b;
			buf_load_b(b, p);
			size_t x = bsearch_b(b, key, p.sz);
			if (x < p.sz && equal(*nthk_b(b, x), key))
			{
				*nthv_b(b, x) = v;
				buf_save_b(b, p);
				return;
			}
			else
			{
				//puts("here");
				throw runtime_error("not_found");
				// throw not_found();
			}
		}
		buffer_t b;
		buf_load_t(b, p);
		size_t x = bsearch_t(b, key, p.sz);
		if (x >= p.sz || !equal(key, *nthk_t(b, x)))
			--x;
		node cn = read_node(*nthc_t(b, x));
		_set(cn, key, v);
	}

	void _balance_left_b(buffer_p b, node &p, node &l)
	{
		buffer_t bl;
		buf_load_b(bl, l);
		int i;
		for (i = p.sz; i > 0; --i)
		{
			*nthk_b(b, i) = *nthk_b(b, i - 1);
			*nthv_b(b, i) = *nthv_b(b, i - 1);
		}
		*nthk_b(b, 0) = *nthk_b(bl, l.sz - 1);
		*nthv_b(b, 0) = *nthv_b(bl, l.sz - 1);
		p.key = *nthk_b(b, 0);
		p.sz++;
		l.sz--;
		save_node(p);
		save_node(l);
		buf_save_b(b, p);
		buf_save_b(bl, l);
	}

	void _balance_right_b(buffer_p b, node &p, node &r)
	{
		buffer_t br;
		buf_load_b(br, r);
		p.sz++;
		*nthk_b(b, p.sz - 1) = *nthk_b(br, 0);
		*nthv_b(b, p.sz - 1) = *nthv_b(br, 0);
		int i;
		for (i = 0; i < r.sz - 1; ++i)
		{
			*nthk_b(br, i) = *nthk_b(br, i + 1);
			*nthv_b(br, i) = *nthv_b(br, i + 1);
		}
		//if (r.key != *nthk_b(br, 0)) printf("baka %d %d\n", r.key, *nthk_b(br, 0));
		r.key = *nthk_b(br, 0);
		r.sz--;
		save_node(p);
		save_node(r);
		buf_save_b(b, p);
		buf_save_b(br, r);
	}

	void _merge_left_b(buffer_p b, node &p, node &l)
	{
		buffer_t bl;
		buf_load_b(bl, l);
		int i;
		for (i = 0; i < p.sz; ++i)
		{
			*nthk_b(bl, i + l.sz) = *nthk_b(b, i);
			*nthv_b(bl, i + l.sz) = *nthv_b(b, i);
		}
		l.sz += p.sz;
		l.next = p.next;
		if (p.next != invalid_off)
		{
			node nn = read_node(p.next);
			nn.prev = l.pos;
			save_node(nn);
		}
		free_node(p);
		save_node(l);
		buf_save_b(bl, l);
	}

	void _merge_right_b(buffer_p b, node &p, node &r)
	{
		buffer_t br;
		buf_load_b(br, r);
		int i;
		for (i = 0; i < r.sz; ++i)
		{
			*nthk_b(b, i + p.sz) = *nthk_b(br, i);
			*nthv_b(b, i + p.sz) = *nthv_b(br, i);
		}
		p.sz += r.sz;
		p.next = r.next;
		if (r.next != invalid_off)
		{
			node nn = read_node(r.next);
			nn.prev = p.pos;
			save_node(nn);
		}
		free_node(r);
		save_node(p);
		buf_save_b(b, p);
	}

	int _balance_b(buffer_p b, node &p, off_t left, off_t right)
	{
		//-1 merged with left node 0 not merged 1 merged with right node 2 deleted
		if (left == invalid_off && right == invalid_off)
		{
			if (p.sz == 0)
			{
				//printf("haha");
				free_node(p);
				head = tail = invalid_off;
				save_info();
				save_node(p);
				buf_save_b(b, p);
				return 2;
			}
			save_node(p);
			buf_save_b(b, p);
			return 0;
		}
		if (left == invalid_off)
		{
			node r = read_node(right);
			if (r.sz > block_max / 2)
			{
				_balance_right_b(b, p, r);
				return 0;
			}
			else
			{
				_merge_right_b(b, p, r);
				return 1;
			}
		}
		if (right == invalid_off)
		{
			node l = read_node(left);
			if (l.sz > block_max / 2)
			{
				_balance_left_b(b, p, l);
				return 0;
			}
			else
			{
				_merge_left_b(b, p, l);
				return -1;
			}
		}
		node l = read_node(left), r = read_node(right);
		if (l.sz > block_max / 2)
		{
			_balance_left_b(b, p, l);
			return 0;
		}
		else if (r.sz > block_max / 2)
		{
			_balance_right_b(b, p, r);
			return 0;
		}
		else
		{
			_merge_left_b(b, p, l);
			return -1;
		}
	}

	int _balance_t(buffer_p b, node &p, off_t left, off_t right)
	{
		//-1 merged with left node 0 not merged 1 merged with right node 2 deleted
		if (left == invalid_off && right == invalid_off)
		{
			if (p.sz == 0)
			{
				free_node(p);
				if (p.father == invalid_off)
				{
					head = tail = root = invalid_off;
					save_info();
				}
			}
			save_node(p);
			buf_save_t(b, p);
			return 0;
		}
		if (left == invalid_off)
		{
			node r = read_node(right);
			if (r.sz > tnode_max / 2)
			{
				_balance_right_t(b, p, r);
				return 0;
			}
			else
			{
				_merge_right_t(b, p, r);
				return 1;
			}
		}
		if (right == invalid_off)
		{
			node l = read_node(left);
			if (l.sz > tnode_max / 2)
			{
				_balance_left_t(b, p, l);
				return 0;
			}
			else
			{
				_merge_left_t(b, p, l);
				return -1;
			}
		}
		node l = read_node(left), r = read_node(right);

		if (l.sz > tnode_max / 2)
		{
			_balance_left_t(b, p, l);
			return 0;
		}
		else if (r.sz > tnode_max / 2)
		{

			_balance_right_t(b, p, r);
			return 0;
		}
		else
		{
			_merge_left_t(b, p, l);
			return -1;
		}
	}

	void _balance_left_t(buffer_p b, node &p, node &l)
	{
		buffer_t bl;
		buf_load_t(bl, l);
		int i;
		for (i = p.sz; i > 0; --i)
		{
			*nthk_t(b, i) = *nthk_t(b, i - 1);
			*nthc_t(b, i) = *nthc_t(b, i - 1);
		}
		*nthk_t(b, 0) = *nthk_t(bl, l.sz - 1);
		*nthc_t(b, 0) = *nthc_t(bl, l.sz - 1);
		p.key = *nthk_t(b, 0);
		p.sz++;
		l.sz--;
		save_node(p);
		save_node(l);
		buf_save_t(b, p);
		buf_save_t(bl, l);
	}

	void _balance_right_t(buffer_p b, node &p, node &r)
	{

		buffer_t br;
		buf_load_t(br, r);
		p.sz++;
		*nthk_t(b, p.sz - 1) = *nthk_t(br, 0);
		*nthc_t(b, p.sz - 1) = *nthc_t(br, 0);
		int i;
		for (i = 0; i < r.sz - 1; ++i)
		{
			*nthk_t(br, i) = *nthk_t(br, i + 1);
			*nthc_t(br, i) = *nthc_t(br, i + 1);
		}

		r.key = *nthk_t(br, 0);
		r.sz--;
		save_node(p);
		save_node(r);
		buf_save_t(b, p);
		buf_save_t(br, r);
	}

	void _merge_left_t(buffer_p b, node &p, node &l)
	{

		buffer_t bl;
		buf_load_t(bl, l);
		int i;
		for (i = 0; i < p.sz; ++i)
		{
			*nthk_t(bl, i + l.sz) = *nthk_t(b, i);
			*nthc_t(bl, i + l.sz) = *nthc_t(b, i);
		}
		l.sz += p.sz;
		l.next = p.next;
		if (p.next != invalid_off)
		{
			node nn = read_node(p.next);
			nn.prev = l.pos;
			save_node(nn);
		}
		free_node(p);
		save_node(l);
		buf_save_t(bl, l);
	}

	void _merge_right_t(buffer_p b, node &p, node &r)
	{

		buffer_t br;
		buf_load_t(br, r);
		int i;
		for (i = 0; i < r.sz; ++i)
		{
			*nthk_t(b, i + p.sz) = *nthk_t(br, i);
			*nthc_t(b, i + p.sz) = *nthc_t(br, i);
		}
		p.sz += r.sz;
		p.next = r.next;
		if (r.next != invalid_off)
		{
			node nn = read_node(r.next);
			nn.prev = p.pos;
			save_node(nn);
		}
		free_node(r);
		save_node(p);
		buf_save_t(b, p);
	}

	int _remove(node &p, const key_t &k, off_t left, off_t right)
	{
		//-1 merged with left node 0 not merged 1 merged with right node 2 deleted
		buffer_t b;
		if (p.type)
		{
			buf_load_b(b, p);
			buf_remove_b(b, k, p);
			if (p.sz < block_max / 2)
			{
				return _balance_b(b, p, left, right);
			}
			save_node(p);
			buf_save_b(b, p);
			return 0;
		}
		buf_load_t(b, p);
		size_t x = bsearch_t(b, k, p.sz);
		if (x >= p.sz || !equal(*nthk_t(b, x), k))
		{
			if (x > 0)
				--x;
			else
				return 0;
		}
		off_t l = (x > 0) ? (*nthc_t(b, x - 1)) : invalid_off;
		off_t r = (x < p.sz - 1) ? (*nthc_t(b, x + 1)) : invalid_off;
		node q = read_node(*nthc_t(b, x));
		int result = _remove(q, k, l, r);
		if (result == -1)
		{
			int i;
			for (i = x; i < p.sz - 1; i++)
			{
				*nthk_t(b, i) = *nthk_t(b, i + 1);
				*nthc_t(b, i) = *nthc_t(b, i + 1);
			}
			p.sz--;
			save_node(p);
			buf_save_t(b, p);
		}
		else if (result == 1)
		{
			int i;
			for (i = x + 1; i < p.sz - 1; i++)
			{
				*nthk_t(b, i) = *nthk_t(b, i + 1);
				*nthc_t(b, i) = *nthc_t(b, i + 1);
			}
			p.sz--;
			*nthk_t(b, x) = q.key;
			if (x == 0)
			{
				p.key = q.key;
			}
			save_node(p);
			buf_save_t(b, p);
		}
		else if (result == 2)
		{
			free_node(p);
			head = tail = root = invalid_off;
			save_info();
			return 2;
		}
		else
		{
			if (r != invalid_off)
				*nthk_t(b, x + 1) = read_node(r).key;
			*nthk_t(b, x) = q.key;
			if (x == 0)
			{
				p.key = q.key;
			}
			save_node(p);
			buf_save_t(b, p);
		}

		if (p.sz < tnode_max / 2)
		{
			return _balance_t(b, p, left, right);
		}

		return 0;
	}

	void _search(node &p, array_t &arr, const key_t &key1, const key_t &key2, std::function<bool(const key_t &, const key_t &)> compar)
	{
		//printf("%d %d %d %d\n",p.key, p.pos, key, p.type);
		if (compar(key1, p.key))
		{
			return;
		}
		if (p.type)
		{
			//puts("haha");
			buffer_t b;
			buf_load_b(b, p);
			node pp = p;
			size_t x;

			size_t l = 0, r = p.sz, mid;
			key_t *t;
			while (l < r)
			{
				mid = (l + r) / 2;
				t = nthk_b(b, mid);
				if (compar(*t, key1))
				{
					l = mid + 1;
				}
				else
				{
					r = mid;
				}
			}

			x = l;
			if (x == p.sz)
			{
				if (pp.next == invalid_off)
					return;
				pp = read_node(pp.next);
				buf_load_b(b, pp);
				x = 0;
			}

			while (!compar(*nthk_b(b, x), key1) && compar(*nthk_b(b, x), key2))
			{
				if (!compar(*nthk_b(b, x), key1))
					arr.push_back(pair_t(*nthk_b(b, x), *nthv_b(b, x)));
				++x;
				if (x == pp.sz)
				{
					if (pp.next == invalid_off)
						break;
					pp = read_node(pp.next);
					buf_load_b(b, pp);
					x = 0;
				}
			}
			return;
		}
		buffer_t b;
		buf_load_t(b, p);
		size_t x;

		/* binary search */

		size_t l = 0, r = p.sz - 1, mid;
		key_t *t;
		while (l < r)
		{
			mid = (l + r + 1) / 2;
			t = nthk_t(b, mid);
			if (compar(*t, key1))
			{
				l = mid;
			}
			else
			{
				r = mid - 1;
			}
		}

		x = l;
		node cn = read_node(*nthc_t(b, x));
		//printf("%d %d %d\n", key, *nthk_t(b, x), compar(*nthk_t(b, x), key));
		return _search(cn, arr, key1, key2, compar);
	}

  public:
	bptree(const char *fname, const char *index_fname) : 
														 tnode_max((node_size - sizeof(node)) / (sizeof(key_t) + sizeof(off_t)) - 1),
														 block_max((node_size - sizeof(node)) / (sizeof(key_t) + sizeof(value_t)) - 1)
	{
		file = fopen(fname, "rb+");
		filename = new char[strlen(fname) + 1];
		strcpy(filename, fname);
		index_file = new char[strlen(index_fname) + 1];
		strcpy(index_file, index_fname);
		load_index();
		if (!file)
		{
			file = fopen(fname, "wb+");
			init();
		}
		else
		{
			read_info();
			//printf("%lld\n", root);
		}
	}
	~bptree()
	{
		save_index();
		save_info();
		//printf("##%lld\n", root);
		if (file)
			fclose(file);
		//printf("%lld\n", root);
		delete filename;
		delete index_file;
	}

	void init()
	{
		alloc.clear();
		save_index();
		new_node();
		head = tail = root = invalid_off;
		fseek(file, 0, SEEK_SET);
		save_info();
	}

	inline void load_index()
	{
		alloc.load(index_file);
	}
	inline void save_index()
	{
		alloc.dump(index_file);
	}

	int count(const key_t &key)
	{
		if (root == invalid_off)
		{
			return 0;
		}
		node rn = read_node(root);
		return _count(rn, key);
	}
	value_t find(const key_t &key, const value_t &d = value_t())
	{
		//printf("##%lld\n", root);
		if (root == invalid_off)
		{
			//puts("haha");

			return d;
		}
		node rn = read_node(root);
		return _find(rn, key, d);
	}

	inline bool empty()
	{
		return root == invalid_off;
	}

	void set(const key_t &key, const value_t &v)
	{
		node rn = read_node(root);
		_set(rn, key, v);
	}
	void insert(const key_t &key, const value_t &v)
	{

		if (empty())
		{
			node p = new_tnode(key);
			node q = new_block(key, p.pos);
			root = p.pos;
			head = tail = q.pos;
			save_info();
			//printf("~%lld %lld\n", p.pos, root);
			_insert_b(q, key, v);
			_insert_t(p, key, q.pos);
			return;
		}
		node rn = read_node(root);
		_insert(rn, key, v);
	}
	void remove(const key_t &key)
	{
		node rn = read_node(root);
		_remove(rn, key, invalid_off, invalid_off);
	}

	inline void print_info()
	{
		printf("[node_cnt: %d key size: %d value size: %d]\n", sz, sizeof(key_t), sizeof(value_t));
		printf("[size1: %d | size2: %d]\n", tnode_max, block_max);
		printf("[file: %s | index file: %s]\n", filename, index_file);
	}

	//array_t search(bool (*compar)(const void*, const void*));

	void search(array_t &arr, const key_t &key1, const key_t &key2, std::function<bool(const key_t &, const key_t &)> compar)
	{
		if (empty())
			return;
		node r = read_node(root); // read_node(root) is not a lvalue
		_search(r, arr, key1, key2, compar);
	}

	void traverse()
	{
		off_t p = head;
		node q;
		buffer_t b;
		while (p != invalid_off)
		{
			q = read_node(p);
			buf_load_b(b, q);
			int i;
			for (i = 0; i < q.sz; ++i)
			{
				cout << *nthk_b(b, i) << "\t" << *nthv_b(b, i) << endl;
			}
			p = q.next;
		}
	}
};
