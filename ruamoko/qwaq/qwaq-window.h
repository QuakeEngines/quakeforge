#ifndef __qwaq_window_h
#define __qwaq_window_h

#include "Object.h"

@class Array;

#include "qwaq-draw.h"
#include "qwaq-rect.h"
#include "qwaq-view.h"
#include "qwaq-group.h"

@interface Window: Group
{
	Point       point;	// FIXME can't be local :(
	struct panel_s *panel;
	DrawBuffer *buf;
}
+windowWithRect: (Rect) rect;
-setBackground: (int) ch;
@end

#endif//__qwaq_window_h
