#include "app.h"
#include "ui/window/editor_window.h"
#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate> {
    NSMenu* menu;
    EditorWindow2 editor_window;
}

@end

@implementation AppDelegate

- (instancetype)init {
    self = [super init];
    if (self) {
        // TODO: Initialize editor window here instead of hard coding window size.
    }
    return self;
}

- (void)applicationWillFinishLaunching:(NSNotification*)notification {
    NSString* appName = NSBundle.mainBundle.infoDictionary[@"CFBundleName"];
    menu = [[NSMenu alloc] init];
    NSMenuItem* appMenu = [[NSMenuItem alloc] init];
    appMenu.submenu = [[NSMenu alloc] init];
    [appMenu.submenu addItemWithTitle:[NSString stringWithFormat:@"About %@", appName]
                               action:@selector(showAboutPanel)
                        keyEquivalent:@""];
    [appMenu.submenu addItem:[NSMenuItem separatorItem]];
    [appMenu.submenu addItemWithTitle:[NSString stringWithFormat:@"Quit %@", appName]
                               action:@selector(terminate:)
                        keyEquivalent:@"q"];
    [menu addItem:appMenu];
    NSApplication.sharedApplication.mainMenu = menu;

    editor_window.show();
}

- (void)showAboutPanel {
    [NSApplication.sharedApplication orderFrontStandardAboutPanel:menu];
    [NSApplication.sharedApplication activateIgnoringOtherApps:true];
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication*)app {
    return true;
}

@end

class App::impl {
public:
    NSApplication* app;
};

App::App() : pimpl{new impl{}} {
    pimpl->app = NSApplication.sharedApplication;
    AppDelegate* appDelegate = [[AppDelegate alloc] init];

    pimpl->app.activationPolicy = NSApplicationActivationPolicyRegular;
    pimpl->app.delegate = appDelegate;
}

void App::run() {
    @autoreleasepool {
        [pimpl->app run];
    }
}
