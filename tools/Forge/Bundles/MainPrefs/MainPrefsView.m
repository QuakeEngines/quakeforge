/*
	MainPrefsView.m

	Forge internal preferences view

	Copyright (C) 2001 Dusk to Dawn Computing, Inc.

	Author: Jeff Teunissen <deek@d2dc.net>
	Date:	17 Nov 2001

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public
	License along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA
*/
static const char rcsid[] = 
	"$Id$";

#ifdef HAVE_CONFIG_H
# include "Config.h"
#endif

#import <AppKit/NSBezierPath.h>
#import <AppKit/NSButton.h>
#import <AppKit/NSColor.h>

#import "MainPrefsView.h"

@implementation MainPrefsView

- (id) initWithFrame: (NSRect) frameRect
{
	id button;

	if ((self = [super initWithFrame: frameRect])) {

		button = [[NSButton alloc] initWithFrame: NSMakeRect (0, 0, 60, 24)];
		[button autorelease];

		[button setTitle: @"Default"];
		[button setTarget: self];
		[button setAction: @selector(resetToDefaults:)];
		[self addSubview: button];
	}
	return self;
}
@end
