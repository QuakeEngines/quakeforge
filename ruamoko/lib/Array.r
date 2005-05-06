#include "Array.h"

@implementation Array

- (id) init
{
	self = [super init];
	count = size = 0;
	incr = 16;
	array = NIL;
	return self;
}

- (id) initWithIncrement: (integer) inc
{
	count = 0;
	size = incr = inc;
	array = (id []) obj_malloc (inc * @sizeof (id));
	return self;
}

- (void) dealloc
{
	local integer i;
	for (i = 0; i < count; i++) {
		if (array[i])
			[array[i] release];
	}
	if (array) {
		obj_free (array);
	}
	[super dealloc];
}

- (id) getItemAt: (integer) index
{
	if (index == -1)
		index = count - 1;
	if (index < 0 || index >= count)
		return NIL;
	return array[index];
}

- (void) setItemAt: (integer) index item: (id) item
{
	if (index == -1)
		index = count - 1;
	if (index < 0 || index >= count)
		return;
	[array[index] release];
	array[index] = item;
	[item retain];
}

- (void) addItem: (id) item
{
	if (count == size) {
		size += incr;
		array = (id [])obj_realloc (array, size * @sizeof (id));
	}
	array[count++] = item;
	[item retain];
}

- (void) removeItem: (id) item
{
	local integer i, n;

	for (i = 0; i < count; i++)
		if (array[i] == item) {
			[item release];
			count--;
			for (n = i; n < count; n++)
				array[n] = array[n + 1];
		}
	return;
}

- (id) removeItemAt: (integer) index
{
	local integer i;
	local id item;

	if (index == -1)
		index = count -1;
	if (index < 0 || index >= count)
		return NIL;
	item = array[index];
	count--;
	for (i = index; i < count; i++)
		array[i] = array[i + 1];
	[item release];
	return item;
}

- (id) insertItemAt: (integer) index item:(id) item
{
	local integer i;
	if (index == -1)
		index = count -1;
	if (index < 0 || index >= count)
		return NIL;
	if (count == size) {
		size += incr;
		array = (id [])obj_realloc (array, size * @sizeof (id));
	}
	for (i = count; i > index; i--)
		array[i] = array[i - 1];
	array[index] = item;
	count++;
	[item retain];
	return item;
}

- (integer) count
{
	return count;
}

-(void)makeObjectsPerformSelector:(SEL)selector
{
	local integer i;
	for (i = 0; i < count; i++)
		[array[i] performSelector:selector];
}

-(void)makeObjectsPerformSelector:(SEL)selector withObject:(id)arg
{
	local integer i;
	for (i = 0; i < count; i++)
		[array[i] performSelector:selector withObject:arg];
}

@end
