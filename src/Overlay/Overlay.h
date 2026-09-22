#pragma once

#import <UIKit/UIKit.h>

// The transparent Metal view that hosts ImGui and draws the overlay on top of the
// game. It owns the render loop; the menu and the skeleton are drawn from inside
// its per-frame callback.
@interface Overlay : UIViewController

// Show or hide the menu window. Touches only reach the overlay while the menu is
// up, so the game stays playable the rest of the time.
+ (void)setMenuVisible:(BOOL)visible;

@end
