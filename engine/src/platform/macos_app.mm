#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalDisplayLink.h>
#import <QuartzCore/CAMetalLayer.h>
#import <dispatch/dispatch.h>

#include <memory>

#include "reng/app.h"
#include "reng/engine.h"
#include "reng/logger.h"
#include "reng/platform.h"

@interface AppDelegate : NSObject <NSApplicationDelegate, CAMetalDisplayLinkDelegate>
@end

@implementation AppDelegate {
  NSWindow* _window;
  CAMetalLayer* _layer;
  CAMetalDisplayLink* _displayLink;
  NSTimer* _fallbackTimer;
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

  reng::PlatformContext context;
  context.platform = reng::PlatformKind::MacOS;
  context.macos.nsWindow = (__bridge void*)_window;
  context.macos.metalLayer = (__bridge void*)_layer;
  _engine = reng::Engine::create(_desc, *_callbacks, context);
  if (!_engine) {
    reng::RengLogger::logError("Failed to initialize engine");
    [[NSApplication sharedApplication] terminate:nil];
    return;
  }

  if (@available(macOS 14.0, *)) {
    _displayLink = [[CAMetalDisplayLink alloc] initWithMetalLayer:_layer];
    _displayLink.preferredFrameRateRange = CAFrameRateRangeMake(30, 60, 60);
    _displayLink.delegate = self;
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop]
                       forMode:NSRunLoopCommonModes];
  } else {
    _fallbackTimer = [NSTimer
        scheduledTimerWithTimeInterval:(1.0 / 60.0)
                                target:self
                              selector:@selector(tick)
                              userInfo:nil
                               repeats:YES];
  }

  if (_desc.maxRunSeconds > 0.0f) {
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,
                                 (int64_t)(_desc.maxRunSeconds * NSEC_PER_SEC)),
                   dispatch_get_main_queue(), ^{
                     [[NSApplication sharedApplication] terminate:nil];
                   });
  }
}

- (void)metalDisplayLink:(CAMetalDisplayLink*)link
             needsUpdate:(CAMetalDisplayLinkUpdate*)update {
  (void)link;
  (void)update;
  [self tick];
}

- (void)applicationWillTerminate:(NSNotification*)notification {
  reng::RengLogger::logInfo("Shutting down macOS app");
  if (_displayLink) {
    [_displayLink invalidate];
    _displayLink = nil;
  }
  if (_fallbackTimer) {
    [_fallbackTimer invalidate];
    _fallbackTimer = nil;
  }
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
