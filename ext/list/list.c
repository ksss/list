#include "ruby.h"
#include "ruby/encoding.h"

#define LIST_VERSION "0.2.0"

VALUE cList;

ID id_cmp, id_each, id_to_list;

typedef struct item_t {
	VALUE value;
	struct item_t *next;
} item_t;

typedef struct {
	item_t *first;
	item_t *last;
	union {
		long len;
		VALUE shared;
	} aux;
} list_t;

static VALUE list_push_ary(VALUE, VALUE);
static VALUE list_push(VALUE, VALUE);
static VALUE list_unshift(VALUE, VALUE);
static VALUE list_replace(VALUE, VALUE);
static VALUE list_length(VALUE);

#define DEBUG 0

#define LIST_MAX_SIZE ULONG_MAX
#define LIST_PTR(list) ((list_t*)DATA_PTR(list))
#define LIST_PTR_LEN(ptr) (ptr)->aux.len
#define LIST_LEN(list) LIST_PTR(list)->aux.len

#define LIST_FOR(self, c) for (c = LIST_PTR(self)->first; c; c = (c)->next)

#define LIST_FOR_DOUBLE(l1, c1, l2, c2, code) do { \
	c1 = LIST_PTR(l1)->first; \
	c2 = LIST_PTR(l2)->first; \
	while ((c1) && (c2)) { \
		code; \
		c1 = (c1)->next; \
		c2 = (c2)->next; \
	} \
} while (0)

#ifndef FALSE
#  define FALSE 0
#endif

#ifndef TRUE
#  define TRUE 1
#endif

static VALUE
list_enum_length(VALUE self, VALUE args, VALUE eobj)
{
	return LONG2NUM(LIST_LEN(self));
}
static VALUE
list_cycle_size(VALUE self, VALUE args, VALUE eobj)
{
	VALUE n = Qnil;
	long mul;

	if (args && (0 < RARRAY_LEN(args))) {
		n = rb_ary_entry(args, 0);
	}
	if (LIST_LEN(self) == 0) return INT2FIX(0);
	if (n == Qnil) return DBL2NUM(INFINITY);
	mul = NUM2LONG(n);
	if (mul <= 0) return INT2FIX(0);
	return rb_funcall(list_length(self), '*', 1, LONG2FIX(mul));
}

/* from intern.h */
#ifndef RBASIC_CLEAR_CLASS
#  define RBASIC_CLEAR_CLASS(obj) (((struct RBasicRaw *)((VALUE)(obj)))->klass = 0)
struct RBasicRaw {
	VALUE flags;
	VALUE klass;
};
#endif

enum list_take_pos_flags {
	LIST_TAKE_FIRST,
	LIST_TAKE_LAST
};

#if DEBUG
static void
p(const char *str)
{
	rb_p(rb_str_new2(str));
}

static void
check_print(list_t *ptr, const char *msg, long lineno)
{
	printf("===ERROR(%s)", msg);
	printf("lineno:%ld===\n", lineno);
	printf("ptr:%p\n",ptr);
	printf("LIST_LEN(self):%ld\n",LIST_LEN(self));
	printf("ptr->first:%p\n",ptr->first);
	printf("ptr->last:%p\n",ptr->last);
	rb_raise(rb_eRuntimeError, "check is NG!");
}

static void
check(list_t *ptr, const char *msg)
{
	item_t *end;
	item_t *c, *b;
	long len, i;

	if (LIST_PTR_LEN(ptr) == 0 && ptr->first != NULL) check_print(ptr, msg, __LINE__);
	if (LIST_PTR_LEN(ptr) != 0 && ptr->first == NULL) check_print(ptr, msg, __LINE__);
	if (LIST_PTR_LEN(ptr) == 0 && ptr->last != NULL)  check_print(ptr, msg, __LINE__);
	if (LIST_PTR_LEN(ptr) != 0 && ptr->last == NULL)  check_print(ptr, msg, __LINE__);
	if (ptr->first == NULL && ptr->last != NULL) check_print(ptr, msg, __LINE__);
	if (ptr->first != NULL && ptr->last == NULL) check_print(ptr, msg, __LINE__);
	len = LIST_LEN(self);
	if (len == 0) return;
	i = 0;
	end = ptr->last->next;
	i++;
	for (c = ptr->first->next; c != end; c = c->next) {
		i++;
		b = c;
	}
	if (b != ptr->last) check_print(ptr, msg, __LINE__);
	if (len != i) check_print(ptr, msg, __LINE__);
}
#endif

static inline VALUE
list_new(void)
{
	return rb_obj_alloc(cList);
}

static VALUE
collect_all(VALUE i, VALUE list, int argc, VALUE *argv)
{
	VALUE pack;
	rb_thread_check_ints();
	if (argc == 0) pack = Qnil;
	else if (argc == 1) pack = argv[0];
	else pack = rb_ary_new4(argc, argv);

	list_push(list, pack);
	return Qnil;
}

static VALUE
ary_to_list(int argc, VALUE *argv, VALUE obj)
{
	VALUE list = list_new();
	rb_block_call(obj, id_each, argc, argv, collect_all, list);
	OBJ_INFECT(list, obj);
	return list;
}

static VALUE
to_list(VALUE obj)
{
	VALUE list;

	switch (rb_type(obj)) {
	case T_DATA:
		return obj;
	case T_ARRAY:
		return ary_to_list(0, NULL, obj);
	default:
		if (rb_respond_to(obj, id_to_list)) {
			return rb_funcall(obj, id_to_list, 0);
		}
	}

	list = list_new();
	return list_push(list, obj);
}

static inline void
list_modify_check(VALUE self)
{
	rb_check_frozen(self);
}

static void
list_mark(list_t *ptr)
{
	item_t *c;
	item_t *end;

	if (ptr->first == NULL) return;
	end = ptr->last->next;
	rb_gc_mark(ptr->first->value);
	for (c = ptr->first->next; c != end; c = c->next) {
		rb_gc_mark(c->value);
	}
}

static void
list_free(list_t *ptr)
{
	item_t *c;
	item_t *first_next;
	item_t *next;
	item_t *end;

	if (ptr->first == NULL) return;
	first_next = ptr->first->next;
	end = ptr->last->next;
	xfree(ptr->first);
	for (c = first_next; c != end;) {
		next = c->next;
		xfree(c);
		c = next;
	}
}

static void
list_mem_clear(VALUE self, long beg, long len)
{
	long i;
	list_t *ptr;
	item_t *c;
	item_t *next;
	item_t *mid, *last;

	ptr = LIST_PTR(self);
	if (beg == 0 && len == LIST_LEN(self)) {
		list_free(ptr);
		ptr->first = NULL;
		ptr->last = NULL;
		LIST_LEN(self) = 0;
		return;
	}

	i = -1;
	if (beg == 0) {
		for (c = ptr->first; c;) {
			i++;
			if (i < len) { // mid
				next = c->next;
				xfree(c);
				c = next;
			}
			if (len == i) { // end
				ptr->first = c;
				break;
			}
		}
	} else if (beg + len == LIST_LEN(self)) {
		for (c = ptr->first; c;) {
			i++;
			next = c->next;
			if (beg - 1 == i) { // begin
				last = c;
			} else if (beg <= i) { // mid
				xfree(c);
			}
			c = next;
		}
		ptr->last = last;
		ptr->last->next = NULL;
	} else {
		for (c = ptr->first; c;) {
			i++;
			if (beg == i) { // begin
				mid = next;
			}
			if (beg + len == i) { // end
				mid->next = c;
				break;
			}
			if (beg <= i) { // mid
				next = c->next;
				xfree(c);
				c = next;
			}
		}
	}

	LIST_LEN(self) -= len;
}

static inline list_t *
list_new_ptr(void)
{
	list_t *ptr = ALLOC(list_t);
	ptr->first = NULL;
	ptr->last = ptr->first;
	LIST_PTR_LEN(ptr) = 0;
	return ptr;
}

static VALUE
list_alloc(VALUE self)
{
	list_t *ptr = list_new_ptr();
	return Data_Wrap_Struct(self, list_mark, list_free, ptr);
}

static item_t *
item_alloc(VALUE obj, item_t *next)
{
	item_t *item;
	item = xmalloc(sizeof(item_t));
	item->value = obj;
	item->next = next;
	return item;
}

static VALUE
list_push(VALUE self, VALUE obj)
{
	list_t *ptr;
	item_t *next;

	list_modify_check(self);
	if (self == obj) {
		rb_raise(rb_eArgError, "`List' cannot set recursive");
	}

	next = item_alloc(obj, NULL);

	Data_Get_Struct(self, list_t, ptr);
	if (ptr->first == NULL) {
		ptr->first = next;
		ptr->last = next;
		ptr->last->next = NULL;
	} else {
		ptr->last->next = next;
		ptr->last = next;
	}
	LIST_LEN(self)++;
	return self;
}

static VALUE
list_push_ary(VALUE self, VALUE ary)
{
	long i;

	list_modify_check(self);
	for (i = 0; i < RARRAY_LEN(ary); i++) {
		list_push(self, rb_ary_entry(ary, i));
	}
	return self;
}

static VALUE
list_push_m(int argc, VALUE *argv, VALUE self)
{
	long i;

	list_modify_check(self);
	if (argc == 0) return self;
	for (i = 0; i < argc; i++) {
		list_push(self, argv[i]);
	}
	return self;
}

static VALUE
list_s_create(int argc, VALUE *argv, VALUE klass)
{
	VALUE list;

	list = rb_obj_alloc(klass);
	return list_push_m(argc, argv, list);
}

static VALUE
check_list_type(VALUE obj)
{
	if (TYPE(obj) == T_DATA) return obj;
	return rb_check_convert_type(obj, T_DATA, "List", "to_list");
}

static VALUE
list_s_try_convert(VALUE dummy, VALUE obj)
{
	return check_list_type(obj);
}

static VALUE
list_initialize(int argc, VALUE *argv, VALUE self)
{
	VALUE size, val;
	long len;
	long i;

	list_modify_check(self);
	if (argc == 0) {
		return self;
	}
	if (argc == 1 && !FIXNUM_P(argv[0])) {
		switch (rb_type(argv[0])) {
		case T_ARRAY:
			return list_push_ary(self, argv[0]);
		case T_DATA:
			return list_replace(self, argv[0]);
		default:
			break;
		}
	}

	rb_scan_args(argc, argv, "02", &size, &val);

	len = NUM2LONG(size);
	if (len < 0) {
		rb_raise(rb_eArgError, "negative size");
	}
	if (LIST_MAX_SIZE < len) {
		rb_raise(rb_eArgError, "size too big");
	}

	if (rb_block_given_p()) {
		if (argc == 2) {
			rb_warn("block supersedes default value argument");
		}
		for (i = 0; i < len; i++) {
			list_push(self, rb_yield(LONG2NUM(i)));
		}
	} else {
		for (i = 0; i < len; i++) {
			list_push(self, val);
		}
	}
	return self;
}

static VALUE
list_each(VALUE self)
{
	item_t *c;

	RETURN_SIZED_ENUMERATOR(self, 0, 0, list_enum_length);

	LIST_FOR(self, c) {
		rb_yield(c->value);
	}
	return self;
}

static VALUE
list_each_index(VALUE self)
{
	item_t *c;
	long index;

	RETURN_SIZED_ENUMERATOR(self, 0, 0, list_enum_length);

	index = 0;
	LIST_FOR(self, c) {
		rb_yield(LONG2NUM(index++));
	}
	return self;
}

static VALUE
list_clear(VALUE self)
{
	list_modify_check(self);
	list_t *ptr;
	Data_Get_Struct(self, list_t, ptr);
	list_free(ptr);
	ptr->first = NULL;
	ptr->last = NULL;
	LIST_LEN(self) = 0;
	return self;
}

static VALUE
list_replace_ary(VALUE copy, VALUE orig)
{
	item_t *c_copy;
	long i, olen;

	list_modify_check(copy);
	if (copy == orig) return copy;

	olen = RARRAY_LEN(orig);
	if (olen == 0) {
		return list_clear(copy);
	}
	if (olen == LIST_LEN(copy)) {
		i = 0;
		LIST_FOR(copy, c_copy) {
			c_copy->value = rb_ary_entry(orig, i);
			i++;
		}
	} else {
		list_clear(copy);
		for (i = 0; i < olen; i++) {
			list_push(copy, rb_ary_entry(orig, i));
		}
	}
	return copy;
}

static VALUE
list_replace(VALUE copy, VALUE orig)
{
	item_t *c_orig;
	item_t *c_copy;
	long olen;

	list_modify_check(copy);
	if (copy == orig) return copy;

	switch (rb_type(orig)) {
	case T_ARRAY:
		return list_replace_ary(copy, orig);
	case T_DATA:
		break;
	default:
		rb_raise(rb_eTypeError, "cannot convert to list");
	}
	orig = to_list(orig);
	olen = LIST_LEN(orig);
	if (olen == 0) {
		return list_clear(copy);
	}
	if (olen == LIST_LEN(copy)) {
		LIST_FOR_DOUBLE(orig, c_orig, copy, c_copy, {
			c_copy->value = c_orig->value;
		});
	} else {
		list_clear(copy);
		LIST_FOR(orig, c_orig) {
			list_push(copy, c_orig->value);
		}
	}

	return copy;
}

static VALUE
inspect_list(VALUE self, VALUE dummy, int recur)
{
	VALUE str, s;
	item_t *c;

	if (recur) return rb_usascii_str_new_cstr("[...]");

	str = rb_str_buf_new2("#<");
	rb_str_buf_cat2(str, rb_obj_classname(self));
	rb_str_buf_cat2(str, ": [");
	LIST_FOR(self, c) {
		s = rb_inspect(c->value);
		if (LIST_PTR(self)->first == c) rb_enc_copy(str, s);
		else rb_str_buf_cat2(str, ", ");
		rb_str_buf_append(str, s);
	}
	rb_str_buf_cat2(str, "]>");
	return str;
}

static VALUE
list_inspect(VALUE self)
{
	if (LIST_LEN(self) == 0)
		return rb_sprintf("#<%s: []>", rb_obj_classname(self));
	return rb_exec_recursive(inspect_list, self, 0);
}

static VALUE
list_to_a(VALUE self)
{
	item_t *c;
	VALUE ary;
	long i = 0;

	ary = rb_ary_new2(LIST_LEN(self));
	LIST_FOR(self, c) {
		rb_ary_store(ary, i++, c->value);
	}
	return ary;
}

static VALUE
list_frozen_p(VALUE self)
{
	if (OBJ_FROZEN(self)) return Qtrue;
	return Qfalse;
}

static VALUE
recursive_equal(VALUE list1, VALUE list2, int recur)
{
	item_t *c1, *c2;

	if (recur) return Qtrue;

	if (LIST_LEN(list1) != LIST_LEN(list2)) return Qfalse;

	LIST_FOR_DOUBLE(list1, c1, list2, c2, {
		if (c1->value != c2->value) {
			if (!rb_equal(c1->value, c2->value)) {
				return Qfalse;
			}
		}
	});
	return Qtrue;
}

static VALUE
list_delegate_rb(int argc, VALUE *argv, VALUE list, ID id)
{
	VALUE result;
	result = rb_funcall2(list_to_a(list), id, argc, argv);
	switch (rb_type(result)) {
	case T_DATA:
	case T_ARRAY:
		return to_list(result);
	default:
		return result;
	}
}


static VALUE
list_equal(VALUE self, VALUE obj)
{
	if (self == obj)
		return Qtrue;

	if (!rb_obj_is_kind_of(obj, cList)) {
		if (rb_type(obj) == T_ARRAY) {
			return Qfalse;
		}
		if (!rb_respond_to(obj, id_to_list)) {
			return Qfalse;
		}
		return rb_equal(obj, self);
	}
	return rb_exec_recursive_paired(recursive_equal, self, obj, obj);

}

static VALUE
list_hash(VALUE self)
{
	item_t *c;
	st_index_t h;
	VALUE n;

	h = rb_hash_start(LIST_LEN(self));
	h = rb_hash_uint(h, (st_index_t)list_hash);
	LIST_FOR(self, c) {
		n = rb_hash(c->value);
		h = rb_hash_uint(h, NUM2LONG(n));
	}
	h = rb_hash_end(h);
	return LONG2FIX(h);
}

static VALUE
list_elt(VALUE self, long offset)
{
	long i;
	long len;
	item_t *c;

	len = LIST_LEN(self);
	if (len == 0) return Qnil;
	if (offset < 0 || len <= offset) {
		return Qnil;
	}

	i = 0;
	LIST_FOR(self, c) {
		if (i++ == offset) {
			return c->value;
		}
	}
	return Qnil;
}

static VALUE
list_entry(VALUE self, long offset)
{
	list_t *ptr;

	Data_Get_Struct(self, list_t, ptr);
	if (offset < 0) {
		offset += LIST_LEN(self);
	}
	return list_elt(self, offset);
}

static VALUE
list_make_partial(VALUE self, VALUE klass, long offset, long len)
{
	VALUE instance;
	item_t *c;
	long i;

	instance = rb_obj_alloc(klass);
	i = -1;
	LIST_FOR(self, c) {
		i++;
		if (i < offset) continue;

		if (i < offset + len) {
			list_push(instance, c->value);
		} else {
			break;
		}
	}
	return instance;
}

static VALUE
list_subseq(VALUE self, long beg, long len)
{
	long alen;
	list_t *ptr;

	Data_Get_Struct(self, list_t, ptr);
	alen = LIST_LEN(self);

	if (alen < beg) return Qnil;
	if (beg < 0 || len < 0) return Qnil;

	if (alen < len || alen < beg + len) {
		len = alen - beg;
	}
	if (len == 0) {
		return list_new();
	}

	return list_make_partial(self, cList, beg, len);
}

static VALUE
list_aref(int argc, VALUE *argv, VALUE self)
{
	VALUE arg;
	long beg, len;
	list_t *ptr;

	Data_Get_Struct(self, list_t, ptr);
	if (argc == 2) {
		beg = NUM2LONG(argv[0]);
		len = NUM2LONG(argv[1]);
		if (beg < 0) {
			beg += LIST_LEN(self);
		}
		return list_subseq(self, beg, len);
	}
	if (argc != 1) {
		rb_scan_args(argc, argv, "11", NULL, NULL);
	}
	arg = argv[0];

	/* special case - speeding up */
	if (FIXNUM_P(arg)) {
		return list_entry(self, FIX2LONG(arg));
	}
	/* check if idx is Range */
	switch (rb_range_beg_len(arg, &beg, &len, LIST_LEN(self), 0)) {
	case Qfalse:
		break;
	case Qnil:
		return Qnil;
	default:
		return list_subseq(self, beg, len);
	}
	return list_entry(self, NUM2LONG(arg));
}

static void
list_splice(VALUE self, long beg, long len, VALUE rpl)
{
	long i;
	long rlen, olen, alen;
	item_t *c = NULL;
	item_t *item_first = NULL, *item_last = NULL, *first = NULL, *last = NULL;

	if (len < 0)
		rb_raise(rb_eIndexError, "negative length (%ld)", len);

	olen = LIST_LEN(self);
	if (beg < 0) {
		beg += olen;
		if (beg < 0) {
			rb_raise(rb_eIndexError, "index %ld too small for array; minimum: %ld",
					beg - olen, -olen);
		}
	}
	if (olen < len || olen < beg + len) {
		len = olen - beg;
	}

	if (rpl == Qundef) {
		rlen = 0;
	} else {
		rpl = rb_ary_to_ary(rpl);
		rlen = RARRAY_LEN(rpl);
		olen = LIST_LEN(self);
	}
	if (olen <= beg) {
		if (LIST_MAX_SIZE - rlen < beg) {
			rb_raise(rb_eIndexError, "index %ld too big", beg);
		}
		for (i = LIST_LEN(self); i < beg; i++) {
			list_push(self, Qnil);
		}
		list_push_ary(self, rpl);
	} else {
		alen = olen + rlen - len;
		if (len != rlen) {
			for (i = rlen; 0 < i; i--) {
				c = item_alloc(rb_ary_entry(rpl, i - 1), c);
				if (i == rlen) {
					item_last = c;
				}
			}
			item_first = c;

			i = -1;
			LIST_FOR(self, c) {
				i++;
				if (i == beg - 1) {
					first = c;
				}
				if (i == beg + len) {
					last = c;
					break;
				}
			}
			if (beg == 0) {
				LIST_PTR(self)->first = item_first;
			} else {
				first->next = item_first;
			}
			if (rlen == 0) {
				LIST_PTR(self)->last = first;
				LIST_PTR(self)->last->next = NULL;
			} else {
				item_last->next = last;
			}
			LIST_LEN(self) += rlen - len;
		} else {
			i = -1;
			LIST_FOR(self, c) {
				i++;
				if (beg <= i && i < beg + rlen) {
					c->value = rb_ary_entry(rpl, i - beg);
				}
			}
		}

	}
}

static void
list_store(VALUE self, long idx, VALUE val)
{
	item_t *c;
	long len, i;

	len = LIST_LEN(self);

	if (idx < 0) {
		idx += len;
		if (idx < 0) {
			rb_raise(rb_eIndexError, "index %ld too small for list; minimum: %ld",
					idx - len, -len);
		}
	} else if (LIST_MAX_SIZE <= idx) {
		rb_raise(rb_eIndexError, "index %ld too big", idx);
	}

	if (LIST_LEN(self) <= idx) {
		for (i = LIST_LEN(self); i <= idx; i++) {
			list_push(self, Qnil);
		}
	}

	i = -1;
	LIST_FOR(self, c) {
		i++;
		if (i == idx) {
			c->value = val;
			break;
		}
	}
}

static VALUE
list_aset(int argc, VALUE *argv, VALUE self)
{
	long offset, beg, len;
	list_t *ptr;

	Data_Get_Struct(self, list_t, ptr);
	if (argc == 3) {
		list_modify_check(self);
		beg = NUM2LONG(argv[0]);
		len = NUM2LONG(argv[1]);
		list_splice(self, beg, len, argv[2]);
		return argv[2];
	}
	rb_check_arity(argc, 2, 2);
	list_modify_check(self);
	if (FIXNUM_P(argv[0])) {
		offset = FIX2LONG(argv[0]);
		goto fixnum;
	}
	if (rb_range_beg_len(argv[0], &beg, &len, LIST_LEN(self), 1)) {
		/* check if idx is Range */
		list_splice(self, beg, len, argv[1]);
		return argv[1];
	}

	offset = NUM2LONG(argv[0]);
fixnum:
	list_store(self, offset, argv[1]);
	return argv[1];
}

static VALUE
list_at(VALUE self, VALUE pos)
{
	return list_entry(self, NUM2LONG(pos));
}

static VALUE
list_fetch(int argc, VALUE *argv, VALUE self)
{
	VALUE pos, ifnone;
	long block_given;
	long idx;
	list_t *ptr;

	Data_Get_Struct(self, list_t, ptr);
	rb_scan_args(argc, argv, "11", &pos, &ifnone);
	block_given = rb_block_given_p();
	if (block_given && argc == 2) {
		rb_warn("block supersedes default value argument");
	}
	idx = NUM2LONG(pos);

	if (idx < 0) {
		idx += LIST_LEN(self);
	}
	if (idx < 0 || LIST_LEN(self) <= idx) {
		if (block_given) return rb_yield(pos);
		if (argc == 1) {
			rb_raise(rb_eIndexError, "index %ld outside of array bounds: %ld...%ld",
					idx - (idx < 0 ? LIST_LEN(self) : 0), -LIST_LEN(self), LIST_LEN(self));
		}
		return ifnone;
	}
	return list_entry(self, idx);
}

static VALUE
list_take_first_or_last(int argc, VALUE *argv, VALUE self, enum list_take_pos_flags flag)
{
	VALUE nv;
	long n;
	long len;
	long offset = 0;
	list_t *ptr;

	Data_Get_Struct(self, list_t, ptr);
	rb_scan_args(argc, argv, "1", &nv);
	n = NUM2LONG(nv);
	len = LIST_LEN(self);
	if (n > len) {
		n = len;
	} else if (n < 0) {
		rb_raise(rb_eArgError, "negative array size");
	}
	if (flag == LIST_TAKE_LAST) {
		offset = len - n;
	}
	return list_make_partial(self, cList, offset, n);
}

static VALUE
list_first(int argc, VALUE *argv, VALUE self)
{
	list_t *ptr;
	Data_Get_Struct(self, list_t, ptr);
	if (argc == 0) {
		if (ptr->first == NULL) return Qnil;
		return ptr->first->value;
	} else {
		return list_take_first_or_last(argc, argv, self, LIST_TAKE_FIRST);
	}
}

static VALUE
list_last(int argc, VALUE *argv, VALUE self)
{
	list_t *ptr;
	long len;

	Data_Get_Struct(self, list_t, ptr);
	if (argc == 0) {
		len = LIST_LEN(self);
		if (len == 0) return Qnil;
		return list_elt(self, len - 1);
	} else {
		return list_take_first_or_last(argc, argv, self, LIST_TAKE_LAST);
	}
}

static VALUE
list_concat(VALUE self, VALUE obj)
{
	long len;
	enum ruby_value_type type;

	list_modify_check(self);
	type = rb_type(obj);
	if (type == T_DATA) {
		len = LIST_LEN(obj);
	} else if (type == T_ARRAY) {
		len = RARRAY_LEN(obj);
	} else {
		obj = to_list(obj);
		len = LIST_LEN(obj);
	}

	if (0 < len) {
		list_splice(self, LIST_LEN(self), 0, obj);
	}
	return self;
}

static VALUE
list_pop(VALUE self)
{
	VALUE result;

	list_modify_check(self);
	if (LIST_LEN(self) == 0) return Qnil;
	result = LIST_PTR(self)->last->value;
	list_mem_clear(self, LIST_LEN(self) - 1, 1);
	return result;
}

static VALUE
list_pop_m(int argc, VALUE *argv, VALUE self)
{
	VALUE result;
	long n;

	if (argc == 0) {
		return list_pop(self);
	}

	list_modify_check(self);
	result = list_take_first_or_last(argc, argv, self, LIST_TAKE_LAST);
	n = NUM2LONG(argv[0]);
	list_mem_clear(self, LIST_LEN(self) - n, n);
	return result;
}

static VALUE
list_shift(VALUE self)
{
	VALUE result;

	if (LIST_LEN(self) == 0) return Qnil;
	result = list_first(0, NULL, self);
	list_mem_clear(self, 0, 1);
	return result;
}

static VALUE
list_shift_m(int argc, VALUE *argv, VALUE self)
{
	VALUE result;
	long i;
	list_t *ptr;

	if (argc == 0) {
		return list_shift(self);
	}

	list_modify_check(self);
	result = list_take_first_or_last(argc, argv, self, LIST_TAKE_FIRST);
	Data_Get_Struct(result, list_t, ptr);
	for (i = 0; i < LIST_LEN(self); i++) {
		list_shift(self);
	}
	return result;
}

static VALUE
list_unshift(VALUE self, VALUE obj)
{
	list_t *ptr;
	item_t *first;
	Data_Get_Struct(self, list_t, ptr);

	first = item_alloc(obj, ptr->first);
	if (ptr->first == NULL) {
		ptr->first = first;
		ptr->last = first;
		ptr->last->next = NULL;
	} else {
		ptr->first = first;
	}
	LIST_LEN(self)++;
	return self;
}

static VALUE
list_unshift_m(int argc, VALUE *argv, VALUE self)
{
	long i;

	list_modify_check(self);
	if (argc == 0) return self;
	for (i = argc - 1; 0 <= i; i--) {
		list_unshift(self, argv[i]);
	}
	return self;
}

static VALUE
list_insert(int argc, VALUE *argv, VALUE self)
{
	list_t *ptr;
	long pos;

	Data_Get_Struct(self, list_t, ptr);
	rb_check_arity(argc, 1, UNLIMITED_ARGUMENTS);
	list_modify_check(self);
	if (argc == 1) return self;
	pos = NUM2LONG(argv[0]);
	if (pos == -1) {
		pos = LIST_LEN(self);
	}
	if (pos < 0) {
		pos++;
	}
	list_splice(self, pos, 0, rb_ary_new4(argc - 1, argv + 1));
	return self;
}

static VALUE
list_length(VALUE self)
{
	list_t *ptr;
	Data_Get_Struct(self, list_t, ptr);
	return LONG2NUM(LIST_LEN(self));
}

static VALUE
list_empty_p(VALUE self)
{
	list_t *ptr;
	Data_Get_Struct(self, list_t, ptr);
	if (LIST_LEN(self) == 0)
		return Qtrue;
	return Qfalse;
}

static VALUE
list_rindex(int argc, VALUE *argv, VALUE self)
{
	long i;
	long len;
	VALUE val;

	i = LIST_LEN(self);
	if (argc == 0) {
		RETURN_ENUMERATOR(self, 0, 0);
		while (i--) {
			if (RTEST(rb_yield(list_elt(self, i))))
				return LONG2NUM(i);
			if (LIST_LEN(self) < i) {
				i = LIST_LEN(self);
			}
		}
		return Qnil;
	}
	rb_check_arity(argc, 0, 1);
	val = argv[0];
	if (rb_block_given_p())
		rb_warn("given block not used");
	while (i--) {
		if (rb_equal(list_elt(self, i), val)) {
			return LONG2NUM(i);
		}
		len = LIST_LEN(self);
		if (len < i) {
			i = len;
		}
	}
	return Qnil;
}

extern VALUE rb_output_fs;

static void list_join_1(VALUE obj, VALUE ary, VALUE sep, long i, VALUE result, int *first);

static VALUE
recursive_join(VALUE obj, VALUE argp, int recur)
{
	VALUE *arg = (VALUE *)argp;
	VALUE list = arg[0];
	VALUE sep = arg[1];
	VALUE result = arg[2];
	int *first = (int *)arg[3];

	if (recur) {
		rb_raise(rb_eArgError, "recursive list join");
	} else {
		list_join_1(obj, list, sep, 0, result, first);
	}
	return Qnil;
}

/* only string join */
static void
list_join_0(VALUE self, VALUE sep, long max, VALUE result)
{
	list_t *ptr;
	item_t *c;
	long i = 1;

	Data_Get_Struct(self, list_t, ptr);
	if (0 < max) rb_enc_copy(result, list_elt(self, 0));
	if (max <= i) return;
	c = ptr->first;
	rb_str_buf_append(result, c->value);
	for (c = c->next; i < max; c = c->next) {
		if (!NIL_P(sep))
			rb_str_buf_append(result, sep);
		rb_str_buf_append(result, c->value);
		i++;
	}
}

/* another object join */
static void
list_join_1(VALUE obj, VALUE list, VALUE sep, long i, VALUE result, int *first)
{
	item_t *c;
	VALUE val, tmp;

	if (LIST_LEN(list) == 0) return;

	LIST_FOR(list, c) {
		val = c->value;
		if (0 < i++ && !NIL_P(sep))
			rb_str_buf_append(result, sep);

		switch (rb_type(val)) {
		case T_STRING:
			str_join:
			rb_str_buf_append(result, val);
			*first = FALSE;
			break;
		case T_ARRAY:
			val = to_list(val);
		case T_DATA:
			obj = val;
			list_join:
			if (val == list) {
				rb_raise(rb_eArgError, "recursive list join");
			}
			VALUE args[4];
			args[0] = val;
			args[1] = sep;
			args[2] = result;
			args[3] = (VALUE)first;
			rb_exec_recursive(recursive_join, obj, (VALUE)args);
			break;
		default:
			tmp = rb_check_string_type(val);
			if (!NIL_P(tmp)) {
				val = tmp;
				goto str_join;
			}
			tmp = rb_check_convert_type(val, T_DATA, "List", "to_list");
			if (!NIL_P(tmp)) {
				obj = val;
				val = tmp;
				goto list_join;
			}
			val = rb_obj_as_string(val);
			if (*first) {
				rb_enc_copy(result, val);
				*first = FALSE;
			}
			goto str_join;
		}
	}
}

static VALUE
list_join(VALUE self, VALUE sep)
{
	long len = 1;
	long i = 0;
	VALUE val, tmp;
	VALUE result;
	item_t *c;
	int first;

	if (LIST_LEN(self) == 0) return rb_usascii_str_new(0, 0);

	if (!NIL_P(sep)) {
		StringValue(sep);
		len += RSTRING_LEN(sep) * (LIST_LEN(self) - 1);
	}
	LIST_FOR(self, c) {
		val = c->value;
		tmp = rb_check_string_type(val);
		if (NIL_P(val) || c->value != tmp) {
			result = rb_str_buf_new(len + LIST_LEN(self));
			rb_enc_associate(result, rb_usascii_encoding());
			list_join_0(self, sep, i, result);
			first = i == 0;
			list_join_1(self, self, sep, i, result, &first);
			return result;
		}
		len += RSTRING_LEN(val);
	}
	result = rb_str_buf_new(len);
	list_join_0(self, sep, LIST_LEN(self), result);

	return result;
}

static VALUE
list_join_m(int argc, VALUE *argv, VALUE self)
{
	VALUE sep;

	rb_scan_args(argc, argv, "01", &sep);
	if (NIL_P(sep)) sep = rb_output_fs;

	return list_join(self, sep);
}

static VALUE
list_reverse_bang(VALUE self)
{
	VALUE tmp;
	item_t *c;
	long len;

	if (LIST_LEN(self) == 0) return self;
	tmp = list_to_a(self);
	len = LIST_LEN(self);
	LIST_FOR(self, c) {
		c->value = rb_ary_entry(tmp, --len);
	}
	return self;
}

static VALUE
list_reverse_m(VALUE self)
{
	VALUE result;
	item_t *c;

	result = list_new();
	if (LIST_LEN(self) == 0) return result;
	LIST_FOR(self, c) {
		list_unshift(result, c->value);
	}
	return result;
}

static VALUE
list_rotate_bang(int argc, VALUE *argv, VALUE self)
{
	list_t *ptr;
	item_t *c;
	long cnt = 1;
	long i = 0;

	switch (argc) {
	case 1: cnt = NUM2LONG(argv[0]);
	case 0: break;
	default: rb_scan_args(argc, argv, "01", NULL);
	}

	if (LIST_LEN(self) == 0) return self;
	cnt = (cnt < 0) ? (LIST_LEN(self) - (~cnt % LIST_LEN(self)) - 1) : (cnt & LIST_LEN(self));
	Data_Get_Struct(self, list_t, ptr);
	LIST_FOR(self, c) {
		if (cnt == ++i) break;
	}

	ptr->last->next = ptr->first;
	ptr->first = c->next;
	ptr->last = c;
	ptr->last->next = NULL;
	return self;
}

static VALUE
list_rotate_m(int argc, VALUE *argv, VALUE self)
{
	VALUE clone = rb_obj_clone(self);
	return list_rotate_bang(argc, argv, clone);
}

static VALUE
list_sort_bang(VALUE self)
{
	item_t *c;
	long i = 0;

	VALUE ary = list_to_a(self);
	rb_ary_sort_bang(ary);
	LIST_FOR(self, c) {
		c->value = rb_ary_entry(ary, i++);
	}
	return self;
}

static VALUE
list_sort(VALUE self)
{
	VALUE clone = rb_obj_clone(self);
	return list_sort_bang(clone);
}

static VALUE
sort_by_i(VALUE i, VALUE _data, int argc, VALUE *argv)
{
	return rb_yield(i);
}

static VALUE
list_sort_by(VALUE self)
{
	VALUE ary;

	RETURN_SIZED_ENUMERATOR(self, 0, 0, list_enum_length);
	ary = rb_block_call(list_to_a(self), rb_intern("sort_by"), 0, 0, sort_by_i, 0);
	return to_list(ary);
}

static VALUE
list_sort_by_bang(VALUE self)
{
	VALUE sorted;

	sorted = list_sort_by(self);
	return list_replace(self, sorted);
}

static VALUE
list_collect_bang(VALUE self)
{
	item_t *c;

	RETURN_SIZED_ENUMERATOR(self, 0, 0, list_enum_length);
	LIST_FOR(self, c) {
		c->value = rb_yield(c->value);
	}
	return self;
}

static VALUE
list_collect(VALUE self)
{
	return list_collect_bang(rb_obj_clone(self));
}

static VALUE
list_select(VALUE self)
{
	VALUE result;
	item_t *c;

	RETURN_SIZED_ENUMERATOR(self, 0, 0, list_enum_length);
	result = list_new();
	LIST_FOR(self, c) {
		if (RTEST(rb_yield(c->value))) {
			list_push(result, c->value);
		}
	}
	return result;
}

static VALUE
list_select_bang(VALUE self)
{
	VALUE result;
	item_t *c;
	long i = 0;

	RETURN_SIZED_ENUMERATOR(self, 0, 0, list_enum_length);
	result = list_new();
	LIST_FOR(self, c) {
		if (RTEST(rb_yield(c->value))) {
			i++;
			list_push(result, c->value);
		}
	}

	if (i == LIST_LEN(self)) return Qnil;
	return list_replace(self, result);
}

static VALUE
list_keep_if(VALUE self)
{
	RETURN_SIZED_ENUMERATOR(self, 0, 0, list_enum_length);
	list_select_bang(self);
	return self;
}

static VALUE
list_values_at(int argc, VALUE *argv, VALUE self)
{
	VALUE result = list_new();
	long beg, len;
	long i, j;
	list_t *ptr;

	Data_Get_Struct(self, list_t, ptr);
	for (i = 0; i < argc; i++) {
		if (FIXNUM_P(argv[i])) {
			list_push(result, list_entry(self, FIX2LONG(argv[i])));
		} else if (rb_range_beg_len(argv[i], &beg, &len, LIST_LEN(self), 1)) {
			for (j = beg; j < beg + len; j++) {
				if (LIST_LEN(self) < j) {
					list_push(result, Qnil);
				} else {
					list_push(result, list_elt(self, j));
				}
			}
		} else {
			list_push(result, list_entry(self, NUM2LONG(argv[i])));
		}
	}
	return result;
}

static VALUE
list_delete(VALUE self, VALUE item)
{
	list_t *ptr;
	item_t *c, *before = NULL, *next;
	long len;

	list_modify_check(self);
	Data_Get_Struct(self, list_t, ptr);
	if (LIST_LEN(self) == 0) return Qnil;

	len = LIST_LEN(self);
	for (c = ptr->first; c; c = next) {
		next = c->next;
		if (rb_equal(c->value, item)) {
			if (ptr->first == ptr->last) {
				ptr->first = NULL;
				ptr->last = NULL;
			} else if (c == ptr->first) {
				ptr->first = c->next;
			} else if (c == ptr->last) {
				ptr->last = before;
				ptr->last->next = NULL;
			} else {
				before->next = c->next;
			}
			xfree(c);
			LIST_LEN(self)--;
		} else {
			before = c;
		}
	}

	if (LIST_LEN(self) == len) {
		if (rb_block_given_p()) {
			return rb_yield(item);
		}
		return Qnil;
	} else {
		return item;
	}
}

static VALUE
list_delete_at(VALUE self, long pos)
{
	VALUE del;
	list_t *ptr;
	item_t *c, *before = NULL, *next;
	long len, i = 0;

	Data_Get_Struct(self, list_t, ptr);
	len = LIST_LEN(self);
	if (LIST_LEN(self) == 0) return Qnil;
	if (pos < 0) {
		pos += len;
		if (pos < 0) return Qnil;
	}
	list_modify_check(self);

	for (c = ptr->first; c; c = next) {
		next = c->next;
		if (pos == i++) {
			del = c->value;
			if (ptr->first == ptr->last) {
				ptr->first = NULL;
				ptr->last = NULL;
			} else if (c == ptr->first) {
				ptr->first = c->next;
			} else if (c == ptr->last) {
				ptr->last = before;
				ptr->last->next = NULL;
			} else {
				before->next = c->next;
			}
			xfree(c);
			LIST_LEN(self)--;
			break;
		} else {
			before = c;
		}
	}

	if (LIST_LEN(self) == len) {
		return Qnil;
	} else {
		return del;
	}
}

static VALUE
list_delete_at_m(VALUE self, VALUE pos)
{
	return list_delete_at(self, NUM2LONG(pos));
}

static VALUE
reject(VALUE self, VALUE result)
{
	item_t *c;

	LIST_FOR(self, c) {
		if (!RTEST(rb_yield(c->value))) {
			list_push(result, c->value);
		}
	}
	return result;
}

static VALUE
reject_bang(VALUE self)
{
	list_t *ptr;
	item_t *c, *before = NULL, *next;
	long len;

	Data_Get_Struct(self, list_t, ptr);
	len = LIST_LEN(self);
	for (c = ptr->first; c; c = next) {
		next = c->next;
		if (RTEST(rb_yield(c->value))) {
			if (ptr->first == ptr->last) {
				ptr->first = NULL;
				ptr->last = NULL;
			} else if (c == ptr->first) {
				ptr->first = c->next;
			} else if (c == ptr->last) {
				ptr->last = before;
				ptr->last->next = NULL;
			} else {
				before->next = c->next;
			}
			xfree(c);
			LIST_LEN(self)--;
		} else {
			before = c;
		}
	}

	if (LIST_LEN(self) == len) {
		return Qnil;
	} else {
		return self;
	}
}

static VALUE
list_reject_bang(VALUE self)
{
	RETURN_SIZED_ENUMERATOR(self, 0, 0, list_enum_length);
	return reject_bang(self);
}

static VALUE
list_reject(VALUE self)
{
	VALUE rejected_list;

	RETURN_SIZED_ENUMERATOR(self, 0, 0, list_enum_length);
	rejected_list = list_new();
	reject(self, rejected_list);
	return rejected_list;
}

static VALUE
list_delete_if(VALUE self)
{
	RETURN_SIZED_ENUMERATOR(self, 0, 0, list_enum_length);
	list_reject_bang(self);
	return self;
}

static VALUE
list_zip(int argc, VALUE *argv, VALUE self)
{
	VALUE ary;
	long i;

	ary = rb_funcall2(list_to_a(self), rb_intern("zip"), argc, argv);
	if (rb_block_given_p()) {
		for (i = 0; i < RARRAY_LEN(ary); i++) {
			rb_yield(rb_ary_entry(ary, i));
		}
		return Qnil;
	} else {
		return to_list(ary);
	}
}

static VALUE
list_transpose(VALUE self)
{
	return list_delegate_rb(0, NULL, self, rb_intern("transpose"));
}

static VALUE
list_fill(int argc, VALUE *argv, VALUE self)
{
	VALUE item, arg1, arg2;
	long beg = 0, len = 0, end = 0;
	long i;
	int block_p = FALSE;
	item_t *c;

	list_modify_check(self);
	if (rb_block_given_p()) {
		block_p = TRUE;
		rb_scan_args(argc, argv, "02", &arg1, &arg2);
		argc += 1; /* hackish */
	} else {
		rb_scan_args(argc, argv, "12", &item, &arg1, &arg2);
	}
	switch (argc) {
	case 1:
		beg = 0;
		len = LIST_LEN(self);
		break;
	case 2:
		if (rb_range_beg_len(arg1, &beg, &len, LIST_LEN(self), 1)) {
			break;
		}
		/* fall through */
	case 3:
		beg = NIL_P(arg1) ? 0 : NUM2LONG(arg1);
		if (beg < 0) {
			beg = LIST_LEN(self) + beg;
			if (beg < 0) beg = 0;
		}
		len = NIL_P(arg2) ? LIST_LEN(self) - beg : NUM2LONG(arg2);
		break;
	}
	if (len < 0) {
		return self;
	}
	if (LIST_MAX_SIZE <= beg || LIST_MAX_SIZE - beg < len) {
		rb_raise(rb_eArgError, "argument too big");
	}
	end = beg + len;
	if (LIST_LEN(self) < end) {
		for (i = 0; i < end - LIST_LEN(self); i++) {
			list_push(self, Qnil);
		}
	}

	i = -1;
	LIST_FOR(self, c) {
		i++;
		if (i < beg) continue;
		if ((end - 1) < i) break;
		if (block_p) {
			c->value = rb_yield(LONG2NUM(i));
		} else {
			c->value = item;
		}
	}
	return self;
}

static VALUE
list_include_p(VALUE self, VALUE item)
{
	item_t *c;

	LIST_FOR(self, c) {
		if (rb_equal(c->value, item)) {
			return Qtrue;
		}
	}
	return Qfalse;
}

static VALUE
recursive_cmp(VALUE list1, VALUE list2, int recur)
{
	item_t *c1, *c2;
	long len;

	if (recur) return Qundef;
	len = LIST_LEN(list1);
	if (LIST_LEN(list2) < len) {
		len = LIST_LEN(list2);
	}
	LIST_FOR_DOUBLE(list1,c1,list2,c2,{
		VALUE v = rb_funcall2(c1->value, id_cmp, 1, &(c2->value));
		if (v != INT2FIX(0)) {
			return v;
		}
	});
	return Qundef;
}

static VALUE
list_cmp(VALUE list1, VALUE list2)
{
	VALUE v;
	long len;

	if (rb_type(list2) != T_DATA) return Qnil;
	if (list1 == list2) return INT2FIX(0);
	v = rb_exec_recursive_paired(recursive_cmp, list1, list2, list2);
	if (v != Qundef) return v;
	len = LIST_LEN(list1) - LIST_LEN(list2);
	if (len == 0) return INT2FIX(0);
	if (0 < len) return INT2FIX(1);
	return INT2FIX(-1);
}

static VALUE
list_slice_bang(int argc, VALUE *argv, VALUE self)
{
	long pos = 0, len = 0;

	list_modify_check(self);
	if (argc == 1) {
		if (FIXNUM_P(argv[0])) {
			return list_delete_at(self, NUM2LONG(argv[0]));
		} else {
			switch (rb_range_beg_len(argv[0], &pos, &len, LIST_LEN(self), 0)) {
			case Qtrue: /* valid range */
				break;
			case Qnil: /* invalid range */
				return Qnil;
			default: /* not a range */
				return list_delete_at(self, NUM2LONG(argv[0]));
			}
		}
	} else if (argc == 2) {
		pos = NUM2LONG(argv[0]);
		len = NUM2LONG(argv[1]);
	} else {
		rb_scan_args(argc, argv, "11", NULL, NULL);
	}
	if (len < 0) return Qnil;
	if (pos < 0) {
		pos += LIST_LEN(self);
		if (pos < 0) return Qnil;
	} else if (LIST_LEN(self) < pos) {
		return Qnil;
	}
	if (LIST_LEN(self) < pos + len) {
		len = LIST_LEN(self) - pos;
	}
	if (len == 0) return list_new();
	VALUE list2 = list_subseq(self, pos, len);
	list_mem_clear(self, pos, len);
	return list2;
}

static VALUE
list_assoc(VALUE self, VALUE key)
{
	VALUE v;
	item_t *c;

	if (list_empty_p(self)) return Qnil;
	LIST_FOR(self, c) {
		v = check_list_type(c->value);
		if (!NIL_P(v)) {
			if (0 < LIST_LEN(v) && rb_equal(list_first(0,NULL,v), key)) {
				return v;
			}
		}
	}
	return Qnil;
}

static VALUE
list_rassoc(VALUE self, VALUE value)
{
	VALUE v;
	item_t *c;

	if (list_empty_p(self)) return Qnil;
	LIST_FOR(self, c) {
		v = check_list_type(c->value);
		if (!NIL_P(v)) {
			if (1 < LIST_LEN(v) && rb_equal(list_elt(v,1), value)) {
				return v;
			}
		}
	}
	return Qnil;
}

static VALUE
list_plus(VALUE x, VALUE y)
{
	item_t *cx, *cy;
	long len;
	VALUE result;

	Check_Type(y, T_DATA);
	len = LIST_LEN(x) + LIST_LEN(y);

	result = list_new();
	LIST_FOR(x,cx) {
		list_push(result, cx->value);
	}
	LIST_FOR(y,cy) {
		list_push(result, cy->value);
	}
	return result;
}

static VALUE
list_times(VALUE self, VALUE times)
{
	VALUE result;
	VALUE tmp;
	long i, len;
	item_t *c;

	tmp = rb_check_string_type(times);
	if (!NIL_P(tmp)) {
		return list_join(self, tmp);
	}

	len = NUM2LONG(times);
	if (len == 0) {
		return rb_obj_alloc(rb_obj_class(self));
	}
	if (len < 0) {
		rb_raise(rb_eArgError, "negative argument");
	}

	if (LIST_MAX_SIZE / len < LIST_LEN(self)) {
		rb_raise(rb_eArgError, "argument too big");
	}

	result = rb_obj_alloc(rb_obj_class(self));
	if (0 < LIST_LEN(self)) {
		for (i = 0; i < len; i++) {
			LIST_FOR(self, c) {
				list_push(result, c->value);
			}
		}
	}
	return result;
}

static inline VALUE
list_tmp_hash_new(void)
{
	VALUE hash = rb_hash_new();
	RBASIC_CLEAR_CLASS(hash);
	return hash;
}

static VALUE
list_add_hash(VALUE hash, VALUE list)
{
	item_t *c;

	LIST_FOR(list, c) {
		if (rb_hash_lookup2(hash, c->value, Qundef) == Qundef) {
			rb_hash_aset(hash, c->value, c->value);
		}
	}
	return hash;
}

static VALUE
list_make_hash(VALUE list)
{
	VALUE hash = list_tmp_hash_new();
	return list_add_hash(hash, list);
}

static VALUE
list_add_hash_by(VALUE hash, VALUE list)
{
	VALUE v, k;
	item_t *c;

	LIST_FOR(list, c) {
		v = c->value;
		k = rb_yield(v);
		if (rb_hash_lookup2(hash, k, Qundef) == Qundef) {
			rb_hash_aset(hash, k, v);
		}
	}
	return hash;
}

static VALUE
list_make_hash_by(VALUE list)
{
	VALUE hash = list_tmp_hash_new();
	return list_add_hash_by(hash, list);
}

static VALUE
list_diff(VALUE list1, VALUE list2)
{
	VALUE list3;
	volatile VALUE hash;
	item_t *c;

	hash = list_make_hash(to_list(list2));
	list3 = list_new();

	LIST_FOR(list1, c) {
		if (st_lookup(RHASH_TBL(hash), c->value, 0)) continue;
		list_push(list3, c->value);
	}
	return list3;
}

static VALUE
list_and(VALUE list1, VALUE list2)
{
	VALUE list3, hash;
	st_table *table;
	st_data_t vv;
	item_t *c1;

	list2 = to_list(list2);
	list3 = list_new();
	if (LIST_LEN(list2) == 0) return list3;
	hash = list_make_hash(list2);
	table = RHASH_TBL(hash);

	LIST_FOR(list1, c1) {
		vv = (st_data_t) c1->value;
		if (st_delete(table, &vv, 0)) {
			list_push(list3, c1->value);
		}
	}

	return list3;
}

static int
values_i(VALUE key, VALUE value, VALUE list)
{
	list_push(list, value);
	return ST_CONTINUE;
}

static VALUE
list_hash_values(VALUE hash)
{
	VALUE list;

	list = list_new();
	rb_hash_foreach(hash, values_i, list);

	return list;
}

static VALUE
list_or(VALUE list1, VALUE list2)
{
	VALUE hash, list3;
	st_data_t vv;
	item_t *c1, *c2;

	list2 = to_list(list2);
	list3 = list_new();
	hash = list_add_hash(list_make_hash(list1), list2);

	LIST_FOR(list1,c1) {
		vv = (st_data_t)c1->value;
		if (st_delete(RHASH_TBL(hash), &vv, 0)) {
			list_push(list3, c1->value);
		}
	}
	LIST_FOR(list2,c2) {
		vv = (st_data_t)c2->value;
		if (st_delete(RHASH_TBL(hash), &vv, 0)) {
			list_push(list3, c2->value);
		}
	}
	return list3;
}

static VALUE
list_dup(VALUE self)
{
	return list_replace(list_new(), self);
}

static VALUE
list_uniq(VALUE self)
{
	VALUE hash, uniq;

	if (LIST_LEN(self) <= 1)
		return list_dup(self);

	if (rb_block_given_p()) {
		hash = list_make_hash_by(self);
	} else {
		hash = list_make_hash(self);
	}
	uniq = list_hash_values(hash);
	return uniq;
}

static VALUE
list_uniq_bang(VALUE self)
{
	long len;

	list_modify_check(self);
	len = LIST_LEN(self);
	if (len <= 1)
		return Qnil;

	list_replace(self, list_uniq(self));
	if (len == LIST_LEN(self)) {
		return Qnil;
	}

	return self;
}

static VALUE
list_compact_bang(VALUE self)
{
	long len;

	list_modify_check(self);
	len = LIST_LEN(self);
	list_delete(self, Qnil);
	if (len == LIST_LEN(self)) {
		return Qnil;
	}
	return self;
}

static VALUE
list_compact(VALUE self)
{
	self = list_dup(self);
	list_compact_bang(self);
	return self;
}

static VALUE
list_make_shared_copy(VALUE list)
{
	return list_make_partial(list, rb_obj_class(list), 0, LIST_LEN(list));
}

static VALUE
flatten(VALUE list, int level, int *modified)
{
	VALUE stack, result;
	VALUE val;
	list_t *ptr, *pv;
	item_t *c;

	*modified = 0;
	stack = rb_ary_new();
	result = list_new();
	Data_Get_Struct(list, list_t, ptr);
	c = ptr->first;
	while (1) {
		while (c) {
			val = check_list_type(c->value);
			if (NIL_P(val) || (0 <= level && level <= RARRAY_LEN(stack))) {
				list_push(result, c->value);
				c = c->next;
			} else {
				*modified = 1;
				rb_ary_push(stack, (VALUE)c); /* stack address */
				Data_Get_Struct(val, list_t, pv);
				c = pv->first;
			}
		}
		if (RARRAY_LEN(stack) == 0) {
			break;
		}
		c = (item_t *)rb_ary_pop(stack); /* pop address */
		c = c->next;
	}

	return result;
}

static VALUE
list_flatten(int argc, VALUE *argv, VALUE self)
{
	int mod = 0, level = -1;
	VALUE result, lv;

	rb_scan_args(argc, argv, "01", &lv);
	if (!NIL_P(lv)) level = NUM2INT(lv);
	if (level == 0) return list_make_shared_copy(self);

	result = flatten(self, level, &mod);
	OBJ_INFECT(result, self);

	return result;
}

static VALUE
list_flatten_bang(int argc, VALUE *argv, VALUE self)
{
	int mod = 0, level = -1;
	VALUE result, lv;

	rb_scan_args(argc, argv, "01", &lv);
	list_modify_check(self);
	if (!NIL_P(lv)) level = NUM2INT(lv);
	if (level == 0) return Qnil;

	result = flatten(self, level, &mod);
	if (mod == 0) {
		return Qnil;
	}
	list_replace(self, result);
	return self;
}

static VALUE
list_count(int argc, VALUE *argv, VALUE self)
{
	VALUE obj;
	item_t *c;
	long n = 0;

	if (argc == 0) {
		if (!rb_block_given_p()) {
			return list_length(self);
		}
		LIST_FOR(self,c) {
			if (RTEST(rb_yield(c->value))) n++;
		}
	} else {
		rb_scan_args(argc, argv, "1", &obj);
		if (rb_block_given_p()) {
			rb_warn("given block not used");
		}
		LIST_FOR(self,c) {
			if (rb_equal(c->value, obj)) n++;
		}
	}
	return LONG2NUM(n);
}

static VALUE
list_shuffle(int argc, VALUE *argv, VALUE self)
{
	return list_delegate_rb(argc, argv, self, rb_intern("shuffle"));
}

static VALUE
list_shuffle_bang(int argc, VALUE *argv, VALUE self)
{
	return list_replace(self, list_shuffle(argc, argv, self));
}

static VALUE
list_sample(int argc, VALUE *argv, VALUE self)
{
	return list_delegate_rb(argc, argv, self, rb_intern("sample"));
}

static VALUE
list_cycle(int argc, VALUE *argv, VALUE self)
{
	VALUE nv = Qnil;
	item_t *c;
	long n;

	RETURN_SIZED_ENUMERATOR(self, argc, argv, list_cycle_size);
	rb_scan_args(argc, argv, "01", &nv);

	if (NIL_P(nv)) {
		n = -1;
	} else {
		n = NUM2LONG(nv);
		if (n <= 0) return Qnil;
	}

	while (0 < LIST_LEN(self) && (n < 0 || 0 < n--)) {
		LIST_FOR(self, c) {
			rb_yield(c->value);
			if (list_empty_p(self)) break;
		}
	}
	return Qnil;
}

static VALUE
list_permutation(int argc, VALUE *argv, VALUE self)
{
	return list_delegate_rb(argc, argv, self, rb_intern("permutation"));
}

static VALUE
list_combination(VALUE self, VALUE num)
{
	return list_delegate_rb(1, &num, self, rb_intern("combination"));
}

static VALUE
list_repeated_permutation(VALUE self, VALUE num)
{
	return list_delegate_rb(1, &num, self, rb_intern("repeated_permutation"));
}

static VALUE
list_repeated_combination(VALUE self, VALUE num)
{
	return list_delegate_rb(1, &num, self, rb_intern("repeated_combination"));
}

static VALUE
list_product(int argc, VALUE *argv, VALUE self)
{
	return list_delegate_rb(argc, argv, self, rb_intern("product"));
}

static VALUE
list_take(VALUE self, VALUE n)
{
	int len = NUM2LONG(n);

	if (len < 0) {
		rb_raise(rb_eArgError, "attempt to take negative size");
	}
	return list_subseq(self, 0, len);
}

static VALUE
list_take_while(VALUE self)
{
	item_t *c;
	long i = 0;

	RETURN_ENUMERATOR(self, 0, 0);
	LIST_FOR(self, c) {
		if (!RTEST(rb_yield(c->value))) break;
		i++;
	}
	return list_take(self, LONG2FIX(i));
}

static VALUE
list_drop(VALUE self, VALUE n)
{
	VALUE result;
	long pos = NUM2LONG(n);

	if (pos < 0) {
		rb_raise(rb_eArgError, "attempt to drop negative size");
	}
	result = list_subseq(self, pos, LIST_LEN(self));
	if (result == Qnil) result = list_new();
	return result;
}

static VALUE
list_drop_while(VALUE self)
{
	long i = 0;
	item_t *c;

	RETURN_ENUMERATOR(self, 0, 0);
	LIST_FOR(self, c) {
		if (!RTEST(rb_yield(c->value))) break;
		i++;
	}
	return list_drop(self, LONG2FIX(i));
}

/**
 * from CRuby rb_ary_bsearch
 */
static VALUE
list_bsearch(VALUE self)
{
	long low, high, mid;
	int smaller = 0, satisfied = 0;
	VALUE v, val, ary;

	ary = list_to_a(self);
	low = 0;
	high = RARRAY_LEN(ary);

	RETURN_ENUMERATOR(self, 0, 0);
	while (low < high) {
		mid = low + ((high - low) / 2);
		val = rb_ary_entry(ary, mid);
		v = rb_yield(val);
		if (FIXNUM_P(v)) {
			if (FIX2INT(v) == 0) return val;
			smaller = FIX2INT(v) < 0;
		}
		else if (v == Qtrue) {
			satisfied = 1;
			smaller = 1;
		}
		else if (v == Qfalse || v == Qnil) {
			smaller = 0;
		}
		else if (rb_obj_is_kind_of(v, rb_cNumeric)) {
			const VALUE zero = INT2FIX(0);
			switch (rb_cmpint(rb_funcall(v, id_cmp, 1, zero), v, INT2FIX(0))) {
				case 0: return val;
				case 1: smaller = 1; break;
				case -1: smaller = 0;
			}
		}
		else {
			rb_raise(rb_eTypeError, "wrong argument type %s"
					" (must be numeric, true, false or nil)",
					rb_obj_classname(v));
		}
		if (smaller) {
			high = mid;
		}
		else {
			low = mid + 1;
		}
	}
	if (low == RARRAY_LEN(ary)) return Qnil;
	if (!satisfied) return Qnil;
	return rb_ary_entry(ary, low);
}

static VALUE
list_ring_bang(VALUE self)
{
	list_t *ptr;

	Data_Get_Struct(self, list_t, ptr);
	if (ptr->first == NULL)
		rb_raise(rb_eRuntimeError, "length is zero list cannot to change ring");
	rb_obj_freeze(self);
	ptr->last->next = ptr->first;
	return self;
}

static VALUE
list_ring(VALUE self)
{
	VALUE clone = rb_obj_clone(self);
	return list_ring_bang(clone);
}

static VALUE
list_ring_p(VALUE self)
{
	list_t *ptr;

	Data_Get_Struct(self, list_t, ptr);
	if (ptr->first == NULL)
		return Qfalse;
	if (ptr->first == ptr->last->next)
		return Qtrue;
	return Qfalse;
}

static VALUE
list_to_list(VALUE self)
{
	return self;
}

static VALUE
list_pack(VALUE self, VALUE str)
{
	return list_delegate_rb(1, &str, self, rb_intern("pack"));
}

void
Init_list(void)
{
	cList = rb_define_class("List", rb_cObject);
	rb_include_module(cList, rb_mEnumerable);

	rb_define_alloc_func(cList, list_alloc);

	rb_define_singleton_method(cList, "[]", list_s_create, -1);
	rb_define_singleton_method(cList, "try_convert", list_s_try_convert, 1);

	rb_define_method(cList, "initialize", list_initialize, -1);
	rb_define_method(cList, "initialize_copy", list_replace, 1);

	rb_define_method(cList, "inspect", list_inspect, 0);
	rb_define_alias(cList, "to_s", "inspect");
	rb_define_method(cList, "to_a", list_to_a, 0);
	rb_define_method(cList, "to_ary", list_to_a, 0);
	rb_define_method(cList, "frozen?", list_frozen_p, 0);

	rb_define_method(cList, "==", list_equal, 1);
	rb_define_method(cList, "hash", list_hash, 0);

	rb_define_method(cList, "[]", list_aref, -1);
	rb_define_method(cList, "[]=", list_aset, -1);
	rb_define_method(cList, "at", list_at, 1);
	rb_define_method(cList, "fetch", list_fetch, -1);
	rb_define_method(cList, "first", list_first, -1);
	rb_define_method(cList, "last", list_last, -1);
	rb_define_method(cList, "concat", list_concat, 1);
	rb_define_method(cList, "<<", list_push, 1);
	rb_define_method(cList, "push", list_push_m, -1);
	rb_define_method(cList, "pop", list_pop_m, -1);
	rb_define_method(cList, "shift", list_shift_m, -1);
	rb_define_method(cList, "unshift", list_unshift_m, -1);
	rb_define_method(cList, "insert", list_insert, -1);
	rb_define_method(cList, "each", list_each, 0);
	rb_define_method(cList, "each_index", list_each_index, 0);
	rb_define_method(cList, "length", list_length, 0);
	rb_define_alias(cList, "size", "length");
	rb_define_method(cList, "empty?", list_empty_p, 0);
	rb_define_alias(cList, "index", "find_index");
	rb_define_method(cList, "rindex", list_rindex, -1);
	rb_define_method(cList, "join", list_join_m, -1);
	rb_define_method(cList, "reverse", list_reverse_m, 0);
	rb_define_method(cList, "reverse!", list_reverse_bang, 0);
	rb_define_method(cList, "rotate", list_rotate_m, -1);
	rb_define_method(cList, "rotate!", list_rotate_bang, -1);
	rb_define_method(cList, "sort", list_sort, 0);
	rb_define_method(cList, "sort!", list_sort_bang, 0);
	rb_define_method(cList, "sort_by", list_sort_by, 0);
	rb_define_method(cList, "sort_by!", list_sort_by_bang, 0);
	rb_define_method(cList, "collect", list_collect, 0);
	rb_define_method(cList, "collect!", list_collect_bang, 0);
	rb_define_method(cList, "map", list_collect, 0);
	rb_define_method(cList, "map!", list_collect_bang, 0);
	rb_define_method(cList, "select", list_select, 0);
	rb_define_method(cList, "select!", list_select_bang, 0);
	rb_define_method(cList, "keep_if", list_keep_if, 0);
	rb_define_method(cList, "values_at", list_values_at, -1);
	rb_define_method(cList, "delete", list_delete, 1);
	rb_define_method(cList, "delete_at", list_delete_at_m, 1);
	rb_define_method(cList, "delete_if", list_delete_if, 0);
	rb_define_method(cList, "reject", list_reject, 0);
	rb_define_method(cList, "reject!", list_reject_bang, 0);
	rb_define_method(cList, "zip", list_zip, -1);
	rb_define_method(cList, "transpose", list_transpose, 0);
	rb_define_method(cList, "replace", list_replace, 1);
	rb_define_method(cList, "clear", list_clear, 0);
	rb_define_method(cList, "fill", list_fill, -1);
	rb_define_method(cList, "include?", list_include_p, 1);
	rb_define_method(cList, "<=>", list_cmp, 1);

	rb_define_method(cList, "slice", list_aref, -1);
	rb_define_method(cList, "slice!", list_slice_bang, -1);

	rb_define_method(cList, "assoc", list_assoc, 1);
	rb_define_method(cList, "rassoc", list_rassoc, 1);

	rb_define_method(cList, "+", list_plus, 1);
	rb_define_method(cList, "*", list_times, 1);
	rb_define_method(cList, "-", list_diff, 1);
	rb_define_method(cList, "&", list_and, 1);
	rb_define_method(cList, "|", list_or, 1);

	rb_define_method(cList, "uniq", list_uniq, 0);
	rb_define_method(cList, "uniq!", list_uniq_bang, 0);
	rb_define_method(cList, "compact", list_compact, 0);
	rb_define_method(cList, "compact!", list_compact_bang, 0);
	rb_define_method(cList, "flatten", list_flatten, -1);
	rb_define_method(cList, "flatten!", list_flatten_bang, -1);
	rb_define_method(cList, "count", list_count, -1);
	rb_define_method(cList, "shuffle", list_shuffle, -1);
	rb_define_method(cList, "shuffle!", list_shuffle_bang, -1);
	rb_define_method(cList, "sample", list_sample, -1);
	rb_define_method(cList, "cycle", list_cycle, -1);
	rb_define_method(cList, "permutation", list_permutation, -1);
	rb_define_method(cList, "combination", list_combination, 1);
	rb_define_method(cList, "repeated_permutation", list_repeated_permutation, 1);
	rb_define_method(cList, "repeated_combination", list_repeated_combination, 1);
	rb_define_method(cList, "product", list_product, -1);

	rb_define_method(cList, "take", list_take, 1);
	rb_define_method(cList, "take_while", list_take_while, 0);
	rb_define_method(cList, "drop", list_drop, 1);
	rb_define_method(cList, "drop_while", list_drop_while, 0);
	rb_define_method(cList, "bsearch", list_bsearch, 0);

	rb_define_method(cList, "ring", list_ring, 0);
	rb_define_method(cList, "ring!", list_ring_bang, 0);
	rb_define_method(cList, "ring?", list_ring_p, 0);

	rb_define_method(cList, "to_list", list_to_list, 0);
	rb_define_method(rb_mEnumerable, "to_list", ary_to_list, -1);

	rb_define_method(cList, "pack", list_pack, 1);

	rb_define_const(cList, "VERSION", rb_str_new2(LIST_VERSION));

	id_cmp = rb_intern("<=>");
	id_each = rb_intern("each");
	id_to_list = rb_intern("to_list");
}
