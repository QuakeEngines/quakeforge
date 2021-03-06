#ifndef __qwaq_debugger_entityview_h
#define __qwaq_debugger_entityview_h

#include "ruamoko/qwaq/debugger/views/defview.h"

@interface EntityView : DefView
{
	entity     *data;
}
+(EntityView *)withType:(qfot_type_t *)type at:(unsigned)offset in:(void *)data;
@end

#endif//__qwaq_debugger_entityview_h
