#import <Cocoa/Cocoa.h>
#include "field_view.h"
#include <math.h>
#define WIDTH 800
#define HEIGHT 600
@interface FieldCanvas : NSView
@property HGFieldView *field;
@property DSCamera camera;
@property(strong) NSBitmapImageRep *bitmap;
@property(strong) NSImage *image;
@property NSPoint dragPoint;
@property NSUInteger renders;
- (BOOL)render;
- (void)resetCamera;
@end
@implementation FieldCanvas
- (instancetype)initWithFrame:(NSRect)rect {
    if(!(self=[super initWithFrame:rect]))return nil;
    self.field=hg_field_view_new(60);if(!self.field)return nil;
    self.bitmap=[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL pixelsWide:WIDTH pixelsHigh:HEIGHT bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:WIDTH*4 bitsPerPixel:32];
    if(!self.bitmap)return nil;[self resetCamera];if(![self render])return nil;return self;
}
- (void)dealloc { hg_field_view_free(self.field); }
- (BOOL)acceptsFirstResponder { return YES; }
- (void)resetCamera { DSCamera c;hg_field_view_camera(self.field,&c,WIDTH,HEIGHT);self.camera=c; }
- (BOOL)render {
    DSCamera c=self.camera;if(!hg_field_view_render(self.field,&c,self.bitmap.bitmapData,WIDTH,HEIGHT))return NO;
    self.image=[[NSImage alloc] initWithSize:NSMakeSize(WIDTH,HEIGHT)];[self.image addRepresentation:self.bitmap];self.renders++;[self setNeedsDisplay:YES];return YES;
}
- (void)drawRect:(NSRect)dirty {
    (void)dirty;NSGraphicsContext.currentContext.imageInterpolation=NSImageInterpolationNone;
    [self.image drawInRect:self.bounds fromRect:NSZeroRect operation:NSCompositingOperationCopy fraction:1];
}
- (void)keyDown:(NSEvent *)event {
    DSCamera c=self.camera;NSString *key=event.charactersIgnoringModifiers.lowercaseString;BOOL changed=YES;
    switch(event.keyCode) {
    case 123:c.yaw-=0.12;break;case 124:c.yaw+=0.12;break;
    case 125:c.pitch=fmax(0.1,c.pitch-0.08);break;case 126:c.pitch=fmin(1.5,c.pitch+0.08);break;
    default:
        if([key isEqualToString:@"w"])c.pan_y+=24;
        else if([key isEqualToString:@"s"])c.pan_y-=24;
        else if([key isEqualToString:@"a"])c.pan_x+=24;
        else if([key isEqualToString:@"d"])c.pan_x-=24;
        else if([key isEqualToString:@"+"] || [key isEqualToString:@"="])c.scale=fmin(20,c.scale*1.15);
        else if([key isEqualToString:@"-"])c.scale=fmax(0.02,c.scale/1.15);
        else if([key isEqualToString:@"r"]) { [self resetCamera];c=self.camera; }
        else changed=NO;
    }
    if(changed) { c.pan_x=fmax(-2000,fmin(2000,c.pan_x));c.pan_y=fmax(-2000,fmin(2000,c.pan_y));self.camera=c;if(![self render])exit(1); }else [super keyDown:event];
}
- (void)scrollWheel:(NSEvent *)event {
    DSCamera c=self.camera;c.scale=fmax(0.02,fmin(20,c.scale*exp(-event.scrollingDeltaY*0.025)));self.camera=c;if(![self render])exit(1);
}
- (void)mouseDown:(NSEvent *)event { self.dragPoint=event.locationInWindow;[self.window makeFirstResponder:self]; }
- (void)mouseDragged:(NSEvent *)event {
    NSPoint p=event.locationInWindow;DSCamera c=self.camera;double x=p.x-self.dragPoint.x,y=p.y-self.dragPoint.y;
    if(event.modifierFlags&NSEventModifierFlagShift) { c.pan_x=fmax(-2000,fmin(2000,c.pan_x+x));c.pan_y=fmax(-2000,fmin(2000,c.pan_y-y)); }
    else { c.yaw+=x*0.008;c.pitch=fmax(0.1,fmin(1.5,c.pitch+y*0.008)); }
    self.camera=c;self.dragPoint=p;if(![self render])exit(1);
}
@end
@interface FieldApp : NSObject <NSApplicationDelegate>
@property(strong) NSWindow *window;
@property BOOL smoke;
@end
@implementation FieldApp
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app { (void)app;return YES; }
- (void)applicationDidFinishLaunching:(NSNotification *)note {
    (void)note;FieldCanvas *canvas=[[FieldCanvas alloc] initWithFrame:NSMakeRect(0,0,WIDTH,HEIGHT)];if(!canvas) { fprintf(stderr,"Cannot load New Bark field resources\n");exit(1); }
    self.window=[[NSWindow alloc] initWithContentRect:canvas.frame styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable|NSWindowStyleMaskMiniaturizable backing:NSBackingStoreBuffered defer:NO];
    self.window.title=@"HeartGold MX — New Bark land • Drag/arrows orbit · Scroll ± zoom · WASD/shift-drag pan · R reset";
    self.window.contentView=canvas;self.window.contentAspectRatio=NSMakeSize(WIDTH,HEIGHT);self.window.contentMinSize=NSMakeSize(640,480);
    [self.window center];[self.window makeKeyAndOrderFront:nil];[self.window makeFirstResponder:canvas];[NSApp activateIgnoringOtherApps:YES];
    if(self.smoke)dispatch_after(dispatch_time(DISPATCH_TIME_NOW,300*NSEC_PER_MSEC),dispatch_get_main_queue(),^{
        NSData *before=[NSData dataWithBytes:canvas.bitmap.bitmapData length:WIDTH*HEIGHT*4];
        NSEvent *key=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"" charactersIgnoringModifiers:@"" isARepeat:NO keyCode:124];
        [canvas keyDown:key];NSData *after=[NSData dataWithBytes:canvas.bitmap.bitmapData length:WIDTH*HEIGHT*4];
        if([before isEqual:after] || canvas.renders<2)exit(1);
        [canvas display];fprintf(stderr,"Field viewer smoke: resource load, camera key event, changed framebuffer, AppKit display passed\n");[NSApp terminate:nil];
    });
}
@end
int main(int argc,const char **argv) {
    @autoreleasepool {
        if(argc==3 && !strcmp(argv[1],"--render-only")) {
            FieldCanvas *canvas=[[FieldCanvas alloc] initWithFrame:NSMakeRect(0,0,WIDTH,HEIGHT)];if(!canvas)return 1;
            NSData *png=[canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
            return [png writeToFile:[NSString stringWithUTF8String:argv[2]] atomically:YES]?0:1;
        }
        if(argc!=1 && (argc!=2 || strcmp(argv[1],"--smoke"))) { fprintf(stderr,"Usage: heartgold_field_viewer [--smoke | --render-only path.png]\n");return 2; }
        NSApplication *app=[NSApplication sharedApplication];[app setActivationPolicy:NSApplicationActivationPolicyRegular];
        NSMenu *main=[NSMenu new],*menu=[NSMenu new];NSMenuItem *root=[NSMenuItem new];[main addItem:root];root.submenu=menu;[menu addItemWithTitle:@"Quit HeartGold MX Field" action:@selector(terminate:) keyEquivalent:@"q"];app.mainMenu=main;
        FieldApp *delegate=[FieldApp new];delegate.smoke=argc==2;app.delegate=delegate;[app run];
    }return 0;
}
