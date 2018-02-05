#!/bin/sh

. ../../dttools/test/test_runner_common.sh

exe="list_core.test"

prepare()
{
	gcc -I../src/ -g $CCTOOLS_TEST_CCFLAGS -o "$exe" -x c - -x none ../src/libdttools.a -lm <<EOF
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "list.h"

int main (int argc, char *argv[])
{
	bool ok;
	intptr_t item = 0;

	// first create an empty list
	struct list *list = list_create();
	assert(list);
	assert(list_size(list) == 0);

	// make a cursor on it
	struct list_cursor *cur = list_cursor_create(list);
	assert(cur);

	// ensure that things don't work when position is undefined
	assert(list_tell(cur) == -1);
	ok = list_seek(cur, 0);
	assert(!ok);
	ok = list_next(cur);
	assert(!ok);
	ok = list_prev(cur);
	assert(!ok);
	ok = list_get(cur, (void **) &item);
	assert(!ok);
	ok = list_set(cur, (void *) item);
	assert(!ok);
	ok = list_drop(cur);
	assert(!ok);

	// put in a couple of items
	list_insert(cur, (void *) 3);
	assert(list_size(list) == 1);
	assert(list_tell(cur) == -1);
	list_insert(cur, (void *) 2);
	assert(list_size(list) == 2);
	assert(list_tell(cur) == -1);

	// move on to an item
	ok = list_seek(cur, 0);
	assert(ok);

	// try moving past the end
	ok = list_seek(cur, 2);
	assert(!ok);

	// try basic functionality
	assert(list_tell(cur) == 0);
	ok = list_get(cur, (void **) &item);
	assert(ok);
	assert(item == 2);
	ok = list_set(cur, (void *) 5);
	assert(ok);
	ok = list_get(cur, (void **) &item);
	assert(ok);
	assert(item == 5);

	// try a negative index
	ok = list_seek(cur, -1);
	assert(ok);

	// try to seek past the beginning
	ok = list_seek(cur, -3);
	assert(!ok);

	// check we're on the right item
	assert(list_tell(cur) == 1);
	ok = list_get(cur, (void **) &item);
	assert(ok);
	assert(item == 3);

	// make another cursor, and insert between the two elements
	struct list_cursor *alt = list_cursor_create(list);
	assert(list_tell(alt) == -1);
	ok = list_seek(alt, -2);
	assert(ok);
	assert(list_tell(alt) == 0);
	list_insert(alt, (void *) 7);
	assert(list_size(list) == 3);

	// make sure the original cursor is OK
	assert(list_tell(cur) == 2);
	ok = list_get(cur, (void **) &item);
	assert(ok);
	assert(item == 3);

	// and the new cursor
	assert(list_tell(alt) == 0);
	ok = list_get(alt, (void **) &item);
	assert(ok);
	assert(item == 5);

	// move to the new item
	ok = list_next(alt);
	assert(ok);

	// and check both again
	assert(list_tell(cur) == 2);
	ok = list_get(cur, (void **) &item);
	assert(ok);
	assert(item == 3);

	assert(list_tell(alt) == 1);
	ok = list_get(alt, (void **) &item);
	assert(ok);
	assert(item == 7);

	// now move the old cursor to the same place
	ok = list_prev(cur);
	assert(ok);

	// and check both again
	assert(list_tell(cur) == 1);
	ok = list_get(cur, (void **) &item);
	assert(ok);
	assert(item == 7);

	assert(list_tell(alt) == 1);
	ok = list_get(alt, (void **) &item);
	assert(ok);
	assert(item == 7);

	// drop the middle element
	ok = list_drop(cur);
	assert(ok);
	assert(list_size(list) == 2);

	// and check both again
	assert(list_tell(cur) == 1);
	ok = list_get(cur, (void **) &item);
	assert(ok);
	assert(item == 3);

	assert(list_tell(alt) == 1);
	ok = list_get(alt, (void **) &item);
	assert(ok);
	assert(item == 3);

	// walk off the right
	ok = list_next(alt);
	assert(!ok);
	assert(list_tell(alt) == -1);

	// and the left
	ok = list_prev(cur);
	assert(ok);
	assert(list_tell(cur) == 0);
	ok = list_get(cur, (void **) &item);
	assert(ok);
	assert(item == 5);
	ok = list_prev(cur);
	assert(!ok);
	assert(list_tell(cur) == -1);

	// delete the cursors and try to delete while there are items
	list_cursor_destroy(cur);
	list_cursor_destroy(alt);
	ok = list_destroy(list);
	assert(!ok);

	// we still need a cursor...
	cur = list_cursor_create(list);
	assert(cur);

	// clear the list
	list_seek(cur, 0);
	while (list_tell(cur) != -1) {
		ok = list_drop(cur);
		assert(ok);
	}

	// try to delete while there's an outstanding cursor
	ok = list_destroy(list);
	assert(!ok);

	// delete said cursor
	list_cursor_destroy(cur);

	// finally delete the list
	ok = list_destroy(list);
	assert(ok);

	return 0;
}
EOF
	return $?
}

run()
{
	./"$exe"
	return $?
}

clean()
{
	rm -f "$exe"
	return 0
}

dispatch "$@"
