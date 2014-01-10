#include "ruby.h"

VALUE cList;

typedef struct item_t {
	VALUE value;
	struct item_t *next;
} item_t;

typedef struct {
	item_t *first;
	item_t *last;
	long size;
} list_t;

static void
list_mark(list_t *ptr)
{
	item_t *i;
	if (ptr->first == NULL) return;

	for (i = ptr->first; i->next != NULL; i = i->next) {
		rb_gc_mark(i->value);
	}
}

static void
list_free(list_t *ptr)
{
	item_t *i;
	item_t *next;
	if (ptr->first == NULL) return;

	for (i = ptr->first; i->next != NULL;) {
		next = i->next;
		xfree(i);
		i = next;
	}
}

static VALUE
list_alloc(VALUE self)
{
	list_t *ptr = ALLOC(list_t);
	ptr->first = NULL;
	ptr->last = ptr->first;
	ptr->size = 0;
	return Data_Wrap_Struct(self, list_mark, list_free, ptr);
}

static VALUE
list_push_one(VALUE self, VALUE obj)
{
	list_t *ptr;
	item_t *next;

	next = xmalloc(sizeof(item_t));
	next->value = obj;
	next->next = NULL;
	Data_Get_Struct(self, list_t, ptr);
	if (ptr->first == NULL) {
		ptr->first = next;
		ptr->last = next;
		ptr->last->next = NULL;
	} else {
		ptr->last->next = next;
		ptr->last = next;
	}
	return self;
}

static VALUE
list_push(int argc, VALUE *argv, VALUE self)
{
	long i;
	switch (argc) {
	case 0:
		rb_raise(rb_eArgError, "no arguments");
		break;
	case 1:
		list_push_one(self, argv[0]);
		break;
	default:
		for (i = 0; i < argc; i++) {
			list_push_one(self, argv[i]);
		}
		break;
	}
	return self;
}

static VALUE
list_initialize(int argc, VALUE *argv, VALUE self)
{
	long i;
	long len;
	VALUE ary;

	if (argc == 0) {
		return self;
	}
	if (rb_type(argv[0]) == T_ARRAY) {
		ary = argv[0];
		len = RARRAY_LEN(ary);
		for (i = 0; i < len; i++) {
			list_push_one(self, rb_ary_entry(ary, i));
		}
		return self;
	}
	return Qnil;
}

static VALUE
list_each(VALUE self)
{
	item_t *i;
	list_t *ptr;

	Data_Get_Struct(self, list_t, ptr);
	if (ptr->first == NULL) return self;
	for (i = ptr->first; i->next != NULL; i = i->next) {
		rb_yield(i->value);
	}
	rb_yield(i->value);
	return self;
}

void
Init_list(void)
{
	cList = rb_define_class("List", rb_cObject);
	rb_include_module(cList, rb_mEnumerable);

	rb_define_alloc_func(cList, list_alloc);

	rb_define_method(cList, "initialize", list_initialize, -1);
	rb_define_method(cList, "each", list_each, 0);
	rb_define_method(cList, "push", list_push, -1);
}
