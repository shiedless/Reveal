#import "Overlay/Overlay.h"

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_metal.h"
#include "Render/Skeleton.hpp"

// Whether the menu is up. Static because the loader flips it from a gesture
// handler, and the render loop reads it every frame.
static bool gMenuVisible = false;

@interface Overlay () <MTKViewDelegate>
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@end

@implementation Overlay

- (instancetype)init
{
    self = [super initWithNibName:nil bundle:nil];
    if (self == nil) {
        return nil;
    }

    _device = MTLCreateSystemDefaultDevice();
    _commandQueue = [_device newCommandQueue];
    if (_device == nil || _commandQueue == nil) {
        return nil;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplMetal_Init(_device);
    return self;
}

+ (void)setMenuVisible:(BOOL)visible
{
    gMenuVisible = visible;
}

- (void)loadView
{
    // Fill the key window and stay transparent, so the game shows through and only
    // our lines and menu are drawn.
    CGRect frame = UIScreen.mainScreen.bounds;
    MTKView *view = [[MTKView alloc] initWithFrame:frame device:self.device];
    view.clearColor = MTLClearColorMake(0, 0, 0, 0);
    view.backgroundColor = UIColor.clearColor;
    view.opaque = NO;
    view.layer.opaque = NO;
    self.view = view;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    MTKView *view = (MTKView *)self.view;
    view.delegate = self;
    view.preferredFramesPerSecond = 60;
}

- (void)feedTouches:(UIEvent *)event
{
    ImGuiIO &io = ImGui::GetIO();
    UITouch *touch = event.allTouches.anyObject;
    CGPoint p = [touch locationInView:self.view];
    io.MousePos = ImVec2(p.x, p.y);

    bool down = false;
    for (UITouch *t in event.allTouches) {
        if (t.phase != UITouchPhaseEnded && t.phase != UITouchPhaseCancelled) {
            down = true;
            break;
        }
    }
    io.MouseDown[0] = down;
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { [self feedTouches:event]; }
- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { [self feedTouches:event]; }
- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { [self feedTouches:event]; }
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { [self feedTouches:event]; }

- (void)drawInMTKView:(MTKView *)view
{
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(view.bounds.size.width, view.bounds.size.height);
    const CGFloat scale = view.window.screen.scale ?: UIScreen.mainScreen.scale;
    io.DisplayFramebufferScale = ImVec2(scale, scale);
    io.DeltaTime = 1.0f / 60.0f;

    // The overlay only wants touches while the menu is showing; otherwise they
    // pass through to the game.
    self.view.userInteractionEnabled = gMenuVisible;

    MTLRenderPassDescriptor *pass = view.currentRenderPassDescriptor;
    if (pass == nil) {
        return;
    }
    id<MTLCommandBuffer> buffer = [self.commandQueue commandBuffer];
    id<MTLRenderCommandEncoder> encoder = [buffer renderCommandEncoderWithDescriptor:pass];

    ImGui_ImplMetal_NewFrame(pass);
    ImGui::NewFrame();

    // The skeletons go on the background draw list so they sit under the menu.
    render::DrawSkeletons(ImGui::GetBackgroundDrawList(), io.DisplaySize.x, io.DisplaySize.y);

    if (gMenuVisible) {
        ImGui::SetNextWindowSize(ImVec2(300, 140), ImGuiCond_FirstUseEver);
        ImGui::Begin("Skeleton ESP");
        ImGui::Text("Skeleton overlay is running.");
        ImGui::Text("%.0f FPS", io.Framerate);
        ImGui::Text("Two-finger double tap to hide.");
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), buffer, encoder);

    [encoder endEncoding];
    [buffer presentDrawable:view.currentDrawable];
    [buffer commit];
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size
{
}

@end
