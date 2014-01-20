#include "ruby.h"
#include "ruby/encoding.h"

#define LIST_MAX_SIZE ULONG_MAX

VALUE cList;

typedef struct item_t {
	VALUE value;
	struct item_t *next;
} item_t;

typedef struct {
	item_t *first;
	item_t *last;
	long len;
} list_t;

enum list_take_pos_flags {
	LIST_TAKE_FIRST,
	LIST_TAKE_LAST
};

static VALUE list_push_ary(VALUE, VALUE);
static VALUE list_unshift(VALUE, VALUE);

static void
check_print(list_t *ptr, const char *msg, long lineno)
{
	printf("===ERROR(%s)", msg);
	printf("lineno:%ld===\n", lineno);
	printf("ptr:%p\n",ptr);
	printf("ptr->len:%ld\n",ptr->len);
	printf("ptr->first:%p\n",ptr->first);
	printf("ptr->last:%p\n",ptr->last);
	rb_p(ptr->first->value);
}

static void
check(list_t *ptr, const char *msg)
{
	item_t *end;
	item_t *c;
	long len, i;

	if (ptr->len == 0 && ptr->first != NULL) check_print(ptr, msg, __LINE__);
	if (ptr->len != 0 && ptr->first == NULL) check_print(ptr, msg, __LINE__);
	if (ptr->len == 0 && ptr->last != NULL)  check_print(ptr, msg, __LINE__);
	if (ptr->len != 0 && ptr->last == NULL)  check_print(ptr, msg, __LINE__);
	if (ptr->first == NULL && ptr->last != NULL) check_print(ptr, msg, __LINE__);
	if (ptr->first != NULL && ptr->last == NULL) check_print(ptr, msg, __LINE__);

	len = ptr->len;
	i = 0;
	end = ptr->last->next;
	i++;
	for (c = ptr->first->next; c != end; c = c->next) {
		i++;
	}
	if (len != i) check_print(ptr, msg, __LINE__);
}

static VALUE
to_list(VALUE obj)
{
	switch (rb_type(obj)) {
	case T_DATA:
		return obj;
	case T_ARRAY:
		return list_push_ary(rb_obj_alloc(cList), obj);
	default:
		return Qnil;
	}
}

static inline void
list_modify_check(VALUE self)
{
	rb_check_frozen(self);
}

static VALUE
list_enum_length(VALUE self, VALUE args, VALUE eobj)
{
	list_t *ptr;
	Data_Get_Struct(self, list_t, ptr);
	return LONG2NUM(ptr->len);
}

static void
list_mark(list_t *ptr)
{
	item_t *c;
	item_t *end;

	if (ptr->first == NULL) return;
check(ptr, "mark");
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
check(ptr, "free");
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
	ptr->len++;
	return self;
}

static VALUE
list_push_ary(VALUE self, VALUE ary)
{
	long i;
	for (i = 0; i < RARRAY_LEN(ary); i++) {
		list_push(self, rb_ary_entry(ary, i));
	}
	return self;
}

static VALUE
list_push_m(int argc, VALUE *argv, VALUE self)
{
	list_modify_check(self);
	long i;

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

	for (c = ptr->first; c; c = c->next) {
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
	for (c = ptr->first; c; c = c->next) {
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
	ptr->last = ptr->first;
	ptr->len = 0;
	return self;
}

static VALUE
list_replace(VALUE copy, VALUE orig)
{
	list_t *ptr_copy;
	list_t *ptr_orig;
	item_t *c;

	list_modify_check(copy);
	if (copy == orig) return copy;
	orig = to_list(orig);
	Data_Get_Struct(copy, list_t, ptr_copy);
	Data_Get_Struct(orig, list_t, ptr_orig);
	if (ptr_orig->len == 0) {
		ptr_copy->first = NULL;
		ptr_copy->last = NULL;
		ptr_copy->len = 0;
		return copy;
	}

	list_clear(copy);
	for (c = ptr_orig->first; c; c = c->next) {
		list_push(copy, c->value);
	}
	return copy;
}

static VALUE
inspect_list(VALUE self, VALUE dummy, int recur)
{
	VALUE str, s;
	list_t *ptr;
	item_t *last;

	Data_Get_Struct(self, list_t, ptr);
	if (recur) return rb_usascii_str_new_cstr("[...]");

	str = rb_str_buf_new2("#<List: [");
	for (last = ptr->first; last != NULL; last = last->next) {
		s = rb_inspect(last->value);
		if (ptr->first == last) rb_enc_copy(str, s);
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
	if (ptr->len == 0) return rb_usascii_str_new2("#<List: []>");
	return rb_exec_recursive(inspect_list, self, 0);
}

static VALUE
list_to_a(VALUE self)
{
	list_t *ptr;
	item_t *current;
	VALUE ary;

	Data_Get_Struct(self, list_t, ptr);
	ary = rb_ary_new2(ptr->len);
	for (current = ptr->first; current != NULL; current = current->next) {
		rb_ary_push(ary, current->value);
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
list_equal(VALUE self, VALUE obj)
{
	long i;
	list_t *ptr_self, *ptr_obj;
	item_t *scur, *ocur;
	VALUE klass;

	if (self == obj)
		return Qtrue;

	klass = rb_funcall(obj, rb_intern("class"), 0);

	if (cList == klass) {
		Data_Get_Struct(self, list_t, ptr_self);
		Data_Get_Struct(obj, list_t, ptr_obj);
		if (ptr_self->len != ptr_obj->len)
			return Qfalse;
		scur = ptr_self->first;
		ocur = ptr_obj->first;
		for (i = 0; i < ptr_self->len; i++) {
			if (!rb_equal(scur->value, ocur->value)) {
				return Qfalse;
			}
			scur = scur->next;
			ocur = ocur->next;
		}
		return Qtrue;
	}

	return Qfalse;
}

static VALUE
list_hash(VALUE self)
{
	item_t *current;
	st_index_t h;
	list_t *ptr;
	VALUE n;

	Data_Get_Struct(self, list_t, ptr);
	h = rb_hash_start(ptr->len);
	h = rb_hash_uint(h, (st_index_t)list_hash);
	for (current = ptr->first; current; current = current->next) {
		n = rb_hash(current->value);
		h = rb_hash_uint(h, NUM2LONG(n));
	}
	h = rb_hash_end(h);
	return LONG2FIX(h);
}

static inline VALUE
list_elt(VALUE self, long offset)
{
	long i;
	list_t *ptr;
	item_t *c;
	long len;

	Data_Get_Struct(self, list_t, ptr);
	len = ptr->len;
	if (len == 0) return Qnil;
	if (offset < 0 || len <= offset) {
		return Qnil;
	}

	i = 0;
	for (c = ptr->first; c; c = c->next) {
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
	for (c = ptr->first; c; c = c->next) {
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
		return rb_obj_alloc(cList);
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
			for (c = ptr->first; c; c = c->next) {
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
			for (c = ptr->first; c; c = c->next) {
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
	for (c = ptr->first; c; c = c->next) {
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
	list_ring_bang(clone);
	return clone;
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

	rb_define_method(cList, "replace", list_replace, 1);

	rb_define_method(cList, "clear", list_clear, 0);

	rb_define_method(cList, "ring", list_ring, 0);
	rb_define_method(cList, "ring!", list_ring_bang, 0);
	rb_define_method(cList, "ring?", list_ring_p, 0);
}
