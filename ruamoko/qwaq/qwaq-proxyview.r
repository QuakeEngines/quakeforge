#include "qwaq-proxyview.h"

@implementation ProxyView
- (void) forward: (SEL) sel : (@va_list) args
{
	if (!view) {
		return;
	}
	obj_msg_sendv (view, sel, args);
}

-initWithView:(View *) view
{
	if (!(self = [super init])) {
		return nil;
	}
	self.view = view;
	return self;
}

-setView:(View *) view
{
	int         state = [self.view state];
	if (state & sfInFocus) {
		[self.view loseFocus];
		[self.view hide];
	}

	self.view = view;
	if (state & sfDrawn) {
		[view draw];
	}
	if (state & sfInFocus) {
		[view takeFocus];
	}
	return self;
}
@end