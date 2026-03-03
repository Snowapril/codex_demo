#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalDisplayLink.h>
#import <QuartzCore/CAMetalLayer.h>

#include "reng/app.h"

namespace {
class MetalPresenter {
 public:
  MetalPresenter(CAMetalLayer* layer, reng::PixelFormat format)
      : _layer(layer) {
    _device = MTLCreateSystemDefaultDevice();
    _queue = [_device newCommandQueue];
    _layer.device = _device;
    _layer.pixelFormat = format == reng::PixelFormat::Bgra8Unorm
                             ? MTLPixelFormatBGRA8Unorm
                             : MTLPixelFormatBGRA8Unorm;
    _layer.framebufferOnly = YES;
  }

  void present() {
    @autoreleasepool {
      id<CAMetalDrawable> drawable = [_layer nextDrawable];
      if (!drawable) {
        return;
      }
      MTLRenderPassDescriptor* pass =
          [MTLRenderPassDescriptor renderPassDescriptor];
      pass.colorAttachments[0].texture = drawable.texture;
      pass.colorAttachments[0].loadAction = MTLLoadActionClear;
      pass.colorAttachments[0].storeAction = MTLStoreActionStore;
      pass.colorAttachments[0].clearColor =
          MTLClearColorMake(0.05, 0.05, 0.08, 1.0);

      id<MTLCommandBuffer> cmd = [_queue commandBuffer];
      id<MTLRenderCommandEncoder> encoder =
          [cmd renderCommandEncoderWithDescriptor:pass];
      [encoder endEncoding];
      [cmd presentDrawable:drawable];
      [cmd commit];
    }
  }

 private:
  id<MTLDevice> _device = nil;
  id<MTLCommandQueue> _queue = nil;
  CAMetalLayer* _layer = nil;
};
}  // namespace

@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AppDelegate {
  NSWindow* _window;
  CAMetalLayer* _layer;
  CAMetalDisplayLink* _displayLink;
  MetalPresenter* _presenter;
  reng::AppCallbacks* _callbacks;
  reng::RenderGraph _graph;
  reng::AppDesc _desc;
  CFTimeInterval _lastTime;
}

- (instancetype)initWithDesc:(const reng::AppDesc&)desc
                   callbacks:(reng::AppCallbacks*)callbacks {
  self = [super init];
  if (self) {
    _callbacks = callbacks;
    _desc = desc;
    _lastTime = CACurrentMediaTime();
  }
  return self;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
  NSRect frame =
      NSMakeRect(100, 100, _desc.swapchain.width, _desc.swapchain.height);
  _window = [[NSWindow alloc]
      initWithContentRect:frame
                styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                           NSWindowStyleMaskResizable)
                  backing:NSBackingStoreBuffered
                    defer:NO];
  NSString* title = _desc.title ? [NSString stringWithUTF8String:_desc.title]
                                : @"Blank Sample";
  [_window setTitle:title];
  [_window makeKeyAndOrderFront:nil];

  NSView* content = [_window contentView];
  content.wantsLayer = YES;

  _layer = [CAMetalLayer layer];
  _layer.frame = content.bounds;
  _layer.contentsScale = NSScreen.mainScreen.backingScaleFactor;
  content.layer = _layer;

  _presenter = new MetalPresenter(_layer, _desc.swapchain.colorFormat);

  _displayLink = [CAMetalDisplayLink displayLinkWithMetalLayer:_layer];
  _displayLink.preferredFrameRateRange = CAFrameRateRangeMake(30, 60, 60);
  __weak AppDelegate* weakSelf = self;
  _displayLink.callback = ^(CAMetalDisplayLink* link) {
    [weakSelf tick];
  };
  [_displayLink addToRunLoop:[NSRunLoop mainRunLoop]
                     forMode:NSRunLoopCommonModes];
}

- (void)tick {
  CFTimeInterval now = CACurrentMediaTime();
  float delta = (float)(now - _lastTime);
  _lastTime = now;

  _callbacks->onInput();
  _callbacks->onUpdateFrame(delta);
  _graph.beginFrame();
  _callbacks->onUpdateRender(_graph);
  _callbacks->onRender(_graph);
  _graph.compile();
  _graph.resolve().execute();

  _presenter->present();
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
  return YES;
}

@end

namespace reng {

int runAppPlatform(const AppDesc& desc, AppCallbacks& callbacks) {
  if (desc.backend != Backend::Metal) {
    return 1;
  }
  @autoreleasepool {
    NSApplication* app = [NSApplication sharedApplication];
    AppDelegate* delegate = [[AppDelegate alloc] initWithDesc:desc
                                                    callbacks:&callbacks];
    app.delegate = delegate;
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];
    [app activateIgnoringOtherApps:YES];
    [app run];
  }
  return 0;
}

}  // namespace reng
