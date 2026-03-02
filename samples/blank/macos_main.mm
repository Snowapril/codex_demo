#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

@interface BlankRenderer : NSObject
- (instancetype)initWithLayer:(CAMetalLayer*)layer;
- (void)render;
@end

@implementation BlankRenderer {
  id<MTLDevice> _device;
  id<MTLCommandQueue> _commandQueue;
  CAMetalLayer* _layer;
}

- (instancetype)initWithLayer:(CAMetalLayer*)layer {
  self = [super init];
  if (self) {
    _device = MTLCreateSystemDefaultDevice();
    _commandQueue = [_device newCommandQueue];
    _layer = layer;
    _layer.device = _device;
    _layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    _layer.framebufferOnly = YES;
  }
  return self;
}

- (void)render {
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

    id<MTLCommandBuffer> cmd = [_commandQueue commandBuffer];
    id<MTLRenderCommandEncoder> encoder =
        [cmd renderCommandEncoderWithDescriptor:pass];
    [encoder endEncoding];
    [cmd presentDrawable:drawable];
    [cmd commit];
  }
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AppDelegate {
  NSWindow* _window;
  CAMetalLayer* _metalLayer;
  BlankRenderer* _renderer;
  NSTimer* _timer;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
  NSRect frame = NSMakeRect(100, 100, 800, 600);
  _window = [[NSWindow alloc]
      initWithContentRect:frame
                styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                           NSWindowStyleMaskResizable)
                  backing:NSBackingStoreBuffered
                    defer:NO];
  [_window setTitle:@"Blank Metal Sample"];
  [_window makeKeyAndOrderFront:nil];

  NSView* contentView = [_window contentView];
  contentView.wantsLayer = YES;

  _metalLayer = [CAMetalLayer layer];
  _metalLayer.frame = contentView.bounds;
  _metalLayer.contentsScale = NSScreen.mainScreen.backingScaleFactor;
  contentView.layer = _metalLayer;

  _renderer = [[BlankRenderer alloc] initWithLayer:_metalLayer];

  _timer = [NSTimer scheduledTimerWithTimeInterval:(1.0 / 60.0)
                                            target:self
                                          selector:@selector(onTick)
                                          userInfo:nil
                                           repeats:YES];
}

- (void)onTick {
  _metalLayer.frame = _window.contentView.bounds;
  [_renderer render];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
  return YES;
}

@end

int main(int argc, const char* argv[]) {
  @autoreleasepool {
    NSApplication* app = [NSApplication sharedApplication];
    AppDelegate* delegate = [[AppDelegate alloc] init];
    app.delegate = delegate;
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];
    [app activateIgnoringOtherApps:YES];
    [app run];
  }
  return 0;
}
