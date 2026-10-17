#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>
#import <GameController/GameController.h>
#include "inventory.h"
#include "game_scene.h"
#include "compat/scheduler.h"

@interface Renderer : NSObject <MTKViewDelegate>
@property(strong) id<MTLCommandQueue> queue;
@property(strong) id<MTLRenderPipelineState> pipeline;
@property(strong) id<MTLTexture> texture;
@property(strong) id<MTLCommandBuffer> lastCommand;
@property BOOL verifyDraw;
@property NSUInteger completedDraws;
- (instancetype)initWithDevice:(id<MTLDevice>)device;
- (void)upload:(const uint8_t *)pixels;
- (void)resizeWidth:(NSUInteger)width height:(NSUInteger)height device:(id<MTLDevice>)device;
@end
@implementation Renderer
- (instancetype)initWithDevice:(id<MTLDevice>)device {
    if(!(self=[super init]))return nil;
    NSString *source=@"#include <metal_stdlib>\nusing namespace metal; struct V {float4 p [[position]];float2 uv;};vertex V vs(uint i [[vertex_id]]){float2 p[3]={float2(-1,-1),float2(3,-1),float2(-1,3)};V v;v.p=float4(p[i],0,1);v.uv=float2((p[i].x+1)/2,1-(p[i].y+1)/2);return v;}fragment float4 fs(V v [[stage_in]],texture2d<float> tex [[texture(0)]]){constexpr sampler s(filter::nearest,address::clamp_to_edge);float4 c=tex.sample(s,v.uv);return float4(mix(float3(0.10,0.13,0.18),c.rgb,c.a),1);}";
    NSError *error=nil;
    id<MTLLibrary> library=[device newLibraryWithSource:source options:nil error:&error];
    MTLRenderPipelineDescriptor *desc=[MTLRenderPipelineDescriptor new];
    desc.vertexFunction=[library newFunctionWithName:@"vs"];desc.fragmentFunction=[library newFunctionWithName:@"fs"];desc.colorAttachments[0].pixelFormat=MTLPixelFormatBGRA8Unorm;
    self.pipeline=library?[device newRenderPipelineStateWithDescriptor:desc error:&error]:nil;
    self.queue=[device newCommandQueue];
    MTLTextureDescriptor *td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:32 height:32 mipmapped:NO];
    self.texture=[device newTextureWithDescriptor:td];
    if(!self.pipeline || !self.queue || !self.texture) { fprintf(stderr,"Metal initialization: %s\n",error.description.UTF8String);return nil; }
    return self;
}
- (void)resizeWidth:(NSUInteger)width height:(NSUInteger)height device:(id<MTLDevice>)device {
    [self.lastCommand waitUntilCompleted];
    MTLTextureDescriptor *td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:width height:height mipmapped:NO];
    self.texture=[device newTextureWithDescriptor:td];if(!self.texture)exit(1);
}
- (void)upload:(const uint8_t *)pixels { [self.lastCommand waitUntilCompleted];[self.texture replaceRegion:MTLRegionMake2D(0,0,self.texture.width,self.texture.height) mipmapLevel:0 withBytes:pixels bytesPerRow:self.texture.width*4]; }
- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size { (void)view;(void)size; }
- (void)drawInMTKView:(MTKView *)view {
    MTLRenderPassDescriptor *pass=view.currentRenderPassDescriptor;id<CAMetalDrawable> drawable=view.currentDrawable;
    if(!pass || !drawable)return;
    id<MTLCommandBuffer> command=[self.queue commandBuffer];
    id<MTLRenderCommandEncoder> enc=[command renderCommandEncoderWithDescriptor:pass];
    double fit=fmin(view.drawableSize.width/self.texture.width,view.drawableSize.height/self.texture.height);
    double scale=fit>=1?floor(fit):fit;
    double width=self.texture.width*scale,height=self.texture.height*scale;
    [enc setViewport:(MTLViewport){(view.drawableSize.width-width)/2,(view.drawableSize.height-height)/2,width,height,0,1}];
    [enc setRenderPipelineState:self.pipeline];[enc setFragmentTexture:self.texture atIndex:0];
    [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];[enc endEncoding];
    [command presentDrawable:drawable];self.lastCommand=command;[command commit];
    if(self.verifyDraw) { [command waitUntilCompleted];if(command.status!=MTLCommandBufferStatusCompleted) { fprintf(stderr,"GPU draw failed\n");exit(1); }self.completedDraws++; }
}
@end

/* Convert pointer coordinates through the same integer viewport used by Metal. */
@interface SceneView : MTKView
@property(copy) void (^hitHandler)(unsigned,unsigned);
- (CGFloat)pixelScale;
@end
@implementation SceneView
- (CGFloat)pixelScale {
    double fit=fmin(self.drawableSize.width/HG_SCENE_WIDTH,self.drawableSize.height/HG_SCENE_HEIGHT);
    double scale=fit>=1?floor(fit):fit;
    return scale*self.bounds.size.width/self.drawableSize.width;
}
- (void)mouseDown:(NSEvent *)event {
    NSPoint point=[self convertPoint:event.locationInWindow fromView:nil];CGFloat scale=[self pixelScale];if(scale<=0)return;
    CGFloat x=(point.x-(self.bounds.size.width-HG_SCENE_WIDTH*scale)/2)/scale;
    CGFloat y=(self.bounds.size.height-point.y-(self.bounds.size.height-HG_SCENE_HEIGHT*scale)/2)/scale;
    if(x>=0 && y>=0 && x<HG_SCENE_WIDTH && y<HG_SCENE_HEIGHT && self.hitHandler)self.hitHandler((unsigned)x,(unsigned)y);
}
@end

@interface AppDelegate : NSObject <NSApplicationDelegate,NSTableViewDataSource,NSTableViewDelegate>
@property(strong) NSWindow *window;
@property(strong) Renderer *renderer;
@property(strong) MTKView *metal;
@property(strong) NSTableView *table;
@property(strong) NSSegmentedControl *pockets;
@property(strong) NSTextField *itemName,*detail,*quantity,*summary,*status;
@property(strong) NSButton *removeButton;
@property(strong) id keyMonitor;
@property(strong) id controllerObserver;
@property HGInventory *inventory;
@property HGScene *scene;
@property(strong) SceneView *classicView;
@property(strong) Renderer *classicRenderer;
@property(strong) NSMutableData *scenePixels;
@property(strong) NSArray<NSView *> *listViews;
@property(strong) NSSegmentedControl *presentation;
@property(strong) NSTimer *sceneTimer;
@property uint64_t animationStart;
@property NSInteger lastTick;
@property BOOL smoke,syncing;
- (void)refresh;
- (void)performAction:(enum HGAction)action;
@end
@implementation AppDelegate
- (NSTextField *)label:(NSString *)text frame:(NSRect)frame size:(CGFloat)size bold:(BOOL)bold {
    NSTextField *label=[NSTextField wrappingLabelWithString:text];label.frame=frame;label.font=bold?[NSFont boldSystemFontOfSize:size]:[NSFont systemFontOfSize:size];
    label.autoresizingMask=NSViewMinYMargin;[self.window.contentView addSubview:label];return label;
}
- (NSButton *)button:(NSString *)title action:(SEL)action frame:(NSRect)frame {
    NSButton *b=[NSButton buttonWithTitle:title target:self action:action];b.frame=frame;[self.window.contentView addSubview:b];return b;
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app { (void)app;return YES; }
- (void)applicationWillTerminate:(NSNotification *)note {
    (void)note;if(self.keyMonitor)[NSEvent removeMonitor:self.keyMonitor];
    if(self.controllerObserver)[NSNotificationCenter.defaultCenter removeObserver:self.controllerObserver];
    [self.sceneTimer invalidate];hg_scene_free(self.scene);self.scene=NULL;hg_inventory_free(self.inventory);self.inventory=NULL;
    if(!hg_scheduler_shutdown())fprintf(stderr,"Outstanding scheduler tasks at shutdown\n");
}
- (void)connectController:(GCController *)controller {
    __weak AppDelegate *weakSelf=self;
    controller.extendedGamepad.dpad.valueChangedHandler=^(GCControllerDirectionPad *pad,float x,float y) {
        (void)pad;if(fabsf(x)<0.5f && fabsf(y)<0.5f)return;
        enum HGAction action=fabsf(y)>=fabsf(x)?(y>0?HG_UP:HG_DOWN):(x>0?HG_NEXT_POCKET:HG_PREVIOUS_POCKET);
        dispatch_async(dispatch_get_main_queue(),^{ if(weakSelf.window.keyWindow)[weakSelf performAction:action]; });
    };
    NSArray<GCControllerButtonInput *> *buttons=@[controller.extendedGamepad.buttonA,controller.extendedGamepad.buttonB,controller.extendedGamepad.leftShoulder,controller.extendedGamepad.rightShoulder];
    const enum HGAction actions[]={HG_ADD,HG_REMOVE,HG_PREVIOUS_POCKET,HG_NEXT_POCKET};
    for(unsigned i=0;i<buttons.count;i++) { enum HGAction action=actions[i];buttons[i].pressedChangedHandler=^(GCControllerButtonInput *b,float value,BOOL pressed) {
        (void)b;(void)value;if(pressed)dispatch_async(dispatch_get_main_queue(),^{ if(weakSelf.window.keyWindow)[weakSelf performAction:action]; });
    }; }
}
- (void)applicationDidFinishLaunching:(NSNotification *)note {
    (void)note;if(!hg_scheduler_init()) { fprintf(stderr,"Cannot initialize task queues\n");exit(1); }
    hg_scheduler_advance((uint64_t)(NSProcessInfo.processInfo.systemUptime*1000000000.0));
    self.inventory=hg_inventory_new();
    id<MTLDevice> device=MTLCreateSystemDefaultDevice();self.renderer=device?[[Renderer alloc] initWithDevice:device]:nil;
    if(!self.renderer)exit(1);self.renderer.verifyDraw=self.smoke;
    self.window=[[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,1000,660) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskResizable backing:NSBackingStoreBuffered defer:NO];
    self.window.title=@"HeartGold MX — Inventory";self.window.minSize=NSMakeSize(1000,680);
    self.window.appearance=[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    [self label:@"HeartGold MX" frame:NSMakeRect(24,604,600,32) size:26 bold:YES];
    self.summary=[self label:@"Inventory" frame:NSMakeRect(24,573,700,24) size:14 bold:NO];self.summary.textColor=NSColor.secondaryLabelColor;
    NSMutableArray *names=[NSMutableArray array];for(unsigned i=0;i<8;i++)[names addObject:@(hg_inventory_pocket_name(i))];
    self.pockets=[NSSegmentedControl segmentedControlWithLabels:names trackingMode:NSSegmentSwitchTrackingSelectOne target:self action:@selector(changePocket:)];
    self.pockets.frame=NSMakeRect(24,526,952,32);self.pockets.autoresizingMask=NSViewWidthSizable|NSViewMinYMargin;[self.window.contentView addSubview:self.pockets];
    NSScrollView *scroll=[[NSScrollView alloc] initWithFrame:NSMakeRect(24,110,560,396)];scroll.hasVerticalScroller=YES;scroll.borderType=NSBezelBorder;scroll.autoresizingMask=NSViewWidthSizable|NSViewHeightSizable;
    self.table=[[NSTableView alloc] initWithFrame:scroll.bounds];self.table.rowHeight=42;self.table.headerView=nil;self.table.allowsEmptySelection=NO;self.table.usesAlternatingRowBackgroundColors=YES;
    NSTableColumn *name=[[NSTableColumn alloc] initWithIdentifier:@"name"];name.width=440;name.minWidth=280;[self.table addTableColumn:name];
    NSTableColumn *qty=[[NSTableColumn alloc] initWithIdentifier:@"qty"];qty.width=90;qty.minWidth=70;qty.maxWidth=90;[self.table addTableColumn:qty];
    self.table.delegate=self;self.table.dataSource=self;scroll.documentView=self.table;[self.window.contentView addSubview:scroll];
    self.metal=[[MTKView alloc] initWithFrame:NSMakeRect(700,310,192,192) device:device];self.metal.colorPixelFormat=MTLPixelFormatBGRA8Unorm;self.metal.delegate=self.renderer;self.metal.paused=YES;self.metal.enableSetNeedsDisplay=YES;self.metal.autoresizingMask=NSViewMinXMargin|NSViewMinYMargin;[self.window.contentView addSubview:self.metal];
    self.itemName=[self label:@"" frame:NSMakeRect(616,257,360,36) size:24 bold:YES];
    self.quantity=[self label:@"" frame:NSMakeRect(616,225,360,24) size:14 bold:NO];
    self.detail=[self label:@"" frame:NSMakeRect(616,104,360,112) size:15 bold:NO];
    for(NSTextField *label in @[self.itemName,self.quantity,self.detail])label.autoresizingMask=NSViewMinXMargin|NSViewMinYMargin;
    [self button:@"Add one  +" action:@selector(addOne:) frame:NSMakeRect(24,64,140,32)];
    self.removeButton=[self button:@"Remove one  −" action:@selector(removeOne:) frame:NSMakeRect(176,64,160,32)];
    self.status=[self label:@"" frame:NSMakeRect(352,68,624,24) size:13 bold:NO];self.status.autoresizingMask=NSViewWidthSizable;
    NSTextField *help=[self label:@"↑ ↓ select   ·   ← → pockets   ·   + / − quantity   ·   controller: D-pad, A / B" frame:NSMakeRect(24,22,952,24) size:12 bold:NO];help.textColor=NSColor.secondaryLabelColor;help.autoresizingMask=NSViewWidthSizable;
    NSMenu *menu=[NSMenu new];NSMenuItem *appItem=[NSMenuItem new];[menu addItem:appItem];NSMenu *appMenu=[NSMenu new];[appMenu addItemWithTitle:@"Quit HeartGold MX" action:@selector(terminate:) keyEquivalent:@"q"];appItem.submenu=appMenu;NSApp.mainMenu=menu;
    __weak AppDelegate *weakSelf=self;
    self.keyMonitor=[NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown handler:^NSEvent *(NSEvent *event) {
        AppDelegate *app=weakSelf;if(event.window!=app.window || event.modifierFlags&(NSEventModifierFlagCommand|NSEventModifierFlagControl|NSEventModifierFlagOption))return event;
        int action=-1;
        switch(event.keyCode) { case 126:action=HG_UP;break;case 125:action=HG_DOWN;break;case 123:action=HG_PREVIOUS_POCKET;break;case 124:action=HG_NEXT_POCKET;break;case 116:action=HG_PAGE_UP;break;case 121:action=HG_PAGE_DOWN;break;case 36:action=HG_CONFIRM;break; }
        NSString *key=event.charactersIgnoringModifiers;
        if([key isEqualToString:@"+"]||[key isEqualToString:@"="])action=HG_ADD;
        if([key isEqualToString:@"-"])action=HG_REMOVE;
        if(action<0)return event;[app performAction:(enum HGAction)action];return nil;
    }];
    self.scene=hg_scene_new();if(!self.scene) { fprintf(stderr,"Font resource validation failed\n");exit(1); }self.scenePixels=[NSMutableData dataWithLength:HG_SCENE_BYTES];
    self.listViews=@[scroll,self.metal,self.itemName,self.quantity,self.detail];
    self.classicRenderer=[[Renderer alloc] initWithDevice:device];self.classicRenderer.verifyDraw=self.smoke;
    [self.classicRenderer resizeWidth:HG_SCENE_WIDTH height:HG_SCENE_HEIGHT device:device];
    self.classicView=[[SceneView alloc] initWithFrame:NSMakeRect(24,110,952,396) device:device];
    self.classicView.colorPixelFormat=MTLPixelFormatBGRA8Unorm;self.classicView.clearColor=MTLClearColorMake(0.025,0.035,0.05,1);
    self.classicView.delegate=self.classicRenderer;self.classicView.paused=YES;self.classicView.enableSetNeedsDisplay=YES;
    self.classicView.autoresizingMask=NSViewWidthSizable|NSViewHeightSizable;
    self.classicView.hitHandler=^(unsigned x,unsigned y) { AppDelegate *app=weakSelf;if(hg_scene_click(app.inventory,x,y))[app refresh]; };
    [self.window.contentView addSubview:self.classicView];
    self.presentation=[NSSegmentedControl segmentedControlWithLabels:@[@"Classic",@"List"] trackingMode:NSSegmentSwitchTrackingSelectOne target:self action:@selector(changePresentation:)];
    self.presentation.frame=NSMakeRect(800,604,176,28);self.presentation.autoresizingMask=NSViewMinXMargin|NSViewMinYMargin;self.presentation.selectedSegment=0;[self.window.contentView addSubview:self.presentation];
    [self.window setContentSize:NSMakeSize(1120,660)];[self changePresentation:nil];
    self.sceneTimer=[NSTimer timerWithTimeInterval:1.0/60.0 repeats:YES block:^(NSTimer *timer) { (void)timer;
        hg_scheduler_advance((uint64_t)(NSProcessInfo.processInfo.systemUptime*1000000000.0));
        [weakSelf drawScene]; }];
    [NSRunLoop.mainRunLoop addTimer:self.sceneTimer forMode:NSRunLoopCommonModes];
    for(GCController *controller in GCController.controllers)if(controller.extendedGamepad)[self connectController:controller];
    self.controllerObserver=[NSNotificationCenter.defaultCenter addObserverForName:GCControllerDidConnectNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *n) { GCController *c=n.object;if(c.extendedGamepad)[weakSelf connectController:c]; }];
    [self refresh];[self.window center];[self.window makeKeyAndOrderFront:nil];[NSApp activateIgnoringOtherApps:YES];
    if(self.smoke)dispatch_after(dispatch_time(DISPATCH_TIME_NOW,300*NSEC_PER_MSEC),dispatch_get_main_queue(),^{ [self smokeTest]; });
}
- (NSInteger)numberOfRowsInTableView:(NSTableView *)table { (void)table;return hg_inventory_count(self.inventory); }
- (NSView *)tableView:(NSTableView *)table viewForTableColumn:(NSTableColumn *)column row:(NSInteger)row {
    (void)table;HGItem item;if(!hg_inventory_item(self.inventory,(unsigned)row,&item))return nil;
    NSTextField *label=[NSTextField labelWithString:[column.identifier isEqualToString:@"name"]?@(item.name):[NSString stringWithFormat:@"× %u",item.quantity]];
    label.font=[NSFont systemFontOfSize:15 weight:NSFontWeightMedium];if([column.identifier isEqualToString:@"qty"])label.alignment=NSTextAlignmentRight;
    return label;
}
- (void)tableViewSelectionDidChange:(NSNotification *)note {
    (void)note;if(self.syncing || self.table.selectedRow<0)return;
    hg_inventory_select(self.inventory,(unsigned)self.table.selectedRow);[self refresh];
}
- (void)refresh {
    self.syncing=YES;[self.table reloadData];unsigned count=hg_inventory_count(self.inventory);
    if(count) { NSUInteger selected=hg_inventory_selected(self.inventory);[self.table selectRowIndexes:[NSIndexSet indexSetWithIndex:selected] byExtendingSelection:NO];[self.table scrollRowToVisible:selected]; }
    else [self.table deselectAll:nil];self.syncing=NO;
    self.pockets.selectedSegment=hg_inventory_pocket(self.inventory);
    self.summary.stringValue=[NSString stringWithFormat:@"Inventory / %s  ·  %u item types",hg_inventory_pocket_name(hg_inventory_pocket(self.inventory)),count];
    HGItem item;uint8_t pixels[4096]={0};
    if(hg_inventory_item(self.inventory,hg_inventory_selected(self.inventory),&item)) {
        self.itemName.stringValue=@(item.name);self.quantity.stringValue=[NSString stringWithFormat:@"Quantity  %u",item.quantity];
        self.detail.stringValue=*item.description?[@(item.description) stringByReplacingOccurrencesOfString:@"\n" withString:@" "]:@"No description available.";
        if(!hg_item_icon(item.id,pixels)) { fprintf(stderr,"Icon failed for item %u\n",item.id);exit(1); }
    } else { self.itemName.stringValue=@"Empty pocket";self.quantity.stringValue=@"";self.detail.stringValue=@"Add one to place an item in this pocket."; }
    self.removeButton.enabled=count>0;[self.renderer upload:pixels];[self.metal setNeedsDisplay:YES];
    self.animationStart=hg_scheduler_ticks();self.lastTick=-1;[self drawScene];
}
- (void)changePresentation:(id)sender {
    (void)sender;BOOL classic=self.presentation.selectedSegment==0;
    for(NSView *view in self.listViews)view.hidden=classic;self.classicView.hidden=!classic;
    if(self.inventory)[self refresh];
}
- (void)drawScene {
    if(!self.scene || self.classicView.hidden)return;
    NSInteger tick=(NSInteger)MIN(UINT64_C(12),hg_scheduler_ticks()-self.animationStart);
    if(tick==self.lastTick)return;self.lastTick=tick;
    if(!hg_scene_render(self.scene,self.inventory,(uint64_t)tick,self.scenePixels.mutableBytes)) { fprintf(stderr,"Scene resource decode failed\n");exit(1); }
    [self.classicRenderer upload:self.scenePixels.bytes];[self.classicView setNeedsDisplay:YES];
}
- (void)performAction:(enum HGAction)action {
    if(!self.inventory)return;
    int result=hg_inventory_action(self.inventory,action);
    if(action==HG_ADD || action==HG_REMOVE)self.status.stringValue=result?(action==HG_ADD?@"Added one item.":@"Removed one item."):@"Quantity unchanged.";
    else if(action==HG_CONFIRM) { HGItem item;if(hg_inventory_item(self.inventory,hg_inventory_selected(self.inventory),&item))self.status.stringValue=[NSString stringWithFormat:@"Selected %@",@(item.name)]; }
    else self.status.stringValue=@"";[self refresh];
}
- (void)changePocket:(id)sender { (void)sender;unsigned target=(unsigned)self.pockets.selectedSegment;while((unsigned)hg_inventory_pocket(self.inventory)!=target)hg_inventory_action(self.inventory,HG_NEXT_POCKET);self.status.stringValue=@"";[self refresh]; }
- (void)addOne:(id)sender { (void)sender;[self performAction:HG_ADD]; }
- (void)removeOne:(id)sender { (void)sender;[self performAction:HG_REMOVE]; }
- (void)smokeTest {
#define VERIFY(x) do { if(!(x)) { fprintf(stderr,"UI smoke failed: %s\n",#x);exit(1); } } while(0)
    VERIFY(hg_scheduler_ticks()>0);
    HGItem before,after;VERIFY(hg_inventory_item(self.inventory,0,&before));
    [self addOne:nil];VERIFY(hg_inventory_item(self.inventory,0,&after));VERIFY(after.quantity==before.quantity+1);
    [self removeOne:nil];[self performAction:HG_NEXT_POCKET];VERIFY(self.pockets.selectedSegment==1);
    NSEvent *down=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"" charactersIgnoringModifiers:@"" isARepeat:NO keyCode:125];
    [NSApp sendEvent:down];VERIFY(hg_inventory_selected(self.inventory)==1);
    [self.table selectRowIndexes:[NSIndexSet indexSetWithIndex:3] byExtendingSelection:NO];VERIFY(hg_inventory_selected(self.inventory)==3);
    VERIFY(hg_inventory_item(self.inventory,3,&after));VERIFY([self.itemName.stringValue isEqualToString:@(after.name)]);
    CGFloat scale=[self.classicView pixelScale];
    NSPoint local=NSMakePoint((self.classicView.bounds.size.width-HG_SCENE_WIDTH*scale)/2+410*scale,(self.classicView.bounds.size.height+HG_SCENE_HEIGHT*scale)/2-80*scale);
    NSPoint windowPoint=[self.classicView convertPoint:local toView:nil];
    NSEvent *click=[NSEvent mouseEventWithType:NSEventTypeLeftMouseDown location:windowPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil eventNumber:1 clickCount:1 pressure:1];
    [self.classicView mouseDown:click];VERIFY(hg_inventory_pocket(self.inventory)==2);
    self.presentation.selectedSegment=1;[self changePresentation:nil];VERIFY(self.classicView.hidden && !self.table.enclosingScrollView.hidden);
    self.presentation.selectedSegment=0;[self changePresentation:nil];[self.classicView draw];VERIFY(self.classicRenderer.completedDraws>0);
    puts("Inventory UI smoke passed: classic/list views, pointer mapping, keyboard, quantities, original text, original scheduled ticks, animated scene and completed Metal draw");
    [NSApp terminate:nil];
#undef VERIFY
}
@end
int main(int argc,const char **argv) { @autoreleasepool {
    if(argc>1 && !strcmp(argv[1],"--decode-only")) { uint8_t pixels[4096];if(!hg_item_icon(2,pixels))return 1;unsigned visible=0;for(unsigned i=3;i<4096;i+=4)visible+=pixels[i]!=0;printf("Original icon decoded: 32x32, %u opaque pixels\n",visible);return visible?0:1; }
    NSApplication *app=NSApplication.sharedApplication;[app setActivationPolicy:NSApplicationActivationPolicyRegular];
    AppDelegate *delegate=[AppDelegate new];delegate.smoke=argc>1 && !strcmp(argv[1],"--smoke");app.delegate=delegate;[app run];return 0;
} }
