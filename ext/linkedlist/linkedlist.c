#include "ruby.h"
#include "ruby/encoding.h"

#define LIST_VERSION "0.0.1"

VALUE cLinkedList;

ID id_cmp, id_each;

typedef struct item_t {
	VALUE value;
	struct item_t *next;
} item_t;

typedef struct {
	item_t *first;
	item_t *last;
	long len;
} list_t;

#define DEBUG 0

#define LIST_MAX_SIZE ULONG_MAX

#ifndef FALSE
#  define FALSE 0
#endif

#ifndef TRUE
#  define TRUE 1
#endif

/* compatible for ruby-v1.9.3 */
#ifndef UNLIMITED_ARGUMENTS
#  define UNLIMITED_ARGUMENTS (-1)
#endif

/* compatible for ruby-v1.9.3 */
#ifndef rb_check_arity
#  define rb_check_arity(argc,min,max) do { \
	if (((argc) < (min) || ((max) != UNLIMITED_ARGUMENTS && (max) < (argc)))) { \
		rb_raise(rb_eArgError, "wrong number of argument (%d for %d..%d)", argc, min, max); \
	} \
} while (0)
#endif

/* compatible for ruby-v1.9.3 */
#ifndef RETURN_SIZED_ENUMERATOR
#  define RETURN_SIZED_ENUMERATOR(a,b,c,d) RETURN_ENUMERATOR(a,b,c)
#else
static VALUE
list_enum_length(VALUE self, VALUE args, VALUE eobj)
{
	list_t *ptr;
	Data_Get_Struct(self, list_t, ptr);
	return LONG2NUM(ptr->len);
}
#endif

#define LIST_FOR(ptr, c) for (c = ptr->first; c; c = c->next)

#define LIST_FOR_DOUBLE(ptr1, c1, ptr2, c2, code) do { \
	c1 = (ptr1)->first; \
	c2 = (ptr2)->first; \
	while ((c1) && (c2)) { \
		(code); \
		c1 = (c1)->next; \
		c2 = (c2)->next; \
	} \
} while (0)

enum list_take_pos_flags {
	LIST_TAKE_FIRST,
	LIST_TAKE_LAST
};

static VALUE list_push_ary(VALUE, VALUE);
static VALUE list_push(VALUE, VALUE);
static VALUE list_unshift(VALUE, VALUE);

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
	printf("ptr->len:%ld\n",ptr->len);
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

	if (ptr->len == 0 && ptr->first != NULL) check_print(ptr, msg, __LINE__);
	if (ptr->len != 0 && ptr->first == NULL) check_print(ptr, msg, __LINE__);
	if (ptr->len == 0 && ptr->last != NULL)  check_print(ptr, msg, __LINE__);
	if (ptr->len != 0 && ptr->last == NULL)  check_print(ptr, msg, __LINE__);
	if (ptr->first == NULL && ptr->last != NULL) check_print(ptr, msg, __LINE__);
	if (ptr->first != NULL && ptr->last == NULL) check_print(ptr, msg, __LINE__);
	len = ptr->len;
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
	return rb_obj_alloc(cLinkedList);
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
	switch (rb_type(obj)) {
	case T_DATA:
		return obj;
	case T_ARRAY:
		return ary_to_list(0, NULL, obj);
	default:
		return Qnil;
	}
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
list_mem_clear(list_t *ptr, long beg, long len)
{
	long i;
	item_t *c;
	item_t *next;
	item_t *mid, *last;

	if (beg == 0 && len == ptr->len) {
		list_free(ptr);
		ptr->first = NULL;
		ptr->last = NULL;
		ptr->len = 0;
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
	} else if (beg + len == ptr->len) {
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

	ptr->len -= len;
}

static inline list_t *
list_new_ptr(void)
{
	list_t *ptr = ALLOC(list_t);
	ptr->first = NULL;
	ptr->last = ptr->first;
	ptr->len = 0;
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
		rb_raise(rb_eArgError, "`LinkedList' cannot set recursive");
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
	ptr->len++;
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
	return to_list(obj);
}

static VALUE
list_s_try_convert(VALUE dummy, VALUE obj)
{
	return check_list_type(obj);
}

static VALUE
list_initialize(int argc, VALUE *argv, VALUE self)
{
	if (argc == 0) {
		return self;
	}
	if (rb_type(argv[0]) == T_ARRAY) {
		return list_push_ary(self, argv[0]);
	}
	return Qnil;
}

static VALUE
list_each(VALUE self)
{
	item_t *c;
	list_t *ptr;

	RETURN_SIZED_ENUMERATOR(self, 0, 0, list_enum_length);
	Data_Get_Struct(self, list_t, ptr);
	if (ptr->first == NULL) return self;

	LIST_FOR(ptr, c) {
		rb_yield(c->value);
	}
	return self;
}

static VALUE
list_each_index(VALUE self)
{
	item_t *c;
	list_t *ptr;
	long index;

	RETURN_SIZED_ENUMERATOR(self, 0, 0, list_enum_length);
	Data_Get_Struct(self, list_t, ptr);
	if (ptr->first == NULL) return self;

	index = 0;
	LIST_FOR(ptr, c) {
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
	ptr->len = 0;
	return self;
}

static VALUE
list_replace_ary(VALUE copy, VALUE orig)
{
	list_t *ptr_copy;
	item_t *c_copy;
	long i, olen;

	list_modify_check(copy);
	if (copy == orig) return copy;

	Data_Get_Struct(copy, list_t, ptr_copy);
	olen = RARRAY_LEN(orig);
	if (olen == 0) {
		return list_clear(copy);
	}
	if (olen == ptr_copy->len) {
		i = 0;
		LIST_FOR(ptr_copy, c_copy) {
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
	list_t *ptr_copy;
	list_t *ptr_orig;
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
	Data_Get_Struct(copy, list_t, ptr_copy);
	Data_Get_Struct(orig, list_t, ptr_orig);
	olen = ptr_orig->len;
	if (olen == 0) {
		return list_clear(copy);
	}
	if (olen == ptr_copy->len) {
		LIST_FOR_DOUBLE(ptr_orig, c_orig, ptr_copy, c_copy, {
			c_copy->value = c_orig->value;
		});
	} else {
		list_clear(copy);
		LIST_FOR(ptr_orig, c_orig) {
			list_push(copy, c_orig->value);
		}
	}

	return copy;
}

static VALUE
inspect_list(VALUE self, VALUE dummy, int recur)
{
	VALUE str, s;
	list_t *ptr;
	item_t *c;

	Data_Get_Struct(self, list_t, ptr);
	if (recur) return rb_usascii_str_new_cstr("[...]");

	str = rb_str_buf_new2("#<");
	rb_str_buf_cat2(str, rb_obj_classname(self));
	rb_str_buf_cat2(str, ": [");
	LIST_FOR(ptr, c) {
		s = rb_inspect(c->value);
		if (ptr->first == c) rb_enc_copy(str, s);
		else rb_str_buf_cat2(str, ", ");
		rb_str_buf_append(str, s);
	}
	rb_str_buf_cat2(str, "]>");
	return str;
}

static VALUE
list_inspect(VALUE self)
{
	list_t *ptr;
	Data_Get_Struct(self, list_t, ptr);
	if (ptr->len == 0)
		return rb_sprintf("#<%s: []>", rb_obj_classname(self));
	return rb_exec_recursive(inspect_list, self, 0);
}

static VALUE
list_to_a(VALUE self)
{
	list_t *ptr;
	item_t *c;
	VALUE ary;
	long i = 0;

	Data_Get_Struct(self, list_t, ptr);
	ary = rb_ary_new2(ptr->len);
	LIST_FOR(ptr, c) {
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
	list_t *p1, *p2;
	item_t *c1, *c2;

	if (recur) return Qtrue;

	Data_Get_Struct(list1, list_t, p1);
	Data_Get_Struct(list2, list_t, p2);
	if (p1->len != p2->len) return Qfalse;

	LIST_FOR_DOUBLE(p1, c1, p2, c2, {
		if (c1->value != c2->value) {
			if (!rb_equal(c1->value, c2->value)) {
				return Qfalse;
			}
		}
	});
	return Qtrue;
}

static VALUE
list_equal(VALUE self, VALUE obj)
{
	if (self == obj)
		return Qtrue;

	if (!rb_obj_is_kind_of(obj, cLinkedList)) {
		if (rb_type(obj) == T_ARRAY) {
			return Qfalse;
		}
		if (!rb_respond_to(obj, rb_intern("to_list"))) {
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
	list_t *ptr;
	VALUE n;

	Data_Get_Struct(self, list_t, ptr);
	h = rb_hash_start(ptr->len);
	h = rb_hash_uint(h, (st_index_t)list_hash);
	LIST_FOR(ptr, c) {
		n = rb_hash(c->value);
		h = rb_hash_uint(h, NUM2LONG(n));
	}
	h = rb_hash_end(h);
	return LONG2FIX(h);
}

static VALUE
list_elt_ptr(list_t* ptr, long offset)
{
	long i;
	item_t *c;
	long len;

	len = ptr->len;
	if (len == 0) return Qnil;
	if (offset < 0 || len <= offset) {
		return Qnil;
	}

	i = 0;
	LIST_FOR(ptr, c) {
		if (i++ == offset) {
			return c->value;
		}
	}
	return Qnil;
}

static VALUE
list_elt(VALUE self, long offset)
{
	list_t *ptr;
	Data_Get_Struct(self, list_t, ptr);
	return list_elt_ptr(ptr, offset);
}

static VALUE
list_entry(VALUE self, long offset)
{
	list_t *ptr;

	Data_Get_Struct(self, list_t, ptr);
	if (offset < 0) {
		offset += ptr->len;
	}
	return list_elt(self, offset);
}

static VALUE
list_make_partial(VALUE self, VALUE klass, long offset, long len)
{
	VALUE instance;
	item_t *c;
	list_t *ptr;
	long i;

	Data_Get_Struct(self, list_t, ptr);
	instance = rb_obj_alloc(klass);
	i = -1;
	LIST_FOR(ptr, c) {
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
	alen = ptr->len;

	if (alen < beg) return Qnil;
	if (beg < 0 || len < 0) return Qnil;

	if (alen < len || alen < beg + len) {
		len = alen - beg;
	}
	if (len == 0) {
		return list_new();
	}

	return list_make_partial(self, cLinkedList, beg, len);
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
			beg += ptr->len;
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
	switch (rb_range_beg_len(arg, &beg, &len, ptr->len, 0)) {
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
	list_t *ptr;
	item_t *c = NULL;
	item_t *item_first = NULL, *item_last = NULL, *first = NULL, *last = NULL;

	if (len < 0)
		rb_raise(rb_eIndexError, "negative length (%ld)", len);

	Data_Get_Struct(self, list_t, ptr);
	olen = ptr->len;
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
		olen = ptr->len;
	}
	if (olen <= beg) {
		if (LIST_MAX_SIZE - rlen < beg) {
			rb_raise(rb_eIndexError, "index %ld too big", beg);
		}
		for (i = ptr->len; i < beg; i++) {
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
			LIST_FOR(ptr, c) {
				i++;
				if (i == beg - 1) {
					first = c;
				}
				if (i == beg + len) {
					last = c;
				}
			}
			if (beg == 0) {
				ptr->first = item_first;
			} else {
				first->next = item_first;
			}
			if (rlen == 0) {
				ptr->last = first;
				ptr->last->next = NULL;
			} else {
				item_last->next = last;
			}
			ptr->len += rlen - len;
		} else {
			i = -1;
			LIST_FOR(ptr, c) {
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
	list_t *ptr;
	long len, i;

	Data_Get_Struct(self, list_t, ptr);
	len = ptr->len;

	if (idx < 0) {
		idx += len;
		if (idx < 0) {
			rb_raise(rb_eIndexError, "index %ld too small for list; minimum: %ld",
					idx - len, -len);
		}
	} else if (LIST_MAX_SIZE <= idx) {
		rb_raise(rb_eIndexError, "index %ld too big", idx);
	}

	if (ptr->len <= idx) {
		for (i = ptr->len; i <= idx; i++) {
			list_push(self, Qnil);
		}
	}

	i = -1;
	LIST_FOR(ptr, c) {
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

	list_modify_check(self);
	Data_Get_Struct(self, list_t, ptr);
	if (argc == 3) {
		beg = NUM2LONG(argv[0]);
		len = NUM2LONG(argv[1]);
		list_splice(self, beg, len, argv[2]);
		return argv[2];
	}
	rb_check_arity(argc, 2, 2);
	if (FIXNUM_P(argv[0])) {
		offset = FIX2LONG(argv[0]);
		goto fixnum;
	}
	if (rb_range_beg_len(argv[0], &beg, &len, ptr->len, 1)) {
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
		idx += ptr->len;
	}
	if (idx < 0 || ptr->len <= idx) {
		if (block_given) return rb_yield(pos);
		if (argc == 1) {
			rb_raise(rb_eIndexError, "index %ld outside of array bounds: %ld...%ld",
					idx - (idx < 0 ? ptr->len : 0), -ptr->len, ptr->len);
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
	len = ptr->len;
	if (n > len) {
		n = len;
	} else if (n < 0) {
		rb_raise(rb_eArgError, "negative array size");
	}
	if (flag == LIST_TAKE_LAST) {
		offset = len - n;
	}
	return list_make_partial(self, cLinkedList, offset, n);
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
		len = ptr->len;
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
	list_t *ptr_self;
	list_t *ptr_obj;
	enum ruby_value_type type;

	list_modify_check(self);
	type = rb_type(obj);
	if (type == T_DATA) {
		Data_Get_Struct(obj, list_t, ptr_obj);
		len = ptr_obj->len;
	} else if (type == T_ARRAY) {
		len = RARRAY_LEN(obj);
	} else {
		obj = to_list(obj);
		Data_Get_Struct(obj, list_t, ptr_obj);
		len = ptr_obj->len;
	}

	Data_Get_Struct(self, list_t, ptr_self);
	if (0 < len) {
		list_splice(self, ptr_self->len, 0, obj);
	}
	return self;
}

static VALUE
list_pop(VALUE self)
{
	VALUE result;
	list_t *ptr;

	list_modify_check(self);
	Data_Get_Struct(self, list_t, ptr);
	if (ptr->len == 0) return Qnil;
	result = ptr->last->value;
	list_mem_clear(ptr, ptr->len - 1, 1);
	return result;
}

static VALUE
list_pop_m(int argc, VALUE *argv, VALUE self)
{
	list_t *ptr;
	VALUE result;
	long n;

	if (argc == 0) {
		return list_pop(self);
	}

	list_modify_check(self);
	result = list_take_first_or_last(argc, argv, self, LIST_TAKE_LAST);
	Data_Get_Struct(self, list_t, ptr);
	n = NUM2LONG(argv[0]);
	list_mem_clear(ptr, ptr->len - n, n);
	return result;
}

static VALUE
list_shift(VALUE self)
{
	VALUE result;
	list_t *ptr;

	Data_Get_Struct(self, list_t, ptr);
	if (ptr->len == 0) return Qnil;
	result = list_first(0, NULL, self);
	list_mem_clear(ptr, 0, 1);
	return result;
}

static VALUE
list_shift_m(int argc, VALUE *argv, VALUE self)
{
	VALUE result;
	long i;
	list_t *ptr_res;

	if (argc == 0) {
		return list_shift(self);
	}

	list_modify_check(self);
	result = list_take_first_or_last(argc, argv, self, LIST_TAKE_FIRST);
	Data_Get_Struct(result, list_t, ptr_res);
	for (i = 0; i < ptr_res->len; i++) {
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
	ptr->len++;
	return self;
}

static VALUE
list_unshift_m(int argc, VALUE *argv, VALUE self)
{
	long i;

	list_modify_check(self);
	if (argc == 0) return self;
	for (i = 0; i < argc; i++) {
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
		pos = ptr->len;
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
	return LONG2NUM(ptr->len);
}

static VALUE
list_empty_p(VALUE self)
{
	list_t *ptr;
	Data_Get_Struct(self, list_t, ptr);
	if (ptr->len == 0)
		return Qtrue;
	return Qfalse;
}

static VALUE
list_rindex(int argc, VALUE *argv, VALUE self)
{
	list_t *ptr;
	long i;
	long len;
	VALUE val;

	Data_Get_Struct(self, list_t, ptr);
	i = ptr->len;
	if (argc == 0) {
		RETURN_ENUMERATOR(self, 0, 0);
		while (i--) {
			Data_Get_Struct(self, list_t, ptr);
			if (RTEST(rb_yield(list_elt_ptr(ptr, i))))
				return LONG2NUM(i);
			if (ptr->len < i) {
				i = ptr->len;
			}
		}
		return Qnil;
	}
	rb_check_arity(argc, 0, 1);
	val = argv[0];
	if (rb_block_given_p())
		rb_warn("given block not used");
	Data_Get_Struct(self, list_t, ptr);
	while (i--) {
		if (rb_equal(list_elt_ptr(ptr, i), val)) {
			return LONG2NUM(i);
		}
		len = ptr->len;
		if (len < i) {
			i = len;
		}
		Data_Get_Struct(self, list_t, ptr);
	}
	return Qnil;
}

extern VALUE rb_output_fs;

static void list_join_1(VALUE obj, VALUE ary, VALUE sep, long i, VALUE result, int *first);

static VALUE
recursive_join(VALUE obj, VALUE argp, int recur)
{
	VALUE *arg = (VALUE *)argp;
	VALUE ary = arg[0];
	VALUE sep = arg[1];
	VALUE result = arg[2];
	int *first = (int *)arg[3];

	if (recur) {
		rb_raise(rb_eArgError, "recursive linkedlist join");
	} else {
		list_join_1(obj, ary, sep, 0, result, first);
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
	if (0 < max) rb_enc_copy(result, list_elt_ptr(ptr, 0));
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
	list_t *ptr;
	item_t *c;
	VALUE val, tmp;

	Data_Get_Struct(list, list_t, ptr);
	if (ptr->len == 0) return;

	LIST_FOR(ptr, c) {
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
		case T_DATA:
			obj = val;
			ary_join:
			if (val == list) {
				rb_raise(rb_eArgError, "recursive linkedlist join");
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
			tmp = rb_check_convert_type(val, T_ARRAY, "Array", "to_ary");
			if (!NIL_P(tmp)) {
				obj = val;
				val = tmp;
				goto ary_join;
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
	list_t *ptr;
	long len = 1;
	long i = 0;
	VALUE val, tmp;
	VALUE result;
	item_t *c;
	int first;

	Data_Get_Struct(self, list_t, ptr);
	if (ptr->len == 0) return rb_usascii_str_new(0, 0);

	if (!NIL_P(sep)) {
		StringValue(sep);
		len += RSTRING_LEN(sep) * (ptr->len - 1);
	}
	LIST_FOR(ptr, c) {
		val = c->value;
		tmp = rb_check_string_type(val);
		if (NIL_P(val) || c->value != tmp) {
			result = rb_str_buf_new(len + ptr->len);
			rb_enc_associate(result, rb_usascii_encoding());
			list_join_0(self, sep, i, result);
			first = i == 0;
			list_join_1(self, self, sep, i, result, &first);
			return result;
		}
		len += RSTRING_LEN(val);
	}
	result = rb_str_buf_new(len);
	list_join_0(self, sep, ptr->len, result);

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
	list_t *ptr;
	long len;

	Data_Get_Struct(self, list_t, ptr);
	if (ptr->len == 0) return self;
	tmp = list_to_a(self);
	len = ptr->len;
	LIST_FOR(ptr, c) {
		c->value = rb_ary_entry(tmp, --len);
	}
	return self;
}

static VALUE
list_reverse_m(VALUE self)
{
	VALUE result;
	item_t *c;
	list_t *ptr;

	Data_Get_Struct(self, list_t, ptr);
	result = list_new();
	if (ptr->len == 0) return result;
	LIST_FOR(ptr, c) {
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

	Data_Get_Struct(self, list_t, ptr);
	if (ptr->len == 0) return self;
	cnt = (cnt < 0) ? (ptr->len - (~cnt % ptr->len) - 1) : (cnt & ptr->len);
	LIST_FOR(ptr, c) {
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
	list_t *ptr;
	item_t *c;
	long i = 0;

	VALUE ary = list_to_a(self);
	rb_ary_sort_bang(ary);
	Data_Get_Struct(self, list_t, ptr);
	LIST_FOR(ptr, c) {
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
	list_t *ptr;
	item_t *c;

	RETURN_SIZED_ENUMERATOR(self, 0, 0, list_enum_length);
	Data_Get_Struct(self, list_t, ptr);
	LIST_FOR(ptr, c) {
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
	list_t *ptr;
	item_t *c;

	RETURN_SIZED_ENUMERATOR(self, 0, 0, list_enum_length);
	Data_Get_Struct(self, list_t, ptr);
	result = list_new();
	LIST_FOR(ptr, c) {
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
	list_t *ptr;
	item_t *c;
	long i = 0;

	RETURN_SIZED_ENUMERATOR(self, 0, 0, list_enum_length);
	Data_Get_Struct(self, list_t, ptr);
	result = list_new();
	LIST_FOR(ptr, c) {
		if (RTEST(rb_yield(c->value))) {
			i++;
			list_push(result, c->value);
		}
	}

	if (i == ptr->len) return Qnil;
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
		} else if (rb_range_beg_len(argv[i], &beg, &len, ptr->len, 1)) {
			for (j = beg; j < beg + len; j++) {
				if (ptr->len < j) {
					list_push(result, Qnil);
				} else {
					list_push(result, list_elt_ptr(ptr, j));
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
	if (ptr->len == 0) return Qnil;

	len = ptr->len;
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
			ptr->len--;
		} else {
			before = c;
		}
	}

	if (ptr->len == len) {
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
	len = ptr->len;
	if (ptr->len == 0) return Qnil;
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
			ptr->len--;
			break;
		} else {
			before = c;
		}
	}

	if (ptr->len == len) {
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
	list_t *ptr;
	item_t *c;

	Data_Get_Struct(self, list_t, ptr);
	LIST_FOR(ptr, c) {
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
	len = ptr->len;
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
			ptr->len--;
		} else {
			before = c;
		}
	}

	if (ptr->len == len) {
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
	for (i = 0; i < RARRAY_LEN(ary); i++) {
		rb_ary_store(ary, i, to_list(rb_ary_entry(ary, i)));
	}
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
	VALUE ary;
	long i;

	ary = rb_funcall(list_to_a(self), rb_intern("transpose"), 0);
	for (i = 0; i < RARRAY_LEN(ary); i++) {
		rb_ary_store(ary, i, to_list(rb_ary_entry(ary, i)));
	}
	return to_list(ary);
}

static VALUE
list_fill(int argc, VALUE *argv, VALUE self)
{
	VALUE item, arg1, arg2;
	long beg = 0, len = 0, end = 0;
	long i;
	int block_p = FALSE;
	list_t *ptr;
	item_t *c;

	Data_Get_Struct(self, list_t, ptr);

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
		len = ptr->len;
		break;
	case 2:
		if (rb_range_beg_len(arg1, &beg, &len, ptr->len, 1)) {
			break;
		}
		/* fall through */
	case 3:
		beg = NIL_P(arg1) ? 0 : NUM2LONG(arg1);
		if (beg < 0) {
			beg = ptr->len + beg;
			if (beg < 0) beg = 0;
		}
		len = NIL_P(arg2) ? ptr->len - beg : NUM2LONG(arg2);
		break;
	}
	if (len < 0) {
		return self;
	}
	if (LIST_MAX_SIZE <= beg || LIST_MAX_SIZE - beg < len) {
		rb_raise(rb_eArgError, "argument too big");
	}
	end = beg + len;
	if (ptr->len < end) {
		for (i = 0; i < end - ptr->len; i++) {
			list_push(self, Qnil);
		}
	}

	i = -1;
	LIST_FOR(ptr, c) {
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
	list_t *ptr;
	item_t *c;

	Data_Get_Struct(self, list_t, ptr);
	LIST_FOR (ptr, c) {
		if (rb_equal(c->value, item)) {
			return Qtrue;
		}
	}
	return Qfalse;
}

static VALUE
recursive_cmp(VALUE list1, VALUE list2, int recur)
{
	list_t *p1, *p2;
	item_t *c1, *c2;
	long len;

	if (recur) return Qundef;
	Data_Get_Struct(list1, list_t, p1);
	Data_Get_Struct(list2, list_t, p2);
	len = p1->len;
	if (p2->len < len) {
		len = p2->len;
	}
	LIST_FOR_DOUBLE(p1,c1,p2,c2,{
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
	list_t *p1, *p2;
	long len;

	if (rb_type(list2) != T_DATA) return Qnil;
	if (list1 == list2) return INT2FIX(0);
	v = rb_exec_recursive_paired(recursive_cmp, list1, list2, list2);
	if (v != Qundef) return v;
	Data_Get_Struct(list1, list_t, p1);
	Data_Get_Struct(list2, list_t, p2);
	len = p1->len - p2->len;
	if (len == 0) return INT2FIX(0);
	if (0 < len) return INT2FIX(1);
	return INT2FIX(-1);
}

static VALUE
list_slice_bang(int argc, VALUE *argv, VALUE self)
{
	long pos = 0, len = 0;
	list_t *ptr;

	Data_Get_Struct(self, list_t, ptr);

	list_modify_check(self);
	if (argc == 1) {
		if (FIXNUM_P(argv[0])) {
			return list_delete_at(self, NUM2LONG(argv[0]));
		} else {
			switch (rb_range_beg_len(argv[0], &pos, &len, ptr->len, 0)) {
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
		pos += ptr->len;
		if (pos < 0) return Qnil;
	} else if (ptr->len < pos) {
		return Qnil;
	}
	if (ptr->len < pos + len) {
		len = ptr->len - pos;
	}
	if (len == 0) return list_new();
	VALUE list2 = list_subseq(self, pos, len);
	list_mem_clear(ptr, pos, len);
	return list2;
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

void
Init_linkedlist(void)
{
	cLinkedList = rb_define_class("LinkedList", rb_cObject);
	rb_include_module(cLinkedList, rb_mEnumerable);

	rb_define_alloc_func(cLinkedList, list_alloc);

	rb_define_singleton_method(cLinkedList, "[]", list_s_create, -1);
	rb_define_singleton_method(cLinkedList, "try_convert", list_s_try_convert, 1);

	rb_define_method(cLinkedList, "initialize", list_initialize, -1);
	rb_define_method(cLinkedList, "initialize_copy", list_replace, 1);

	rb_define_method(cLinkedList, "inspect", list_inspect, 0);
	rb_define_alias(cLinkedList, "to_s", "inspect");
	rb_define_method(cLinkedList, "to_a", list_to_a, 0);
	rb_define_method(cLinkedList, "to_ary", list_to_a, 0);
	rb_define_method(cLinkedList, "frozen?", list_frozen_p, 0);

	rb_define_method(cLinkedList, "==", list_equal, 1);
	rb_define_method(cLinkedList, "hash", list_hash, 0);

	rb_define_method(cLinkedList, "[]", list_aref, -1);
	rb_define_method(cLinkedList, "[]=", list_aset, -1);
	rb_define_method(cLinkedList, "at", list_at, 1);
	rb_define_method(cLinkedList, "fetch", list_fetch, -1);
	rb_define_method(cLinkedList, "first", list_first, -1);
	rb_define_method(cLinkedList, "last", list_last, -1);
	rb_define_method(cLinkedList, "concat", list_concat, 1);
	rb_define_method(cLinkedList, "<<", list_push, 1);
	rb_define_method(cLinkedList, "push", list_push_m, -1);
	rb_define_method(cLinkedList, "pop", list_pop_m, -1);
	rb_define_method(cLinkedList, "shift", list_shift_m, -1);
	rb_define_method(cLinkedList, "unshift", list_unshift_m, -1);
	rb_define_method(cLinkedList, "insert", list_insert, -1);
	rb_define_method(cLinkedList, "each", list_each, 0);
	rb_define_method(cLinkedList, "each_index", list_each_index, 0);
	rb_define_method(cLinkedList, "length", list_length, 0);
	rb_define_alias(cLinkedList, "size", "length");
	rb_define_method(cLinkedList, "empty?", list_empty_p, 0);
	rb_define_alias(cLinkedList, "index", "find_index");
	rb_define_method(cLinkedList, "rindex", list_rindex, -1);
	rb_define_method(cLinkedList, "join", list_join_m, -1);
	rb_define_method(cLinkedList, "reverse", list_reverse_m, 0);
	rb_define_method(cLinkedList, "reverse!", list_reverse_bang, 0);
	rb_define_method(cLinkedList, "rotate", list_rotate_m, -1);
	rb_define_method(cLinkedList, "rotate!", list_rotate_bang, -1);
	rb_define_method(cLinkedList, "sort", list_sort, 0);
	rb_define_method(cLinkedList, "sort!", list_sort_bang, 0);
	rb_define_method(cLinkedList, "sort_by", list_sort_by, 0);
	rb_define_method(cLinkedList, "sort_by!", list_sort_by_bang, 0);
	rb_define_method(cLinkedList, "collect", list_collect, 0);
	rb_define_method(cLinkedList, "collect!", list_collect_bang, 0);
	rb_define_method(cLinkedList, "map", list_collect, 0);
	rb_define_method(cLinkedList, "map!", list_collect_bang, 0);
	rb_define_method(cLinkedList, "select", list_select, 0);
	rb_define_method(cLinkedList, "select!", list_select_bang, 0);
	rb_define_method(cLinkedList, "keep_if", list_keep_if, 0);
	rb_define_method(cLinkedList, "values_at", list_values_at, -1);
	rb_define_method(cLinkedList, "delete", list_delete, 1);
	rb_define_method(cLinkedList, "delete_at", list_delete_at_m, 1);
	rb_define_method(cLinkedList, "delete_if", list_delete_if, 0);
	rb_define_method(cLinkedList, "reject", list_reject, 0);
	rb_define_method(cLinkedList, "reject!", list_reject_bang, 0);
	rb_define_method(cLinkedList, "zip", list_zip, -1);
	rb_define_method(cLinkedList, "transpose", list_transpose, 0);
	rb_define_method(cLinkedList, "replace", list_replace, 1);
	rb_define_method(cLinkedList, "clear", list_clear, 0);
	rb_define_method(cLinkedList, "fill", list_fill, -1);
	rb_define_method(cLinkedList, "include?", list_include_p, 1);
	rb_define_method(cLinkedList, "<=>", list_cmp, 1);

	rb_define_method(cLinkedList, "slice", list_aref, -1);
	rb_define_method(cLinkedList, "slice!", list_slice_bang, -1);

	rb_define_method(cLinkedList, "ring", list_ring, 0);
	rb_define_method(cLinkedList, "ring!", list_ring_bang, 0);
	rb_define_method(cLinkedList, "ring?", list_ring_p, 0);

	rb_define_method(cLinkedList, "to_list", list_to_list, 0);
	rb_define_method(rb_mEnumerable, "to_list", ary_to_list, -1);

	rb_define_const(cLinkedList, "VERSION", rb_str_new2(LIST_VERSION));

	id_cmp = rb_intern("<=>");
	id_each = rb_intern("each");
}
