#import <Cocoa/Cocoa.h>
#include "field_app.h"
#include <math.h>
#include <time.h>
#define WIDTH 800
#define HEIGHT 600
static void field_fail(unsigned line) { fprintf(stderr,"Field viewer check failed at src/field_viewer.m:%u\n",line);exit(EXIT_FAILURE); }
void hg_ui_test_approach(HGFieldApp *,unsigned);
void hg_ui_test_return_to_elm(HGFieldApp *);
void hg_test_prepare_defeat(HGFieldApp *);
void hg_ui_test_approach_center_door(HGFieldApp *);
void hg_ui_test_prepare_nurse(HGFieldApp *);
void hg_ui_test_prepare_mart(HGFieldApp *);
void hg_ui_test_prepare_mother(HGFieldApp *);
int hg_ui_test_nurse_healed(HGFieldApp *);
@interface FieldCanvas : NSView
@property const HGFieldView *field;
@property HGFieldApp *application;
@property HGFieldApp *suspendedStory;
@property HGFieldApp *previousCheckpoint;
@property BOOL exploring;
@property BOOL audioEnabled;
@property HGFieldView *characterScene;
@property DSCamera camera;
@property(strong) NSBitmapImageRep *bitmap;
@property(strong) NSImage *image;
@property NSPoint dragPoint;
@property NSUInteger renders;
@property uint16_t movementKeys;
@property(strong) NSTimer *simulationTimer;
@property(copy) void (^mapChanged)(unsigned);
- (BOOL)render;
- (void)resetCamera;
- (BOOL)loadMap:(unsigned)map error:(char *)error;
- (BOOL)loadCheckpoint:(NSString *)path error:(char *)error;
- (BOOL)loadSession:(NSString *)path cartridge:(BOOL)cartridge error:(char *)error;
- (BOOL)undoCheckpoint:(char *)error;
- (BOOL)loadCharacter:(unsigned)model error:(char *)error;
- (BOOL)setExplorationEnabled:(BOOL)enabled error:(char *)error;
@end
@implementation FieldCanvas
- (instancetype)initWithFrame:(NSRect)rect {
    if(!(self=[super initWithFrame:rect]))return nil;
    self.bitmap=[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL pixelsWide:WIDTH pixelsHigh:HEIGHT bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:WIDTH*4 bitsPerPixel:32];
    if(!self.bitmap)return nil;char error[256];if(![self loadMap:64 error:error])return nil;
    __weak FieldCanvas *weakCanvas=self;
    self.simulationTimer=[NSTimer timerWithTimeInterval:1.0/60.0 repeats:YES block:^(NSTimer *timer) {
        (void)timer;FieldCanvas *canvas=weakCanvas;if(canvas.application) {
            if(!canvas.window.isKeyWindow || canvas.window.firstResponder!=canvas){canvas.movementKeys=0;hg_field_app_clear_input(canvas.application); }
            hg_field_app_input(canvas.application,canvas.movementKeys);
            double before[3]={0},after[3]={0};hg_field_app_presentation(canvas.application,before,NULL);
            unsigned long long revision=hg_field_app_revision(canvas.application);
            struct timespec now;clock_gettime(CLOCK_MONOTONIC,&now);
            hg_field_app_advance(canvas.application,(uint64_t)now.tv_sec*UINT64_C(1000000000)+(uint64_t)now.tv_nsec);
            hg_field_app_presentation(canvas.application,after,NULL);
            if(revision!=hg_field_app_revision(canvas.application)) {
                DSCamera camera=canvas.camera;camera.x+=after[0]-before[0];camera.y+=after[1]-before[1];camera.z+=after[2]-before[2];canvas.camera=camera;
                if(![canvas render])field_fail(__LINE__);
            }
        }
    }];
    [[NSRunLoop mainRunLoop] addTimer:self.simulationTimer forMode:NSRunLoopCommonModes];return self;
}
- (void)dealloc { [self.simulationTimer invalidate];hg_field_app_free(self.application);hg_field_app_free(self.suspendedStory);hg_field_app_free(self.previousCheckpoint);hg_field_view_free(self.characterScene); }
- (BOOL)loadMap:(unsigned)map error:(char *)error {
    self.movementKeys=0;hg_field_view_free(self.characterScene);self.characterScene=NULL;
    HGFieldApp *next=self.exploring?hg_field_app_new_explore(map,error,256):map==64?hg_field_app_new_game(error,256):hg_field_app_new(map,error,256);hg_field_app_free(self.application);self.application=next;self.field=hg_field_app_view(next);
    if(next&&self.audioEnabled&&!hg_field_app_audio_enable(next,1,error,256))return NO;
    if(next)[self resetCamera];
    if(![self render]) { snprintf(error,256,"Rasterization failed");hg_field_app_free(self.application);self.application=NULL;self.field=NULL;[self render];return NO; }
    return next!=NULL;
}
- (BOOL)loadCheckpoint:(NSString *)path error:(char *)error { return [self loadSession:path cartridge:NO error:error]; }
- (BOOL)loadSession:(NSString *)path cartridge:(BOOL)cartridge error:(char *)error {
    HGFieldApp *next=cartridge?hg_field_app_continue(path.fileSystemRepresentation,error,256):hg_field_app_checkpoint_load(path.fileSystemRepresentation,error,256);if(!next)return NO;
    DSCamera camera;hg_field_app_camera(next,&camera,WIDTH,HEIGHT);
    if(!hg_field_app_render(next,&camera,self.bitmap.bitmapData,WIDTH,HEIGHT)){snprintf(error,256,"Cannot display this checkpoint");hg_field_app_free(next);return NO;}
    if(self.audioEnabled&&!hg_field_app_audio_enable(next,1,error,256)){hg_field_app_free(next);return NO;}
    HGFieldApp *old=self.exploring?self.suspendedStory:self.application;
    if(self.exploring){hg_field_app_free(self.application);self.suspendedStory=NULL;}
    hg_field_app_free(self.previousCheckpoint);self.previousCheckpoint=old;hg_field_app_audio_enable(old,0,NULL,0);
    self.application=next;self.exploring=NO;self.movementKeys=0;hg_field_app_clear_input(next);hg_field_view_free(self.characterScene);self.characterScene=NULL;self.field=hg_field_app_view(next);self.camera=camera;return [self render];
}
- (BOOL)undoCheckpoint:(char *)error {
    if(!self.previousCheckpoint){snprintf(error,256,"No previous session");return NO;}
    if(self.audioEnabled&&!hg_field_app_audio_enable(self.previousCheckpoint,1,error,256))return NO;
    if(self.exploring){hg_field_app_free(self.application);self.application=self.suspendedStory;self.suspendedStory=NULL;self.exploring=NO;}
    hg_field_app_audio_enable(self.application,0,NULL,0);HGFieldApp *old=self.application;self.application=self.previousCheckpoint;self.previousCheckpoint=old;
    self.movementKeys=0;hg_field_app_resume(self.application);hg_field_view_free(self.characterScene);self.characterScene=NULL;self.field=hg_field_app_view(self.application);[self resetCamera];return [self render];
}
- (BOOL)setExplorationEnabled:(BOOL)enabled error:(char *)error {
    if(enabled==self.exploring)return YES;
    BOOL success=YES;
    unsigned map=self.application?hg_field_app_map(self.application):64;
    self.movementKeys=0;hg_field_app_clear_input(self.application);
    if(enabled) {
        hg_field_app_audio_enable(self.application,0,NULL,0);self.suspendedStory=self.application;self.application=NULL;self.exploring=YES;
        if([self loadMap:map error:error])return YES;
        success=NO;self.application=self.suspendedStory;self.suspendedStory=NULL;self.exploring=NO;
    } else {
        hg_field_app_free(self.application);self.application=self.suspendedStory;self.suspendedStory=NULL;self.exploring=NO;
        if(!self.application)return [self loadMap:map error:error];
    }
    if(self.audioEnabled&&!hg_field_app_audio_enable(self.application,1,error,256))return NO;
    hg_field_app_resume(self.application);self.field=hg_field_app_view(self.application);[self resetCamera];
    if(![self render]){snprintf(error,256,"Rasterization failed after restoring story");return NO;}return success;
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
    if(self.application && self.field!=hg_field_app_view(self.application)) {
        self.field=hg_field_app_view(self.application);self.movementKeys=0;hg_field_app_clear_input(self.application);
        [self resetCamera];if(self.mapChanged)self.mapChanged(hg_field_app_map(self.application));
    }
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
    if(event.keyCode==53&&hg_field_app_battle_active(self.application)){self.movementKeys=0;hg_field_app_battle_cancel(self.application);return;}
    if(event.keyCode==53&&hg_field_app_shop(self.application)){hg_field_app_shop_cancel(self.application);return;}
    if(!self.field)return;
    if(hg_field_app_name_pending(self.application)){
        self.movementKeys=0;hg_field_app_clear_input(self.application);
        NSString *draft=[NSString stringWithUTF8String:hg_field_app_name_text(self.application)];
        if(event.keyCode==36||event.keyCode==53){if(!event.isARepeat)hg_field_app_name_submit(self.application,draft.UTF8String,event.keyCode==53);}
        else if(event.keyCode==51){if(draft.length){NSRange last=[draft rangeOfComposedCharacterSequenceAtIndex:draft.length-1];hg_field_app_name_edit(self.application,[draft substringToIndex:last.location].UTF8String);}}
        else if(event.characters.length&&![event.characters rangeOfCharacterFromSet:NSCharacterSet.controlCharacterSet].length){hg_field_app_name_edit(self.application,[draft stringByAppendingString:event.characters].UTF8String);}
        [self render];return;
    }
    DSCamera c=self.camera;NSString *key=event.charactersIgnoringModifiers.lowercaseString;BOOL changed=YES;
    if(hg_field_app_player(self.application,NULL,NULL)) {
        if([key isEqualToString:@"b"]||[key isEqualToString:@"p"]||event.keyCode==53){
            if(!event.isARepeat){self.movementKeys=0;hg_field_app_inspect(self.application,event.keyCode==53?0:[key isEqualToString:@"b"]?1:2);[self render];}return;
        }
        if(event.keyCode==36 || event.keyCode==49) {
            if(!event.isARepeat){self.movementKeys=0;hg_field_app_interact(self.application);}return;
        }
        uint16_t bit=[key isEqualToString:@"w"]?0x40:[key isEqualToString:@"s"]?0x80:
                     [key isEqualToString:@"a"]?0x20:[key isEqualToString:@"d"]?0x10:0;
        if(bit){self.movementKeys=(self.movementKeys|bit)&~2u;if(event.modifierFlags&NSEventModifierFlagShift)self.movementKeys|=2;
            hg_field_app_input(self.application,self.movementKeys);return;}
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
    if(changed) { c.pan_x=fmax(-2000,fmin(2000,c.pan_x));c.pan_y=fmax(-2000,fmin(2000,c.pan_y));self.camera=c;if(![self render])field_fail(__LINE__); }else [super keyDown:event];
}
- (void)keyUp:(NSEvent *)event {
    NSString *key=event.charactersIgnoringModifiers.lowercaseString;
    uint16_t bit=[key isEqualToString:@"w"]?0x40:[key isEqualToString:@"s"]?0x80:
                 [key isEqualToString:@"a"]?0x20:[key isEqualToString:@"d"]?0x10:0;
    if(bit){self.movementKeys&=~bit;hg_field_app_input(self.application,self.movementKeys);return;}
    [super keyUp:event];
}
- (void)flagsChanged:(NSEvent *)event {
    if(hg_field_app_player(self.application,NULL,NULL)) {
        self.movementKeys&=~2u;if(event.modifierFlags&NSEventModifierFlagShift)self.movementKeys|=2;
        hg_field_app_input(self.application,self.movementKeys);
    }
    [super flagsChanged:event];
}
- (void)scrollWheel:(NSEvent *)event {
    if(!self.field)return;
    DSCamera c=self.camera;c.scale=fmax(0.02,fmin(200,c.scale*exp(-event.scrollingDeltaY*0.025)));self.camera=c;if(![self render])field_fail(__LINE__);
}
- (void)mouseDown:(NSEvent *)event { self.dragPoint=event.locationInWindow;[self.window makeFirstResponder:self]; }
- (void)mouseDragged:(NSEvent *)event {
    if(!self.field)return;
    NSPoint p=event.locationInWindow;DSCamera c=self.camera;double x=p.x-self.dragPoint.x,y=p.y-self.dragPoint.y;
    if(event.modifierFlags&NSEventModifierFlagShift) { c.pan_x=fmax(-2000,fmin(2000,c.pan_x+x));c.pan_y=fmax(-2000,fmin(2000,c.pan_y-y)); }
    else { c.yaw+=x*0.008;c.pitch=fmax(0.1,fmin(1.5,c.pitch+y*0.008)); }
    self.camera=c;self.dragPoint=p;if(![self render])field_fail(__LINE__);
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
@property(strong) NSPopUpButton *checkpoints;
@property(strong) NSButton *checkpointBack;
@property BOOL characters;
@property(strong) NSTableView *table;
@property(strong) NSTextField *status;
@property(strong) NSButton *exploration;
@property(strong) NSButton *previous;
@property(strong) NSButton *next;
@property(strong) NSMutableArray<NSNumber *> *rows;
@property(strong) NSMutableDictionary<NSNumber *,NSString *> *results;
@property unsigned selectedMap;
@property BOOL smoke;
@property BOOL audioSmoke;
@property BOOL exploreAtLaunch;
@property unsigned initialMap;
@property BOOL synchronizingSelection;
@end
@implementation FieldApp
- (BOOL)saveToPath:(NSString *)path {
    self.canvas.movementKeys=0;hg_field_app_clear_input(self.canvas.application);char error[256];
    BOOL ok=hg_field_app_save(self.canvas.application,path.fileSystemRepresentation,error,sizeof(error));
    self.status.stringValue=ok?@"Game saved.":[NSString stringWithUTF8String:error];return ok;
}
- (void)saveGame:(id)sender {
    (void)sender;
    if(!hg_field_app_save_unlocked(self.canvas.application)){self.status.stringValue=@"Save unlocks after Mom's introduction.";return;}
    const char *existing=hg_field_app_save_path(self.canvas.application);
    if(existing[0]){[self saveToPath:[NSString stringWithUTF8String:existing]];return;}
    NSSavePanel *panel=[NSSavePanel savePanel];panel.nameFieldStringValue=@"HeartGold.sav";
    if([panel runModal]==NSModalResponseOK)[self saveToPath:panel.URL.path];
    [self.window makeFirstResponder:self.canvas];
}
- (void)continueGame:(id)sender {
    (void)sender;NSOpenPanel *panel=[NSOpenPanel openPanel];panel.canChooseDirectories=NO;panel.allowsMultipleSelection=NO;
    if([panel runModal]==NSModalResponseOK) {
        char error[256];
        if([self.canvas loadSession:panel.URL.path cartridge:YES error:error])[self synchronizeCheckpoint];
        else self.status.stringValue=[NSString stringWithUTF8String:error];
    }
    [self.window makeFirstResponder:self.canvas];
}
- (void)showBag:(id)sender { (void)sender;self.canvas.movementKeys=0;hg_field_app_inspect(self.canvas.application,1);[self.canvas render];[self.window makeFirstResponder:self.canvas]; }
- (void)showPokemon:(id)sender { (void)sender;self.canvas.movementKeys=0;hg_field_app_inspect(self.canvas.application,2);[self.canvas render];[self.window makeFirstResponder:self.canvas]; }
- (void)checkpointChanged:(id)sender {
    (void)sender;NSString *path=self.checkpoints.selectedItem.representedObject;if(!path)return;char error[256];
    if(![self.canvas loadCheckpoint:path error:error]){self.status.stringValue=[NSString stringWithUTF8String:error];[self.checkpoints selectItemAtIndex:0];return;}
    [self synchronizeCheckpoint];
}
- (void)synchronizeCheckpoint {
    self.characters=NO;[self.category selectItemAtIndex:0];self.exploration.enabled=YES;self.exploration.state=NSControlStateValueOff;
    self.selectedMap=hg_field_app_map(self.canvas.application);self.search.stringValue=@"";self.results=[NSMutableDictionary new];self.synchronizingSelection=YES;[self filterMaps];self.synchronizingSelection=NO;
    self.checkpointBack.enabled=self.canvas.previousCheckpoint!=NULL;[self updateSceneStatus:self.selectedMap success:YES reason:@""];[self.window makeFirstResponder:self.canvas];
}
- (void)checkpointBack:(id)sender { (void)sender;char error[256];if(![self.canvas undoCheckpoint:error]){self.status.stringValue=[NSString stringWithUTF8String:error];return;}[self.checkpoints selectItemAtIndex:0];[self synchronizeCheckpoint]; }
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
    (void)sender;self.characters=self.category.indexOfSelectedItem==1;self.exploration.enabled=!self.characters;self.selectedMap=self.characters?146:60;
    self.results=[NSMutableDictionary new];self.search.stringValue=@"";[self filterMaps];
}
- (void)changeExploration:(id)sender {
    (void)sender;char error[256];BOOL enabled=self.exploration.state==NSControlStateValueOn;
    if(![self.canvas setExplorationEnabled:enabled error:error]) { self.exploration.state=self.canvas.exploring?NSControlStateValueOn:NSControlStateValueOff;self.status.stringValue=[NSString stringWithUTF8String:error];return; }
    self.selectedMap=hg_field_app_map(self.canvas.application);self.search.stringValue=@"";self.results=[NSMutableDictionary new];self.synchronizingSelection=YES;[self filterMaps];self.synchronizingSelection=NO;
    [self updateSceneStatus:self.selectedMap success:YES reason:@""];[self.window makeFirstResponder:self.canvas];
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
    (void)note;if(self.synchronizingSelection)return;NSInteger row=self.table.selectedRow;[self updateButtons];if(row<0)return;
    unsigned map=self.rows[row].unsignedIntValue;self.selectedMap=map;char error[256];
    BOOL ok=self.characters?[self.canvas loadCharacter:map error:error]:[self.canvas loadMap:map error:error];NSString *reason=ok?@"":[NSString stringWithUTF8String:error];self.results[@(map)]=reason;
    [self updateSceneStatus:map success:ok reason:reason];
}
- (void)updateSceneStatus:(unsigned)map success:(BOOL)ok reason:(NSString *)reason {
    NSInteger row=self.table.selectedRow;
    self.status.stringValue=ok?[NSString stringWithFormat:@"%u · %@ — %zu triangles · %lu entries in list",map,[self titleFor:map],hg_field_view_triangles(self.canvas.field),(unsigned long)self.rows.count]:[NSString stringWithFormat:@"%u · %@ — %@",map,[self titleFor:map],reason];
    if(ok&&!self.characters&&hg_field_app_is_exploring(self.canvas.application))self.status.stringValue=[self.status.stringValue stringByAppendingString:hg_field_app_player(self.canvas.application,NULL,NULL)?@" · Exploration · WASD / Shift run · Story and NPCs off":@" · Exploration · View only; walking unavailable"];
    else if(ok && !self.characters && hg_field_app_player(self.canvas.application,NULL,NULL))self.status.stringValue=[self.status.stringValue stringByAppendingString:@" · WASD walk · Shift run · B Bag · P Pokémon"];
    else if(ok&&!self.characters)self.status.stringValue=[self.status.stringValue stringByAppendingString:@" · Inspection only; field gameplay unavailable"];
    self.status.toolTip=self.status.stringValue;
    self.window.title=[NSString stringWithFormat:@"HeartGold MX — %@%@",[self titleFor:map],ok?@"":@" (not supported yet)"];
    if(row>=0)[self.table reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:row] columnIndexes:[NSIndexSet indexSetWithIndex:0]];
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
    (void)note;self.selectedMap=self.exploreAtLaunch?self.initialMap:self.smoke?60:64;self.results=[NSMutableDictionary new];
    self.canvas=[[FieldCanvas alloc] initWithFrame:NSMakeRect(270,42,WIDTH,HEIGHT)];if(!self.canvas) { fprintf(stderr,"Cannot load New Bark field resources\n");field_fail(__LINE__); }
    __weak FieldApp *weakApp=self;
    self.canvas.mapChanged=^(unsigned map) {
        FieldApp *app=weakApp;if(!app)return;app.selectedMap=map;app.search.stringValue=@"";
        app.synchronizingSelection=YES;[app filterMaps];app.synchronizingSelection=NO;
        app.results[@(map)]=@"";[app updateSceneStatus:map success:YES reason:@""];
    };
    NSView *root=[[NSView alloc] initWithFrame:NSMakeRect(0,0,1080,650)];self.canvas.autoresizingMask=NSViewWidthSizable|NSViewHeightSizable;self.canvas.toolTip=@"WASD walk · Shift + WASD run · Enter/Space read town/lab signs · Drag/arrows orbit · Scroll ± zoom · Shift-drag pan · R reset";[root addSubview:self.canvas];
    self.category=[[NSPopUpButton alloc] initWithFrame:NSMakeRect(10,612,248,28) pullsDown:NO];[self.category addItemsWithTitles:@[@"Maps",@"Characters · sprite meshes"]];self.category.target=self;self.category.action=@selector(changeCategory:);self.category.autoresizingMask=NSViewMinYMargin;[root addSubview:self.category];
    self.exploration=[NSButton checkboxWithTitle:@"Explore freely" target:self action:@selector(changeExploration:)];self.exploration.frame=NSMakeRect(280,612,180,28);self.exploration.autoresizingMask=NSViewMinYMargin;self.exploration.toolTip=@"Browse scenery without story events or NPCs. Untick to resume your previous session.";[root addSubview:self.exploration];
    NSButton *bag=[NSButton buttonWithTitle:@"Bag (B)" target:self action:@selector(showBag:)];bag.frame=NSMakeRect(470,612,110,28);bag.autoresizingMask=NSViewMinYMargin;[root addSubview:bag];
    NSButton *pokemon=[NSButton buttonWithTitle:@"Pokémon (P)" target:self action:@selector(showPokemon:)];pokemon.frame=NSMakeRect(590,612,140,28);pokemon.autoresizingMask=NSViewMinYMargin;[root addSubview:pokemon];
    self.checkpoints=[[NSPopUpButton alloc] initWithFrame:NSMakeRect(740,612,250,28) pullsDown:NO];[self.checkpoints addItemWithTitle:@"Jump to checkpoint…"];
    NSString *checkpointDirectory=[NSBundle.mainBundle.executablePath.stringByDeletingLastPathComponent stringByAppendingPathComponent:@"checkpoints"];
    NSArray<NSString *> *files=[[[NSFileManager defaultManager] contentsOfDirectoryAtPath:checkpointDirectory error:nil] sortedArrayUsingSelector:@selector(compare:)];
    NSDictionary *starters=@{@"152":@"Chikorita",@"155":@"Cyndaquil",@"158":@"Totodile"};NSArray *stages=@[@"",@"Route 29",@"Before rival",@"After rival",@"Before tutorial",@"After tutorial"];
    for(NSString *file in files){if(![file.pathExtension isEqualToString:@"hgcheckpoint"])continue;NSArray *parts=[file.stringByDeletingPathExtension componentsSeparatedByString:@"-"];if(parts.count!=2)continue;NSString *name=starters[parts[0]];NSInteger stage=[parts[1] integerValue];if(!name||stage<1||stage>5)continue;
        [self.checkpoints addItemWithTitle:[NSString stringWithFormat:@"%@ · %@",name,stages[stage]]];self.checkpoints.lastItem.representedObject=[checkpointDirectory stringByAppendingPathComponent:file];}
    self.checkpoints.target=self;self.checkpoints.action=@selector(checkpointChanged:);self.checkpoints.autoresizingMask=NSViewMinYMargin;self.checkpoints.toolTip=@"Resume an opening-route state captured after its required events. Back returns to your previous session.";[root addSubview:self.checkpoints];
    self.checkpointBack=[NSButton buttonWithTitle:@"Back" target:self action:@selector(checkpointBack:)];self.checkpointBack.frame=NSMakeRect(995,612,75,28);self.checkpointBack.autoresizingMask=NSViewMinYMargin;self.checkpointBack.enabled=NO;[root addSubview:self.checkpointBack];
    self.search=[[NSSearchField alloc] initWithFrame:NSMakeRect(10,578,248,28)];self.search.placeholderString=@"Search name or enter an ID";self.search.delegate=self;self.search.autoresizingMask=NSViewMinYMargin;[root addSubview:self.search];
    NSScrollView *scroll=[[NSScrollView alloc] initWithFrame:NSMakeRect(10,48,248,520)];scroll.hasVerticalScroller=YES;scroll.autoresizingMask=NSViewHeightSizable;
    self.table=[[NSTableView alloc] initWithFrame:scroll.bounds];NSTableColumn *column=[[NSTableColumn alloc] initWithIdentifier:@"map"];column.width=240;[self.table addTableColumn:column];self.table.headerView=nil;self.table.rowHeight=28;self.table.dataSource=self;self.table.delegate=self;self.table.allowsEmptySelection=YES;scroll.documentView=self.table;[root addSubview:scroll];
    self.previous=[NSButton buttonWithTitle:@"Previous" target:self action:@selector(previousMap:)];self.previous.frame=NSMakeRect(10,10,120,28);[root addSubview:self.previous];
    self.next=[NSButton buttonWithTitle:@"Next" target:self action:@selector(nextMap:)];self.next.frame=NSMakeRect(138,10,120,28);[root addSubview:self.next];
    NSButton *save=[NSButton buttonWithTitle:@"Save" target:self action:@selector(saveGame:)];save.frame=NSMakeRect(280,10,75,28);[root addSubview:save];
    NSButton *load=[NSButton buttonWithTitle:@"Continue…" target:self action:@selector(continueGame:)];load.frame=NSMakeRect(360,10,100,28);[root addSubview:load];
    self.status=[NSTextField labelWithString:@""];self.status.frame=NSMakeRect(470,10,600,24);self.status.autoresizingMask=NSViewWidthSizable;self.status.lineBreakMode=NSLineBreakByTruncatingMiddle;[root addSubview:self.status];
    self.window=[[NSWindow alloc] initWithContentRect:root.frame styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable|NSWindowStyleMaskMiniaturizable backing:NSBackingStoreBuffered defer:NO];
    self.window.contentView=root;self.window.contentMinSize=NSMakeSize(1080,580);
    if(self.exploreAtLaunch) {
        char error[256];if(![self.canvas setExplorationEnabled:YES error:error]){fprintf(stderr,"%s\n",error);field_fail(__LINE__);}
        self.exploration.state=NSControlStateValueOn;
    }
    [self filterMaps];
    [self.window center];[self.window makeKeyAndOrderFront:nil];[self.window makeFirstResponder:self.canvas];[NSApp activateIgnoringOtherApps:YES];
    if(!self.smoke||self.audioSmoke){char error[256];self.canvas.audioEnabled=YES;if(!hg_field_app_audio_enable(self.canvas.application,1,error,sizeof(error)))self.status.stringValue=[NSString stringWithUTF8String:error];}
    if(self.smoke)dispatch_after(dispatch_time(DISPATCH_TIME_NOW,300*NSEC_PER_MSEC),dispatch_get_main_queue(),^{
        if(self.audioSmoke){uint64_t frames=hg_field_app_audio_frames(self.canvas.application);fprintf(stderr,"Field native audio consumed %llu PCM frames\n",(unsigned long long)frames);if(!frames)field_fail(__LINE__);}
        if(hg_field_app_ticks(self.canvas.application)<3)field_fail(__LINE__);
        NSData *before=[NSData dataWithBytes:self.canvas.bitmap.bitmapData length:WIDTH*HEIGHT*4];
        NSEvent *key=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"" charactersIgnoringModifiers:@"" isARepeat:NO keyCode:124];
        [self.canvas keyDown:key];NSData *after=[NSData dataWithBytes:self.canvas.bitmap.bitmapData length:WIDTH*HEIGHT*4];
        if([before isEqual:after])field_fail(__LINE__);
        [self nextMap:nil];if(self.selectedMap!=61)field_fail(__LINE__);
        [self previousMap:nil];if(self.selectedMap!=60 || !self.canvas.field)field_fail(__LINE__);
        self.search.stringValue=@"Route 5";[self filterMaps];if(self.selectedMap!=13 || !self.canvas.field)field_fail(__LINE__);
        self.search.stringValue=@"player house";[self filterMaps];if(!self.rows.count || !self.canvas.field)field_fail(__LINE__);
        self.search.stringValue=@"Route 2";[self filterMaps];if(self.selectedMap!=10 || self.canvas.field)field_fail(__LINE__);
        self.search.stringValue=@"no such map xyz";[self filterMaps];if(self.rows.count || self.next.enabled || self.previous.enabled)field_fail(__LINE__);
        self.search.stringValue=@"";self.selectedMap=60;[self filterMaps];if(!self.canvas.field || self.rows.count!=hg_field_map_count())field_fail(__LINE__);
        [self.category selectItemAtIndex:1];[self changeCategory:nil];if(!self.canvas.characterScene || self.selectedMap!=146)field_fail(__LINE__);
        NSData *characterBefore=[NSData dataWithBytes:self.canvas.bitmap.bitmapData length:WIDTH*HEIGHT*4];[self.canvas keyDown:key];
        NSData *characterAfter=[NSData dataWithBytes:self.canvas.bitmap.bitmapData length:WIDTH*HEIGHT*4];if([characterBefore isEqual:characterAfter])field_fail(__LINE__);
        self.search.stringValue=@"Whitney";[self filterMaps];if(self.selectedMap!=148 || !self.canvas.characterScene)field_fail(__LINE__);
        [self.category selectItemAtIndex:0];[self changeCategory:nil];if(!self.canvas.application || self.selectedMap!=60)field_fail(__LINE__);
        int px,pz,nx,nz;if(!hg_field_app_player(self.canvas.application,&px,&pz))field_fail(__LINE__);
        NSEvent *walk=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"w" charactersIgnoringModifiers:@"w" isARepeat:NO keyCode:13];
        [self.canvas keyDown:walk];if(self.canvas.movementKeys!=0x40)field_fail(__LINE__);
        for(unsigned i=0;i<8;i++)hg_field_app_step(self.canvas.application);
        if(!hg_field_app_player(self.canvas.application,&nx,&nz)||(nx==px&&nz==pz))field_fail(__LINE__);
        NSEvent *release=[NSEvent keyEventWithType:NSEventTypeKeyUp location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"w" charactersIgnoringModifiers:@"w" isARepeat:NO keyCode:13];
        [self.canvas keyUp:release];if(self.canvas.movementKeys)field_fail(__LINE__);
        for(unsigned i=0;i<16;i++)hg_field_app_step(self.canvas.application);
        if(!hg_field_app_player(self.canvas.application,&px,&pz)||px!=nx||pz!=nz)field_fail(__LINE__);
        char reloadError[256];if(![self.canvas loadMap:60 error:reloadError])field_fail(__LINE__);
        double runStart[3],runStep[3];unsigned runPose;
        hg_field_app_presentation(self.canvas.application,runStart,NULL);
        NSEvent *run=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:NSEventModifierFlagShift timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"W" charactersIgnoringModifiers:@"w" isARepeat:NO keyCode:13];
        [self.canvas keyDown:run];if(self.canvas.movementKeys!=0x42)field_fail(__LINE__);hg_field_app_step(self.canvas.application);
        hg_field_app_presentation(self.canvas.application,runStep,&runPose);
        if(runStep[2]!=runStart[2]-4 || (runPose!=18&&runPose!=20) || hg_field_app_steps(self.canvas.application)!=1)field_fail(__LINE__);
        NSEvent *shiftUp=[NSEvent keyEventWithType:NSEventTypeFlagsChanged location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"" charactersIgnoringModifiers:@"" isARepeat:NO keyCode:56];
        [self.canvas flagsChanged:shiftUp];if(self.canvas.movementKeys!=0x40)field_fail(__LINE__);
        for(unsigned tick=0;tick<3;tick++)hg_field_app_step(self.canvas.application);
        hg_field_app_presentation(self.canvas.application,runStep,NULL);if(runStep[2]!=runStart[2]-16)field_fail(__LINE__);
        hg_field_app_step(self.canvas.application);hg_field_app_presentation(self.canvas.application,runStep,NULL);
        if(runStep[2]!=runStart[2]-18 || hg_field_app_steps(self.canvas.application)!=2)field_fail(__LINE__);
        [self.canvas keyUp:release];if(![self.canvas loadMap:60 error:reloadError])field_fail(__LINE__);
        /* Exercise both sideways stride poses through actual modifier-bearing
         * input, including frames 26/32 that previously lost 42 opaque texels. */
        if(![[NSFileManager defaultManager] createDirectoryAtPath:@"build/evidence/hero-crop" withIntermediateDirectories:YES attributes:nil error:nil])field_fail(__LINE__);
        for(NSString *letter in @[@"a",@"d"]) {
            if(![self.canvas loadMap:60 error:reloadError])field_fail(__LINE__);
            DSCamera closeup=self.canvas.camera;closeup.scale=6;self.canvas.camera=closeup;
            hg_field_app_presentation(self.canvas.application,runStart,NULL);
            NSEvent *sideRun=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:NSEventModifierFlagShift timestamp:0 windowNumber:self.window.windowNumber context:nil characters:letter.uppercaseString charactersIgnoringModifiers:letter isARepeat:NO keyCode:0];
            NSEvent *sideUp=[NSEvent keyEventWithType:NSEventTypeKeyUp location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:letter charactersIgnoringModifiers:letter isARepeat:NO keyCode:0];
            [self.canvas keyDown:sideRun];unsigned poses=0;
            for(unsigned tick=0;tick<8;tick++) {
                hg_field_app_step(self.canvas.application);hg_field_app_presentation(self.canvas.application,runStep,&runPose);
                if(runPose==26||runPose==28||runPose==30||runPose==32) {
                    unsigned bit=1u<<(runPose-25);
                    if(!(poses&bit)) {
                        if(![self.canvas render])field_fail(__LINE__);
                        NSString *path=[NSString stringWithFormat:@"build/evidence/hero-crop/running-%u.png",runPose];
                        if(![[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:path atomically:YES])field_fail(__LINE__);
                        poses|=bit;
                    }
                }
            }
            BOOL west=[letter isEqualToString:@"a"];
            if(poses!=(west?10u:160u)||runStep[0]!=runStart[0]+(west?-32:32)||runStep[2]!=runStart[2])field_fail(__LINE__);
            [self.canvas keyUp:sideUp];
        }
        if(![self.canvas loadMap:60 error:reloadError])field_fail(__LINE__);
        NSString *route=@"aassa";
        for(NSUInteger i=0;i<route.length;i++) {
            NSString *letter=[route substringWithRange:NSMakeRange(i,1)];
            NSEvent *down=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:letter charactersIgnoringModifiers:letter isARepeat:NO keyCode:0];
            [self.canvas keyDown:down];for(unsigned tick=0;tick<8;tick++)hg_field_app_step(self.canvas.application);
            NSEvent *up=[NSEvent keyEventWithType:NSEventTypeKeyUp location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:letter charactersIgnoringModifiers:letter isARepeat:NO keyCode:0];[self.canvas keyUp:up];
        }
        NSEvent *confirm=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"\r" charactersIgnoringModifiers:@"\r" isARepeat:NO keyCode:36];
        void (^settleAutomatic)(HGFieldApp *)=^(HGFieldApp *session) {
            unsigned idle=0;
            for(unsigned tick=0;tick<4096;tick++) {
                hg_field_app_step(session);
                if(strstr(hg_field_app_dialogue(session),"isn’t available yet"))field_fail(__LINE__);
                if(hg_field_app_dialogue(session)[0]||hg_field_app_menu_pending(session))[self.canvas keyDown:confirm];
                if(!hg_field_app_interaction_busy(session)&&!hg_field_app_dialogue(session)[0]){if(++idle==2)return;}else idle=0;
            }
            field_fail(__LINE__);
        };
        [self.canvas keyDown:confirm];hg_field_app_step(self.canvas.application);
        if(strcmp(hg_field_app_dialogue(self.canvas.application),"New Bark Town\nWinds of a New Beginning"))field_fail(__LINE__);
        if(![self.canvas render])field_fail(__LINE__);
        [[NSFileManager defaultManager] createDirectoryAtPath:@"build/evidence" withIntermediateDirectories:YES attributes:nil error:nil];
        [[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@"build/evidence/new-bark-dialogue.png" atomically:YES];
        [self.canvas keyDown:confirm];hg_field_app_step(self.canvas.application);
        if(hg_field_app_dialogue(self.canvas.application)[0])field_fail(__LINE__);
        [self showBag:nil];if(hg_field_app_inspect_kind(self.canvas.application)!=1)field_fail(__LINE__);
        char menuText[4096];hg_field_app_inspect_text(self.canvas.application,menuText,sizeof(menuText));if(!strstr(menuText,"BAG"))field_fail(__LINE__);
        NSEvent *pocketKey=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"d" charactersIgnoringModifiers:@"d" isARepeat:NO keyCode:2];
        [self.canvas keyDown:pocketKey];for(unsigned hold=0;hold<120;hold++){hg_field_app_input(self.canvas.application,self.canvas.movementKeys);hg_field_app_step(self.canvas.application);}
        hg_field_app_inspect_text(self.canvas.application,menuText,sizeof(menuText));if(!strstr(menuText,"Medicine"))field_fail(__LINE__);
        self.canvas.movementKeys=0;hg_field_app_input(self.canvas.application,0);
        [[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@"build/evidence/field-bag-panel.png" atomically:YES];
        NSEvent *partyKey=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"p" charactersIgnoringModifiers:@"p" isARepeat:NO keyCode:35];
        [self.canvas keyDown:partyKey];if(hg_field_app_inspect_kind(self.canvas.application)!=2)field_fail(__LINE__);
        hg_field_app_inspect_text(self.canvas.application,menuText,sizeof(menuText));if(!strstr(menuText,"No Pokemon yet"))field_fail(__LINE__);
        NSEvent *closeKey=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"" charactersIgnoringModifiers:@"" isARepeat:NO keyCode:53];
        [self.canvas keyDown:closeKey];if(hg_field_app_inspect_kind(self.canvas.application))field_fail(__LINE__);
        if(!hg_field_app_player(self.canvas.application,&px,&pz))field_fail(__LINE__);
        [self.canvas keyDown:walk];[self.canvas keyUp:release];
        if(hg_field_app_advance(self.canvas.application,UINT64_C(1000000000)))field_fail(__LINE__);
        if(hg_field_app_advance(self.canvas.application,UINT64_C(1100000000))!=6)field_fail(__LINE__);
        if(!hg_field_app_player(self.canvas.application,&nx,&nz)||nx!=px||nz!=pz-1)field_fail(__LINE__);
        hg_field_app_advance(self.canvas.application,UINT64_C(1133333334));
        if(![self.canvas render])field_fail(__LINE__);
        if(![self.canvas loadMap:60 error:reloadError])field_fail(__LINE__);
        HGFieldApp *travelSession=self.canvas.application;NSString *houseRoute=@"wdddddddw";
        for(NSUInteger i=0;i<houseRoute.length;i++) {
            NSString *letter=[houseRoute substringWithRange:NSMakeRange(i,1)];
            NSEvent *down=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:letter charactersIgnoringModifiers:letter isARepeat:NO keyCode:0];
            NSEvent *up=[NSEvent keyEventWithType:NSEventTypeKeyUp location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:letter charactersIgnoringModifiers:letter isARepeat:NO keyCode:0];
            [self.canvas keyDown:down];hg_field_app_step(self.canvas.application);[self.canvas keyUp:up];
            for(unsigned tick=0;tick<7;tick++)hg_field_app_step(self.canvas.application);
            for(unsigned tick=0;tick<64&&hg_field_app_transition_active(self.canvas.application);tick++)hg_field_app_step(self.canvas.application);
        }
        if(hg_field_app_map(travelSession)!=63 || hg_field_app_steps(travelSession)!=8 || ![self.canvas render])field_fail(__LINE__);
        if(self.canvas.application!=travelSession || self.selectedMap!=63 || self.rows[self.table.selectedRow].unsignedIntValue!=63)field_fail(__LINE__);
        [[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@"build/evidence/new-bark-house-travel.png" atomically:YES];
        settleAutomatic(travelSession);
        if(hg_field_app_steps(travelSession)!=8)field_fail(__LINE__);
        NSEvent *exitHouse=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"s" charactersIgnoringModifiers:@"s" isARepeat:NO keyCode:1];
        [self.canvas keyDown:exitHouse];hg_field_app_step(self.canvas.application);
        /* Door travel now completes through the original asynchronous fade. */
        for(unsigned tick=0;tick<256&&hg_field_app_map(travelSession)!=60;tick++)hg_field_app_step(travelSession);
        if(hg_field_app_map(travelSession)!=60 || hg_field_app_steps(travelSession)!=8 || ![self.canvas render])field_fail(__LINE__);
        if(self.canvas.application!=travelSession || self.selectedMap!=60 || self.canvas.movementKeys)field_fail(__LINE__);
        if(![self.canvas loadMap:65 error:reloadError])field_fail(__LINE__);
        for(unsigned step=0;step<3;step++) {
            [self.canvas keyDown:walk];hg_field_app_step(self.canvas.application);[self.canvas keyUp:release];
            for(unsigned tick=1;tick<8;tick++)hg_field_app_step(self.canvas.application);
        }
        NSEvent *faceNpc=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"d" charactersIgnoringModifiers:@"d" isARepeat:NO keyCode:2];
        [self.canvas keyDown:faceNpc];hg_field_app_step(self.canvas.application);self.canvas.movementKeys=0;hg_field_app_clear_input(self.canvas.application);
        [self.canvas keyDown:confirm];hg_field_app_step(self.canvas.application);
        if(strcmp(hg_field_app_dialogue_page(self.canvas.application),"PIKACHU is an evolved Pokémon.")||![self.canvas render])field_fail(__LINE__);
        [[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@"build/evidence/new-bark-npc-dialogue.png" atomically:YES];
        unsigned pages=0;
        while(hg_field_app_dialogue(self.canvas.application)[0]&&pages++<16){[self.canvas keyDown:confirm];hg_field_app_step(self.canvas.application);}
        if(pages<3||pages>=16||hg_field_app_dialogue(self.canvas.application)[0]||![self.canvas render])field_fail(__LINE__);
        if(![self.canvas loadMap:63 error:reloadError])field_fail(__LINE__);
        settleAutomatic(self.canvas.application);
        hg_ui_test_approach(self.canvas.application,2);
        HGStoryState *story=hg_field_app_story(self.canvas.application);unsigned long long beforeStory=hg_story_revision(story);
        [self.canvas keyDown:confirm];hg_field_app_step(self.canvas.application);
        if(strcmp(hg_field_app_dialogue(self.canvas.application),"Professor Elm has been waiting for\nyou, dear. Please, go and help him.")||hg_story_revision(story)!=beforeStory||![self.canvas render])field_fail(__LINE__);
        [[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@"build/evidence/new-bark-mom-story.png" atomically:YES];
        [self.canvas keyDown:confirm];hg_field_app_step(self.canvas.application);
        settleAutomatic(self.canvas.application);
        if(hg_field_app_dialogue(self.canvas.application)[0]||hg_story_revision(story)!=beforeStory+1||![self.canvas render])field_fail(__LINE__);
        NSArray *conversations=@[
            @{@"map":@60,@"script":@16,@"text":@"Yo, Ethan!",@"image":@"new-bark-breeder-dialogue.png"},
            @{@"map":@61,@"script":@1,@"text":@"Professor Elm: If a wild Pokémon",@"image":@"new-bark-elm-dialogue.png"},
            @{@"map":@60,@"script":@1,@"text":@"So this is the famous Elm",@"image":@"new-bark-rival-dialogue.png"}
        ];
        for(NSDictionary *conversation in conversations) {
            if(![self.canvas loadMap:[conversation[@"map"] unsignedIntValue] error:reloadError])field_fail(__LINE__);
            hg_ui_test_approach(self.canvas.application,[conversation[@"script"] unsignedIntValue]);
            [self.canvas resetCamera];
            story=hg_field_app_story(self.canvas.application);beforeStory=hg_story_revision(story);
            [self.canvas keyDown:confirm];hg_field_app_step(self.canvas.application);
            if(!strstr(hg_field_app_dialogue(self.canvas.application),[conversation[@"text"] UTF8String])||hg_story_revision(story)!=beforeStory||![self.canvas render])field_fail(__LINE__);
            NSString *path=[@"build/evidence" stringByAppendingPathComponent:conversation[@"image"]];
            [[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:path atomically:YES];
            unsigned ticks=0;BOOL second=NO,jumped=NO;double origin[3];hg_field_app_presentation(self.canvas.application,origin,NULL);
            while(hg_field_app_interaction_busy(self.canvas.application)&&ticks++<2048) {
                if(strstr(hg_field_app_dialogue(self.canvas.application),"What are you staring at?"))second=YES;
                if(hg_field_app_dialogue(self.canvas.application)[0])[self.canvas keyDown:confirm];
                hg_field_app_step(self.canvas.application);double world[3];hg_field_app_presentation(self.canvas.application,world,NULL);
                if(!jumped&&world[1]>origin[1]+1) {
                    jumped=YES;[self.canvas resetCamera];if(![self.canvas render])field_fail(__LINE__);
                    [[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@"build/evidence/new-bark-rival-jump.png" atomically:YES];
                }
            }
            if(ticks>=2048||hg_story_revision(story)!=beforeStory+1||![self.canvas render])field_fail(__LINE__);
            if([conversation[@"image"] isEqualToString:@"new-bark-rival-dialogue.png"]&&(!second||!jumped))field_fail(__LINE__);
        }
        hg_ui_test_approach(self.canvas.application,16);
        hg_field_app_interact(self.canvas.application);hg_field_app_step(self.canvas.application);
        if(!hg_field_app_interaction_busy(self.canvas.application))field_fail(__LINE__);
        NSString *retainedPage=[NSString stringWithUTF8String:hg_field_app_dialogue_page(self.canvas.application)];
        char exploreError[256];HGFieldApp *retained=self.canvas.application;int retainedX,retainedZ;
        if(!hg_field_app_player(retained,&retainedX,&retainedZ))field_fail(__LINE__);
        unsigned long long retainedRevision=hg_story_revision(hg_field_app_story(retained));
        self.exploration.state=NSControlStateValueOn;[self changeExploration:nil];
        if(self.canvas.application==retained||self.canvas.suspendedStory!=retained||!hg_field_app_is_exploring(self.canvas.application))field_fail(__LINE__);
        if(![self.canvas loadMap:67 error:exploreError])field_fail(__LINE__);
        hg_ui_test_approach_center_door(self.canvas.application);[self.canvas resetCamera];
        [self.canvas keyDown:walk];hg_field_app_step(self.canvas.application);[self.canvas keyUp:release];
        unsigned doorFrames=0;BOOL doorImage=NO;
        for(unsigned tick=0;tick<64&&hg_field_app_transition_active(self.canvas.application);tick++){
            if(hg_field_app_error(self.canvas.application)[0]||![self.canvas render])field_fail(__LINE__);
            if(hg_field_app_map(self.canvas.application)==67&&hg_field_app_brightness(self.canvas.application)==0){
                if(++doorFrames==7){[[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@"build/evidence/center-door-opening-ui.png" atomically:YES];doorImage=YES;}
            }
            hg_field_app_step(self.canvas.application);
        }
        if(!doorImage||hg_field_app_map(self.canvas.application)!=69||hg_field_app_transition_active(self.canvas.application))field_fail(__LINE__);
        NSEvent *exitDoor=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.canvas.window.windowNumber context:nil characters:@"s" charactersIgnoringModifiers:@"s" isARepeat:NO keyCode:1];
        [self.canvas keyDown:exitDoor];hg_field_app_step(self.canvas.application);[self.canvas keyUp:release];
        unsigned exitFrames=0;BOOL exitImage=NO;
        for(unsigned tick=0;tick<80&&hg_field_app_transition_active(self.canvas.application);tick++){
            if(hg_field_app_error(self.canvas.application)[0]||![self.canvas render])field_fail(__LINE__);
            if(hg_field_app_map(self.canvas.application)==67&&hg_field_app_brightness(self.canvas.application)==0){
                if(++exitFrames==5){[[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@"build/evidence/center-door-exit-ui.png" atomically:YES];exitImage=YES;}
            }
            hg_field_app_step(self.canvas.application);
        }
        if(!exitImage||hg_field_app_map(self.canvas.application)!=67||hg_field_app_transition_active(self.canvas.application))field_fail(__LINE__);
        if(![self.canvas loadMap:143 error:exploreError])field_fail(__LINE__);
        for(unsigned i=0;i<64;i++)hg_field_app_step(self.canvas.application);
        if(hg_field_app_interaction_busy(self.canvas.application)||![self.canvas render])field_fail(__LINE__);
        [[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@"build/evidence/exploration-mr-pokemon.png" atomically:YES];
        self.exploration.state=NSControlStateValueOff;[self changeExploration:nil];
        int restoredX,restoredZ;
        if(self.canvas.application!=retained||self.canvas.suspendedStory||!hg_field_app_player(retained,&restoredX,&restoredZ)||restoredX!=retainedX||restoredZ!=retainedZ||hg_story_revision(hg_field_app_story(retained))!=retainedRevision)field_fail(__LINE__);
        if(![retainedPage isEqualToString:[NSString stringWithUTF8String:hg_field_app_dialogue_page(retained)]]||!hg_field_app_interaction_busy(retained))field_fail(__LINE__);
        settleAutomatic(retained);
        HGFieldApp *beforeJump=self.canvas.application;HGStoryState *beforeJumpStory=hg_field_app_story(beforeJump);unsigned long long beforeJumpRevision=hg_story_revision(beforeJumpStory);
        NSInteger checkpointIndex=-1;for(NSInteger i=1;i<self.checkpoints.numberOfItems;i++)if([self.checkpoints.itemArray[i].representedObject hasSuffix:@"152-2.hgcheckpoint"]){checkpointIndex=i;break;}
        if(checkpointIndex<0)field_fail(__LINE__);[self.checkpoints selectItemAtIndex:checkpointIndex];[self checkpointChanged:self.checkpoints];
        if(self.canvas.application==beforeJump||hg_field_app_map(self.canvas.application)!=67||!self.checkpointBack.enabled)field_fail(__LINE__);
        NSEvent *rivalStep=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"d" charactersIgnoringModifiers:@"d" isARepeat:NO keyCode:2];
        [self.canvas keyDown:rivalStep];hg_field_app_step(self.canvas.application);self.canvas.movementKeys=0;hg_field_app_clear_input(self.canvas.application);
        BOOL battleSeen=NO,battleDone=NO,battleImage=NO;
        for(unsigned tick=0;tick<16000;tick++){
            hg_field_app_step(self.canvas.application);if(hg_field_app_error(self.canvas.application)[0])field_fail(__LINE__);
            if(hg_field_app_battle_active(self.canvas.application)){
                battleSeen=YES;if(!hg_field_app_battle_trainer(self.canvas.application))field_fail(__LINE__);
                if(hg_field_app_battle_state(self.canvas.application)==HG_BATTLE_MOVES&&!battleImage){if(![self.canvas render])field_fail(__LINE__);[[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@"build/evidence/rival-checkpoint-ui.png" atomically:YES];battleImage=YES;}
                [self.canvas keyDown:confirm];
            }else if(hg_field_app_dialogue(self.canvas.application)[0])[self.canvas keyDown:confirm];
            if(battleSeen&&!hg_field_app_interaction_busy(self.canvas.application)&&!hg_field_app_dialogue(self.canvas.application)[0]){battleDone=YES;break;}
        }
        if(!battleSeen||!battleDone||!battleImage)field_fail(__LINE__);
        hg_ui_test_return_to_elm(self.canvas.application);BOOL nameSeen=NO,nameDone=NO;
        for(unsigned tick=0;tick<8192;tick++){
            hg_field_app_step(self.canvas.application);if(hg_field_app_error(self.canvas.application)[0])field_fail(__LINE__);
            if(hg_field_app_name_pending(self.canvas.application)){
                nameSeen=YES;self.canvas.movementKeys=0;
                for(NSString *letter in @[@"S",@"i",@"l",@"v",@"e",@"r"]){NSEvent *typed=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:letter charactersIgnoringModifiers:letter isARepeat:NO keyCode:1];[self.canvas keyDown:typed];}
                if(strcmp(hg_field_app_name_text(self.canvas.application),"Silver")||self.canvas.movementKeys)field_fail(__LINE__);
                if(![self.canvas render])field_fail(__LINE__);[[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@"build/evidence/rival-naming-ui.png" atomically:YES];
                [self.canvas keyDown:confirm];
            }else if(hg_field_app_menu_pending(self.canvas.application)){
                hg_field_app_clear_input(self.canvas.application);hg_field_app_input(self.canvas.application,0x40);hg_field_app_step(self.canvas.application);hg_field_app_clear_input(self.canvas.application);[self.canvas keyDown:confirm];
            }else if(hg_field_app_dialogue(self.canvas.application)[0])[self.canvas keyDown:confirm];
            if(nameSeen&&!hg_field_app_interaction_busy(self.canvas.application)){nameDone=YES;break;}
        }
        if(!nameSeen||!nameDone)field_fail(__LINE__);
        for(unsigned tick=0;tick<32;tick++)hg_field_app_step(self.canvas.application);
        if(![self saveToPath:@"build/evidence/native-save.sav"])field_fail(__LINE__);
        char continueError[256];HGFieldApp *continued=hg_field_app_continue("build/evidence/native-save.sav",continueError,sizeof(continueError));
        if(!continued)field_fail(__LINE__);DSCamera continuedCamera;hg_field_app_camera(continued,&continuedCamera,WIDTH,HEIGHT);
        if(!hg_field_app_render(continued,&continuedCamera,self.canvas.bitmap.bitmapData,WIDTH,HEIGHT))field_fail(__LINE__);
        hg_field_app_free(continued);

        [self checkpointBack:nil];if(self.canvas.application!=beforeJump||hg_field_app_story(self.canvas.application)!=beforeJumpStory||hg_story_revision(beforeJumpStory)!=beforeJumpRevision)field_fail(__LINE__);
        NSInteger tutorialIndex=-1;for(NSInteger i=1;i<self.checkpoints.numberOfItems;i++)if([self.checkpoints.itemArray[i].representedObject hasSuffix:@"152-4.hgcheckpoint"]){tutorialIndex=i;break;}
        if(tutorialIndex<0)field_fail(__LINE__);[self.checkpoints selectItemAtIndex:tutorialIndex];[self checkpointChanged:self.checkpoints];
        NSEvent *tutorialStep=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"a" charactersIgnoringModifiers:@"a" isARepeat:NO keyCode:0];
        [self.canvas keyDown:tutorialStep];hg_field_app_step(self.canvas.application);self.canvas.movementKeys=0;hg_field_app_clear_input(self.canvas.application);
        BOOL tutorialSeen=NO,tutorialDone=NO,captureImage=NO;
        for(unsigned tick=0;tick<8192;tick++){
            hg_field_app_step(self.canvas.application);if(hg_field_app_error(self.canvas.application)[0])field_fail(__LINE__);
            if(hg_field_app_battle_active(self.canvas.application)){
                tutorialSeen=YES;if(!hg_field_app_battle_tutorial(self.canvas.application)||hg_field_app_battle_trainer(self.canvas.application))field_fail(__LINE__);
                if(hg_field_app_capture_phase(self.canvas.application)==5&&!captureImage){if(![self.canvas render])field_fail(__LINE__);[[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@"build/evidence/catching-tutorial-ui.png" atomically:YES];captureImage=YES;}
            }
            if(hg_field_app_dialogue(self.canvas.application)[0])[self.canvas keyDown:confirm];
            if(tutorialSeen&&!hg_field_app_interaction_busy(self.canvas.application)){tutorialDone=YES;break;}
        }
        if(!tutorialSeen||!tutorialDone||!captureImage)field_fail(__LINE__);char tutorialError[256];if(!hg_field_app_checkpoint_write(self.canvas.application,"build/checkpoints/ui-tutorial.hgcheckpoint",5,tutorialError,sizeof(tutorialError)))field_fail(__LINE__);
        [self checkpointBack:nil];if(self.canvas.application!=beforeJump||hg_field_app_story(self.canvas.application)!=beforeJumpStory||hg_story_revision(beforeJumpStory)!=beforeJumpRevision)field_fail(__LINE__);
        for(unsigned recoveryCase=0;recoveryCase<2;recoveryCase++) {
        NSString *lossSuffix=recoveryCase?@"152-5.hgcheckpoint":@"152-1.hgcheckpoint";
        NSInteger lossIndex=-1;for(NSInteger i=1;i<self.checkpoints.numberOfItems;i++)if([self.checkpoints.itemArray[i].representedObject hasSuffix:lossSuffix]){lossIndex=i;break;}
        if(lossIndex<0)field_fail(__LINE__);[self.checkpoints selectItemAtIndex:lossIndex];[self checkpointChanged:self.checkpoints];
        hg_test_prepare_defeat(self.canvas.application);BOOL lossSeen=NO,lossDone=NO,blackoutSeen=NO;unsigned lossSteps=0,centerFrames=0,battleMenuPhase=0;
        NSEvent *lossDown=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"s" charactersIgnoringModifiers:@"s" isARepeat:NO keyCode:1];
        NSEvent *battleEscape=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"" charactersIgnoringModifiers:@"" isARepeat:NO keyCode:53];
        for(unsigned tick=0;tick<20000;tick++){
            if(!lossSeen&&!hg_field_app_battle_active(self.canvas.application)&&!hg_field_app_interaction_busy(self.canvas.application)&&!(tick%32)){
                hg_field_app_input(self.canvas.application,lossSteps++&1?0x20:0x10);
            }
            hg_field_app_step(self.canvas.application);hg_field_app_clear_input(self.canvas.application);
            if(recoveryCase&&hg_field_app_map(self.canvas.application)==69){
                if(![self.canvas render])field_fail(__LINE__);
                if(++centerFrames==120)[[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@"build/evidence/center-recovery-ui.png" atomically:YES];
            }
            if(hg_field_app_error(self.canvas.application)[0])field_fail(__LINE__);
            if(hg_field_app_battle_active(self.canvas.application)){
                lossSeen=YES;int action=hg_field_app_battle_state(self.canvas.application);
                if(action==HG_BATTLE_COMMAND&&battleMenuPhase==0){if(!hg_field_app_battle_choose(self.canvas.application,2))field_fail(__LINE__);battleMenuPhase=1;}
                else if(action==HG_BATTLE_BAG&&battleMenuPhase==1){if(![self.canvas render])field_fail(__LINE__);[[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@"build/evidence/battle-bag-ui.png" atomically:YES];[self.canvas keyDown:battleEscape];battleMenuPhase=2;}
                else if(action==HG_BATTLE_COMMAND&&battleMenuPhase==2){if(!hg_field_app_battle_choose(self.canvas.application,3))field_fail(__LINE__);battleMenuPhase=3;}
                else if(action==HG_BATTLE_PARTY&&battleMenuPhase==3){if(![self.canvas render])field_fail(__LINE__);[self.canvas keyDown:battleEscape];battleMenuPhase=4;}
                else if(action==HG_BATTLE_COMMAND||action==HG_BATTLE_MESSAGE)[self.canvas keyDown:confirm];
                else if(action==HG_BATTLE_MOVES){[self.canvas keyDown:lossDown];hg_field_app_step(self.canvas.application);self.canvas.movementKeys=0;hg_field_app_clear_input(self.canvas.application);[self.canvas keyDown:confirm];}
            }else if(hg_field_app_dialogue(self.canvas.application)[0]){
                const char *lossText=hg_field_app_dialogue(self.canvas.application);
                if(recoveryCase?(strstr(lossText,"scurried to")&&strstr(lossText,"Center, protecting")):(strstr(lossText,"scurried back")&&strstr(lossText,"home, protecting")))blackoutSeen=YES;
                if(![self.canvas render])field_fail(__LINE__);[self.canvas keyDown:confirm];
            }
            if(lossSeen&&hg_field_app_map(self.canvas.application)==(recoveryCase?69:63)&&!hg_field_app_interaction_busy(self.canvas.application)){lossDone=YES;break;}
        }
        if(!lossSeen||!blackoutSeen||!lossDone||battleMenuPhase!=4||![self.canvas render]){fprintf(stderr,"Blackout UI: loss=%d message=%d done=%d map=%u state=%d text=%s\n",lossSeen,blackoutSeen,lossDone,hg_field_app_map(self.canvas.application),hg_field_app_battle_state(self.canvas.application),hg_field_app_dialogue(self.canvas.application));field_fail(__LINE__);}
        if(recoveryCase){
            hg_ui_test_prepare_nurse(self.canvas.application);[self.canvas keyDown:confirm];BOOL nurseDone=NO,nurseImage=NO;
            for(unsigned tick=0;tick<2048;tick++){
                hg_field_app_step(self.canvas.application);
                if(hg_field_app_error(self.canvas.application)[0]||![self.canvas render])field_fail(__LINE__);
                if(tick==100){[[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@"build/evidence/nurse-healing-ui.png" atomically:YES];nurseImage=YES;}
                if(hg_field_app_menu_pending(self.canvas.application)){
                    if(!hg_field_app_menu_choice_supported(self.canvas.application,0))field_fail(__LINE__);
                    [self.canvas keyDown:walk];hg_field_app_step(self.canvas.application);[self.canvas keyUp:release];[self.canvas keyDown:confirm];
                }else if(hg_field_app_dialogue(self.canvas.application)[0])[self.canvas keyDown:confirm];
                if(tick&&!hg_field_app_interaction_busy(self.canvas.application)){nurseDone=YES;break;}
            }
            if(!nurseDone||!nurseImage||!hg_ui_test_nurse_healed(self.canvas.application))field_fail(__LINE__);
            hg_ui_test_prepare_mart(self.canvas.application);[self.canvas keyDown:confirm];BOOL martSeen=NO,martDone=NO;
            for(unsigned tick=0;tick<2048;tick++){
                hg_field_app_step(self.canvas.application);if(hg_field_app_error(self.canvas.application)[0]||![self.canvas render])field_fail(__LINE__);
                if(hg_field_app_shop(self.canvas.application)){
                    if(!martSeen){[[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@"build/evidence/mart-ui.png" atomically:YES];martSeen=YES;}
                    hg_field_app_shop_cancel(self.canvas.application);
                }else if(hg_field_app_menu_pending(self.canvas.application)){
                    if(martSeen){[self.canvas keyDown:walk];hg_field_app_step(self.canvas.application);[self.canvas keyUp:release];}
                    [self.canvas keyDown:confirm];
                }else if(hg_field_app_dialogue(self.canvas.application)[0])[self.canvas keyDown:confirm];
                if(martSeen&&!hg_field_app_interaction_busy(self.canvas.application)){martDone=YES;break;}
            }
            if(!martSeen||!martDone){fprintf(stderr,"Mart UI: seen=%d done=%d text=%s\n",martSeen,martDone,hg_field_app_dialogue(self.canvas.application));field_fail(__LINE__);}

        }
        [self checkpointBack:nil];if(self.canvas.application!=beforeJump||hg_story_revision(beforeJumpStory)!=beforeJumpRevision)field_fail(__LINE__);
        }
        {
            char phoneError[256];HGFieldApp *previous=self.canvas.application;
            HGFieldApp *mother=hg_field_app_checkpoint_load("build/checkpoints/158-5.hgcheckpoint",phoneError,sizeof(phoneError));if(!mother)field_fail(__LINE__);
            self.canvas.application=mother;hg_ui_test_prepare_mother(mother);[self.canvas resetCamera];BOOL seen=NO,done=NO;
            NSEvent *answer=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@" " charactersIgnoringModifiers:@" " isARepeat:NO keyCode:49];
            for(unsigned tick=0;tick<4096;tick++){
                hg_field_app_step(mother);if(hg_field_app_error(mother)[0]||![self.canvas render])field_fail(__LINE__);
                if(hg_field_app_menu_pending(mother)){
                    if(!seen){[[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@"build/evidence/mother-call-ui.png" atomically:YES];seen=YES;}
                    [self.canvas keyDown:answer];
                }else if(hg_field_app_dialogue(mother)[0])[self.canvas keyDown:answer];
                if(seen&&!hg_field_app_interaction_busy(mother)){done=YES;break;}
            }
            if(!seen||!done)field_fail(__LINE__);
            if(![self.canvas render])field_fail(__LINE__);
            [[self.canvas.bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:@"build/evidence/mother-call-return-ui.png" atomically:YES];
            self.canvas.application=previous;hg_field_app_free(mother);[self.canvas resetCamera];
        }
        [self.canvas display];fprintf(stderr,"Map browser smoke: Elm, breeder and rival conversations, rival shove/jump and original completion, Mom, NPC paging, retained travel/session statistics, map-list/camera synchronization, validated checkpoint/Back, visible rival battle/cutscene return, typed rival naming and Elm Egg handoff, automatic tutorial capture/return and five-Ball checkpoint, original wild loss/home and Center recovery with healing presentation, visible Center entrance and exit doors, normal Nurse Joy healing, original Mart clerk/menu and native shop panel, walk/run, town sign, input, timing, search, exploration/session restoration and AppKit display passed\n");[NSApp terminate:nil];
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
        unsigned exploreMap=0;BOOL explore=argc==3&&!strcmp(argv[1],"--explore");
        if(explore) {
            char *end;unsigned long map=strtoul(argv[2],&end,10);
            if(!argv[2][0]||*end||map>=hg_field_map_count())return 2;exploreMap=(unsigned)map;
        }
        if(!explore&&argc!=1 && (argc!=2 || (strcmp(argv[1],"--smoke")&&strcmp(argv[1],"--audio-smoke")))) { fprintf(stderr,"Usage: heartgold_field_viewer [--explore ID | --smoke | --audio-smoke | --render-only path.png | --render-map ID path.png | --render-character ID path.png]\n");return 2; }
        NSApplication *app=[NSApplication sharedApplication];[app setActivationPolicy:NSApplicationActivationPolicyRegular];
        NSMenu *main=[NSMenu new],*menu=[NSMenu new];NSMenuItem *root=[NSMenuItem new];[main addItem:root];root.submenu=menu;[menu addItemWithTitle:@"Quit HeartGold MX Field" action:@selector(terminate:) keyEquivalent:@"q"];app.mainMenu=main;
        FieldApp *delegate=[FieldApp new];delegate.smoke=argc==2;delegate.audioSmoke=argc==2&&!strcmp(argv[1],"--audio-smoke");delegate.exploreAtLaunch=explore;delegate.initialMap=exploreMap;app.delegate=delegate;[app run];
    }return 0;
}
