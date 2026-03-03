#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalDisplayLink.h>
#import <QuartzCore/CAMetalLayer.h>

#include <memory>

#include "reng/app.h"
#include "reng/engine.h"
#include "reng/logger.h"
#include "engine/src/backends/metal/metal_device.h"
#include "engine/src/backends/metal/metal_swapchain.h"

@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AppDelegate {
  NSWindow* _window;
  CAMetalLayer* _layer;
  CAMetalDisplayLink* _displayLink;
  std::unique_ptr<reng::MetalDevice> _device;
  std::unique_ptr<reng::MetalSwapchain> _swapchain;
  std::unique_ptr<reng::Engine> _engine;
  reng::AppCallbacks* _callbacks;
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
  reng::RengLogger::logInfo("Starting macOS app");
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

  _device = std::make_unique<reng::MetalDevice>();
  _swapchain = std::make_unique<reng::MetalSwapchain>(
      _layer, *_device, _desc.swapchain);
  _engine = std::make_unique<reng::Engine>(_desc, *_callbacks, _swapchain.get());

  _displayLink = [CAMetalDisplayLink displayLinkWithMetalLayer:_layer];
  _displayLink.preferredFrameRateRange = CAFrameRateRangeMake(30, 60, 60);
  __weak AppDelegate* weakSelf = self;
  _displayLink.callback = ^(CAMetalDisplayLink* link) {
    [weakSelf tick];
  };
  [_displayLink addToRunLoop:[NSRunLoop mainRunLoop]
                     forMode:NSRunLoopCommonModes];
}

- (void)applicationWillTerminate:(NSNotification*)notification {
  reng::RengLogger::logInfo("Shutting down macOS app");
  reng::RengLogger::shutdown();
}

- (void)tick {
  CFTimeInterval now = CACurrentMediaTime();
  float delta = (float)(now - _lastTime);
  _lastTime = now;

  _engine->tick(delta);
  if (_callbacks->shouldExit()) {
    [[NSApplication sharedApplication] terminate:nil];
  }
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
