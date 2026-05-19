#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

#include "WindowHelper.h"

namespace Quelos::Platform {
    QS_API void* GetNSViewFromWindow(void* nsWindowPtr) {
        NSWindow* win = (__bridge NSWindow*)nsWindowPtr;
        NSView* view  = [win contentView];

        [view setWantsLayer:YES];
        if (![view.layer isKindOfClass:[CAMetalLayer class]]) {
            view.layer = [CAMetalLayer layer];
        }

        return (__bridge void*)view;
    }
}
