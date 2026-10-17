#import <Cocoa/Cocoa.h>
#include "field_app.h"
#include <math.h>
#define WIDTH 800
#define HEIGHT 600
@interface FieldCanvas : NSView
@property const HGFieldView *field;
@property HGFieldApp *application;
@property HGFieldView *characterScene;
@property DSCamera camera;
@property(strong) NSBitmapImageRep *bitmap;
@property(strong) NSImage *image;
@property NSPoint dragPoint;
@property NSUInteger renders;
@property uint16_t movementKeys;
@property(strong) NSTimer *simulationTimer;
- (BOOL)render;
- (void)resetCamera;
- (BOOL)loadMap:(unsigned)map error:(char *)error;
- (BOOL)loadCharacter:(unsigned)model error:(char *)error;
@end
@implementation FieldCanvas
- (instancetype)initWithFrame:(NSRect)rect {
    if(!(self=[super initWithFrame:rect]))return nil;
    self.bitmap=[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL pixelsWide:WIDTH pixelsHigh:HEIGHT bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:WIDTH*4 bitsPerPixel:32];
    if(!self.bitmap)return nil;char error[256];if(![self loadMap:60 error:error])return nil;
    __weak FieldCanvas *weakCanvas=self;
    self.simulationTimer=[NSTimer timerWithTimeInterval:1.0/60.0 repeats:YES block:^(NSTimer *timer) {
        (void)timer;FieldCanvas *canvas=weakCanvas;if(canvas.application) {
            if(!canvas.window.isKeyWindow || canvas.window.firstResponder!=canvas)canvas.movementKeys=0;
            hg_field_app_input(canvas.application,canvas.movementKeys);
            double before[3]={0},after[3]={0};hg_field_app_presentation(canvas.application,before,NULL);
            unsigned long long revision=hg_field_app_revision(canvas.application);
            hg_field_app_step(canvas.application);hg_field_app_presentation(canvas.application,after,NULL);
            if(revision!=hg_field_app_revision(canvas.application)) {
                DSCamera camera=canvas.camera;camera.x+=after[0]-before[0];camera.y+=after[1]-before[1];camera.z+=after[2]-before[2];canvas.camera=camera;
                if(![canvas render])exit(1);
            }
        }
    }];
    [[NSRunLoop mainRunLoop] addTimer:self.simulationTimer forMode:NSRunLoopCommonModes];return self;
}
- (void)dealloc { [self.simulationTimer invalidate];hg_field_app_free(self.application);hg_field_view_free(self.characterScene); }
- (BOOL)loadMap:(unsigned)map error:(char *)error {
    self.movementKeys=0;hg_field_view_free(self.characterScene);self.characterScene=NULL;
    HGFieldApp *next=hg_field_app_new(map,error,256);hg_field_app_free(self.application);self.application=next;self.field=hg_field_app_view(next);
    if(next)[self resetCamera];
    if(![self render]) { snprintf(error,256,"Rasterization failed");hg_field_app_free(self.application);self.application=NULL;self.field=NULL;[self render];return NO; }
    return next!=NULL;
}
- (BOOL)loadCharacter:(unsigned)model error:(char *)error {
    self.movementKeys=0;HGFieldView *next=hg_character_view_load(model,error,256);
    hg_field_app_free(self.application);self.application=NULL;hg_field_view_free(self.characterScene);self.characterScene=next;self.field=next;
    if(next) { [self resetCamera];DSCamera c=self.camera;c.yaw=0;c.pitch=0;self.camera=c; }
    if(![self render]) { snprintf(error,256,"Character rasterization failed");hg_field_view_free(self.characterScene);self.characterScene=NULL;self.field=NULL;[self render];return NO; }
    return next!=NULL;
}
- (BOOL)acceptsFirstResponder { return YES; }
- (void)resetCamera { DSCamera c;if(self.application)hg_field_app_camera(self.application,&c,WIDTH,HEIGHT);else hg_field_view_camera(self.field,&c,WIDTH,HEIGHT);self.camera=c; }
- (BOOL)render {
    if(!self.field) {
        for(unsigned i=0;i<WIDTH*HEIGHT;i++) { unsigned char *p=self.bitmap.bitmapData+4*i;p[0]=28;p[1]=31;p[2]=36;p[3]=255; }
    }
    DSCamera c=self.camera;if(self.field && !(self.application?hg_field_app_render(self.application,&c,self.bitmap.bitmapData,WIDTH,HEIGHT):hg_field_view_render(self.field,&c,self.bitmap.bitmapData,WIDTH,HEIGHT)))return NO;
    self.image=[[NSImage alloc] initWithSize:NSMakeSize(WIDTH,HEIGHT)];[self.image addRepresentation:self.bitmap];self.renders++;[self setNeedsDisplay:YES];return YES;
}
- (void)drawRect:(NSRect)dirty {
    (void)dirty;NSGraphicsContext.currentContext.imageInterpolation=NSImageInterpolationNone;
    [self.image drawInRect:self.bounds fromRect:NSZeroRect operation:NSCompositingOperationCopy fraction:1];
}
- (void)keyDown:(NSEvent *)event {
    if(!self.field)return;
    DSCamera c=self.camera;NSString *key=event.charactersIgnoringModifiers.lowercaseString;BOOL changed=YES;
    if(hg_field_app_player(self.application,NULL,NULL)) {
        uint16_t bit=[key isEqualToString:@"w"]?0x40:[key isEqualToString:@"s"]?0x80:
                     [key isEqualToString:@"a"]?0x20:[key isEqualToString:@"d"]?0x10:0;
        if(bit){self.movementKeys|=bit;hg_field_app_input(self.application,self.movementKeys);return;}
    }
    switch(event.keyCode) {
    case 123:c.yaw-=0.12;break;case 124:c.yaw+=0.12;break;
    case 125:c.pitch=fmax(0.1,c.pitch-0.08);break;case 126:c.pitch=fmin(1.5,c.pitch+0.08);break;
    default:
        if([key isEqualToString:@"w"])c.pan_y+=24;
        else if([key isEqualToString:@"s"])c.pan_y-=24;
        else if([key isEqualToString:@"a"])c.pan_x+=24;
        else if([key isEqualToString:@"d"])c.pan_x-=24;
        else if([key isEqualToString:@"+"] || [key isEqualToString:@"="])c.scale=fmin(200,c.scale*1.15);
        else if([key isEqualToString:@"-"])c.scale=fmax(0.02,c.scale/1.15);
        else if([key isEqualToString:@"r"]) { [self resetCamera];c=self.camera; }
        else changed=NO;
    }
    if(changed) { c.pan_x=fmax(-2000,fmin(2000,c.pan_x));c.pan_y=fmax(-2000,fmin(2000,c.pan_y));self.camera=c;if(![self render])exit(1); }else [super keyDown:event];
}
- (void)keyUp:(NSEvent *)event {
    NSString *key=event.charactersIgnoringModifiers.lowercaseString;
    uint16_t bit=[key isEqualToString:@"w"]?0x40:[key isEqualToString:@"s"]?0x80:
                 [key isEqualToString:@"a"]?0x20:[key isEqualToString:@"d"]?0x10:0;
    if(bit){self.movementKeys&=~bit;hg_field_app_input(self.application,self.movementKeys);return;}
    [super keyUp:event];
}
- (void)scrollWheel:(NSEvent *)event {
    if(!self.field)return;
    DSCamera c=self.camera;c.scale=fmax(0.02,fmin(200,c.scale*exp(-event.scrollingDeltaY*0.025)));self.camera=c;if(![self render])exit(1);
}
- (void)mouseDown:(NSEvent *)event { self.dragPoint=event.locationInWindow;[self.window makeFirstResponder:self]; }
- (void)mouseDragged:(NSEvent *)event {
    if(!self.field)return;
    NSPoint p=event.locationInWindow;DSCamera c=self.camera;double x=p.x-self.dragPoint.x,y=p.y-self.dragPoint.y;
    if(event.modifierFlags&NSEventModifierFlagShift) { c.pan_x=fmax(-2000,fmin(2000,c.pan_x+x));c.pan_y=fmax(-2000,fmin(2000,c.pan_y-y)); }
    else { c.yaw+=x*0.008;c.pitch=fmax(0.1,fmin(1.5,c.pitch+y*0.008)); }
    self.camera=c;self.dragPoint=p;if(![self render])exit(1);
}
@end
static NSString *mapTitle(unsigned map) {
    const char *name=hg_field_map_name(map);if(!name)return @"Unknown map";
    NSString *title=[NSString stringWithUTF8String:name];
    if([title hasPrefix:@"MAP_"])title=[title substringFromIndex:4];
    return [[title stringByReplacingOccurrencesOfString:@"_" withString:@" "] capitalizedString];
}
@interface FieldApp : NSObject <NSApplicationDelegate,NSTableViewDataSource,NSTableViewDelegate,NSSearchFieldDelegate>
@property(strong) NSWindow *window;
@property(strong) FieldCanvas *canvas;
@property(strong) NSSearchField *search;
@property(strong) NSPopUpButton *category;
@property BOOL characters;
@property(strong) NSTableView *table;
@property(strong) NSTextField *status;
@property(strong) NSButton *previous;
@property(strong) NSButton *next;
@property(strong) NSMutableArray<NSNumber *> *rows;
@property(strong) NSMutableDictionary<NSNumber *,NSString *> *results;
@property unsigned selectedMap;
@property BOOL smoke;
@end
@implementation FieldApp
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app { (void)app;return YES; }
- (NSString *)titleFor:(unsigned)model {
    if(!self.characters)return mapTitle(model);
    if(model>=146 && model<=153) {
        NSArray *names=@[@"Falkner",@"Bugsy",@"Whitney",@"Morty",@"Chuck",@"Jasmine",@"Pryce",@"Clair"];
        return [NSString stringWithFormat:@"%@ · Johto Gym Leader",names[model-146]];
    }
    if(model>=162 && model<=169) {
        NSArray *names=@[@"Brock",@"Misty",@"Lt. Surge",@"Erika",@"Janine",@"Sabrina",@"Blaine",@"Blue"];
        return [NSString stringWithFormat:@"%@ · Kanto Gym Leader",names[model-162]];
    }
    return [[[NSString stringWithUTF8String:hg_character_name(model)] stringByReplacingOccurrencesOfString:@"_" withString:@" "] capitalizedString];
}
- (void)changeCategory:(id)sender {
    (void)sender;self.characters=self.category.indexOfSelectedItem==1;self.selectedMap=self.characters?146:60;
    self.results=[NSMutableDictionary new];self.search.stringValue=@"";[self filterMaps];
}
- (NSInteger)numberOfRowsInTableView:(NSTableView *)table { (void)table;return self.rows.count; }
- (NSView *)tableView:(NSTableView *)table viewForTableColumn:(NSTableColumn *)column row:(NSInteger)row {
    (void)table;(void)column;unsigned map=self.rows[row].unsignedIntValue;
    NSTextField *label=[NSTextField labelWithString:@""];label.font=[NSFont systemFontOfSize:12];
    NSString *result=self.results[@(map)];NSString *mark=result?([result length]?@"! ":@"✓ "):@"";
    label.stringValue=[NSString stringWithFormat:@"%@%u · %@",mark,map,[self titleFor:map]];
    label.toolTip=result.length?result:[NSString stringWithUTF8String:(self.characters?hg_character_name(map):hg_field_map_name(map))];return label;
}
- (void)updateButtons {
    NSInteger row=self.table.selectedRow;self.previous.enabled=row>0;self.next.enabled=row>=0 && row+1<(NSInteger)self.rows.count;
}
- (void)tableViewSelectionDidChange:(NSNotification *)note {
    (void)note;NSInteger row=self.table.selectedRow;[self updateButtons];if(row<0)return;
    unsigned map=self.rows[row].unsignedIntValue;self.selectedMap=map;char error[256];
    BOOL ok=self.characters?[self.canvas loadCharacter:map error:error]:[self.canvas loadMap:map error:error];NSString *reason=ok?@"":[NSString stringWithUTF8String:error];self.results[@(map)]=reason;
    self.status.stringValue=ok?[NSString stringWithFormat:@"%u · %@ — %zu triangles · %lu entries in list",map,[self titleFor:map],hg_field_view_triangles(self.canvas.field),(unsigned long)self.rows.count]:[NSString stringWithFormat:@"%u · %@ — %@",map,[self titleFor:map],reason];
    if(ok && !self.characters && map==60)self.status.stringValue=[self.status.stringValue stringByAppendingString:@" · Click scene, WASD to walk"];
    self.status.toolTip=self.status.stringValue;
    self.window.title=[NSString stringWithFormat:@"HeartGold MX — %@%@",[self titleFor:map],ok?@"":@" (not supported yet)"];
    [self.table reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:row] columnIndexes:[NSIndexSet indexSetWithIndex:0]];
}
- (void)filterMaps {
    NSString *query=[self.search.stringValue stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    self.rows=[NSMutableArray new];
    for(unsigned map=0;map<(self.characters?hg_character_count():hg_field_map_count());map++) {
        NSString *symbol=[NSString stringWithUTF8String:(self.characters?hg_character_name(map):hg_field_map_name(map))];
        if(!query.length || [[self titleFor:map] localizedCaseInsensitiveContainsString:query] || [symbol localizedCaseInsensitiveContainsString:query] || [[NSString stringWithFormat:@"%u",map] isEqualToString:query]) [self.rows addObject:@(map)];
    }
    [self.table reloadData];NSUInteger row=[self.rows indexOfObject:@(self.selectedMap)];
    if(self.rows.count) {
        if(row==NSNotFound)row=0;[self.table selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];[self.table scrollRowToVisible:row];
    } else {
        char error[256];[self.canvas loadMap:hg_field_map_count() error:error];self.status.stringValue=@"No entries match your search.";
    }
    [self updateButtons];
}
- (void)controlTextDidChange:(NSNotification *)note { if(note.object==self.search)[self filterMaps]; }
- (void)previousMap:(id)sender { (void)sender;NSInteger row=self.table.selectedRow;if(row>0) { [self.table selectRowIndexes:[NSIndexSet indexSetWithIndex:row-1] byExtendingSelection:NO];[self.table scrollRowToVisible:row-1]; } }
- (void)nextMap:(id)sender { (void)sender;NSInteger row=self.table.selectedRow;if(row>=0 && row+1<(NSInteger)self.rows.count) { [self.table selectRowIndexes:[NSIndexSet indexSetWithIndex:row+1] byExtendingSelection:NO];[self.table scrollRowToVisible:row+1]; } }
- (void)applicationDidFinishLaunching:(NSNotification *)note {
    (void)note;self.selectedMap=60;self.results=[NSMutableDictionary new];
    self.canvas=[[FieldCanvas alloc] initWithFrame:NSMakeRect(270,42,WIDTH,HEIGHT)];if(!self.canvas) { fprintf(stderr,"Cannot load New Bark field resources\n");exit(1); }
    NSView *root=[[NSView alloc] initWithFrame:NSMakeRect(0,0,1080,650)];self.canvas.autoresizingMask=NSViewWidthSizable|NSViewHeightSizable;self.canvas.toolTip=@"WASD walk in New Bark · Drag/arrows orbit · Scroll ± zoom · Shift-drag pan · R reset";[root addSubview:self.canvas];
    self.category=[[NSPopUpButton alloc] initWithFrame:NSMakeRect(10,612,248,28) pullsDown:NO];[self.category addItemsWithTitles:@[@"Maps",@"Characters · sprite meshes"]];self.category.target=self;self.category.action=@selector(changeCategory:);self.category.autoresizingMask=NSViewMinYMargin;[root addSubview:self.category];
    self.search=[[NSSearchField alloc] initWithFrame:NSMakeRect(10,578,248,28)];self.search.placeholderString=@"Search name or enter an ID";self.search.delegate=self;self.search.autoresizingMask=NSViewMinYMargin;[root addSubview:self.search];
    NSScrollView *scroll=[[NSScrollView alloc] initWithFrame:NSMakeRect(10,48,248,520)];scroll.hasVerticalScroller=YES;scroll.autoresizingMask=NSViewHeightSizable;
    self.table=[[NSTableView alloc] initWithFrame:scroll.bounds];NSTableColumn *column=[[NSTableColumn alloc] initWithIdentifier:@"map"];column.width=240;[self.table addTableColumn:column];self.table.headerView=nil;self.table.rowHeight=28;self.table.dataSource=self;self.table.delegate=self;self.table.allowsEmptySelection=YES;scroll.documentView=self.table;[root addSubview:scroll];
    self.previous=[NSButton buttonWithTitle:@"Previous" target:self action:@selector(previousMap:)];self.previous.frame=NSMakeRect(10,10,120,28);[root addSubview:self.previous];
    self.next=[NSButton buttonWithTitle:@"Next" target:self action:@selector(nextMap:)];self.next.frame=NSMakeRect(138,10,120,28);[root addSubview:self.next];
    self.status=[NSTextField labelWithString:@""];self.status.frame=NSMakeRect(280,10,790,24);self.status.autoresizingMask=NSViewWidthSizable;self.status.lineBreakMode=NSLineBreakByTruncatingMiddle;[root addSubview:self.status];
    self.window=[[NSWindow alloc] initWithContentRect:root.frame styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable|NSWindowStyleMaskMiniaturizable backing:NSBackingStoreBuffered defer:NO];
    self.window.contentView=root;self.window.contentMinSize=NSMakeSize(900,580);[self filterMaps];
    [self.window center];[self.window makeKeyAndOrderFront:nil];[self.window makeFirstResponder:self.canvas];[NSApp activateIgnoringOtherApps:YES];
    if(self.smoke)dispatch_after(dispatch_time(DISPATCH_TIME_NOW,300*NSEC_PER_MSEC),dispatch_get_main_queue(),^{
        if(hg_field_app_ticks(self.canvas.application)<3)exit(1);
        NSData *before=[NSData dataWithBytes:self.canvas.bitmap.bitmapData length:WIDTH*HEIGHT*4];
        NSEvent *key=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"" charactersIgnoringModifiers:@"" isARepeat:NO keyCode:124];
        [self.canvas keyDown:key];NSData *after=[NSData dataWithBytes:self.canvas.bitmap.bitmapData length:WIDTH*HEIGHT*4];
        if([before isEqual:after])exit(1);
        [self nextMap:nil];if(self.selectedMap!=61)exit(1);
        [self previousMap:nil];if(self.selectedMap!=60 || !self.canvas.field)exit(1);
        self.search.stringValue=@"Route 5";[self filterMaps];if(self.selectedMap!=13 || !self.canvas.field)exit(1);
        self.search.stringValue=@"player house";[self filterMaps];if(!self.rows.count || !self.canvas.field)exit(1);
        self.search.stringValue=@"Route 2";[self filterMaps];if(self.selectedMap!=10 || self.canvas.field)exit(1);
        self.search.stringValue=@"no such map xyz";[self filterMaps];if(self.rows.count || self.next.enabled || self.previous.enabled)exit(1);
        self.search.stringValue=@"";self.selectedMap=60;[self filterMaps];if(!self.canvas.field || self.rows.count!=hg_field_map_count())exit(1);
        [self.category selectItemAtIndex:1];[self changeCategory:nil];if(!self.canvas.characterScene || self.selectedMap!=146)exit(1);
        NSData *characterBefore=[NSData dataWithBytes:self.canvas.bitmap.bitmapData length:WIDTH*HEIGHT*4];[self.canvas keyDown:key];
        NSData *characterAfter=[NSData dataWithBytes:self.canvas.bitmap.bitmapData length:WIDTH*HEIGHT*4];if([characterBefore isEqual:characterAfter])exit(1);
        self.search.stringValue=@"Whitney";[self filterMaps];if(self.selectedMap!=148 || !self.canvas.characterScene)exit(1);
        [self.category selectItemAtIndex:0];[self changeCategory:nil];if(!self.canvas.application || self.selectedMap!=60)exit(1);
        int px,pz,nx,nz;if(!hg_field_app_player(self.canvas.application,&px,&pz))exit(1);
        NSEvent *walk=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"w" charactersIgnoringModifiers:@"w" isARepeat:NO keyCode:13];
        [self.canvas keyDown:walk];if(self.canvas.movementKeys!=0x40)exit(1);
        for(unsigned i=0;i<8;i++)hg_field_app_step(self.canvas.application);
        if(!hg_field_app_player(self.canvas.application,&nx,&nz)||(nx==px&&nz==pz))exit(1);
        NSEvent *release=[NSEvent keyEventWithType:NSEventTypeKeyUp location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"w" charactersIgnoringModifiers:@"w" isARepeat:NO keyCode:13];
        [self.canvas keyUp:release];if(self.canvas.movementKeys)exit(1);
        for(unsigned i=0;i<16;i++)hg_field_app_step(self.canvas.application);
        if(!hg_field_app_player(self.canvas.application,&px,&pz)||px!=nx||pz!=nz)exit(1);
        if(![self.canvas render])exit(1);
        [self.canvas display];fprintf(stderr,"Map browser smoke: player key-down/step/key-up, camera, search, supported/unsupported selection, recovery and AppKit display passed\n");[NSApp terminate:nil];
    });
}
@end

int main(int argc,const char **argv) {
    @autoreleasepool {
        if((argc==3 && !strcmp(argv[1],"--render-only")) || (argc==4 && (!strcmp(argv[1],"--render-map") || !strcmp(argv[1],"--render-character")))) {
            FieldCanvas *canvas=[[FieldCanvas alloc] initWithFrame:NSMakeRect(0,0,WIDTH,HEIGHT)];if(!canvas)return 1;
            if(argc==4) {
                char *end;unsigned long map=strtoul(argv[2],&end,10);char error[256];
                if(!argv[2][0] || *end || map>=hg_field_map_count())return 2;
                if(!(!strcmp(argv[1],"--render-character")?[canvas loadCharacter:(unsigned)map error:error]:[canvas loadMap:(unsigned)map error:error])) { fprintf(stderr,"%s\n",error);return 1; }
            }
            NSData *png=[canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
            return [png writeToFile:[NSString stringWithUTF8String:argv[argc-1]] atomically:YES]?0:1;
        }
        if(argc!=1 && (argc!=2 || strcmp(argv[1],"--smoke"))) { fprintf(stderr,"Usage: heartgold_field_viewer [--smoke | --render-only path.png | --render-map ID path.png | --render-character ID path.png]\n");return 2; }
        NSApplication *app=[NSApplication sharedApplication];[app setActivationPolicy:NSApplicationActivationPolicyRegular];
        NSMenu *main=[NSMenu new],*menu=[NSMenu new];NSMenuItem *root=[NSMenuItem new];[main addItem:root];root.submenu=menu;[menu addItemWithTitle:@"Quit HeartGold MX Field" action:@selector(terminate:) keyEquivalent:@"q"];app.mainMenu=main;
        FieldApp *delegate=[FieldApp new];delegate.smoke=argc==2;app.delegate=delegate;[app run];
    }return 0;
}
