#import <UIKit/UIKit.h>

#import "Overlay/Overlay.h"

// Brings the overlay up once the app has finished launching, and wires the
// gestures that show and hide the menu. This is the only Objective-C entry point;
// the rest of the tweak is plain C++ behind it.

@interface Loader : NSObject
@end

@implementation Loader

static Loader *gLoader = nil;
static Overlay *gOverlay = nil;

// A key window that is actually attached to a live scene. During launch the app
// can hand back a window whose scene is tearing down, and parenting to that is a
// good way to crash on a stale view.
+ (UIWindow *)keyWindow
{
    for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
        if (![scene isKindOfClass:UIWindowScene.class]) {
            continue;
        }
        for (UIWindow *window in ((UIWindowScene *)scene).windows) {
            if (window.isKeyWindow) {
                return window;
            }
        }
    }
    return UIApplication.sharedApplication.windows.firstObject;
}

+ (void)attach
{
    UIWindow *window = [self keyWindow];
    if (window == nil || window.windowScene == nil) {
        return;
    }
    if (gOverlay == nil) {
        gOverlay = [[Overlay alloc] init];
    }
    if (gOverlay.view.superview != window) {
        [window addSubview:gOverlay.view];
    }

    // Three-finger double tap shows the menu, two-finger double tap hides it. The
    // gestures live on the game's own view so they fire wherever you touch.
    UITapGestureRecognizer *show =
        [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(showMenu)];
    show.numberOfTapsRequired = 2;
    show.numberOfTouchesRequired = 3;
    [window addGestureRecognizer:show];

    UITapGestureRecognizer *hide =
        [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(hideMenu)];
    hide.numberOfTapsRequired = 2;
    hide.numberOfTouchesRequired = 2;
    [window addGestureRecognizer:hide];
}

+ (void)showMenu
{
    [Overlay setMenuVisible:YES];
}

+ (void)hideMenu
{
    [Overlay setMenuVisible:NO];
}

+ (void)load
{
    // Give the app a few seconds to build its UI, then attach on the main thread.
    // A fixed delay is crude but it keeps the loader tiny and it is plenty for a
    // game that spends its first seconds on a splash screen anyway.
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(4 * NSEC_PER_SEC)),
                   dispatch_get_main_queue(), ^{
                       gLoader = [Loader new];
                       [Loader attach];
                   });
}

@end
