#include "gl_layer.h"

@interface GLLayer () {
    app::DisplayGL* mDisplayGL;
}

@end

@implementation GLLayer

- (instancetype)initWithDisplayGL:(app::DisplayGL*)displayGL {
    self = [super init];
    if (self) {
        mDisplayGL = displayGL;
    }
    return self;
}

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
    return mDisplayGL->pixelFormat();
}

- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat {
    // Set up KVO to detect resizing.
    [self addObserver:self forKeyPath:@"bounds" options:0 context:nil];

    // Call OpenGL activation callback.
    CGLSetCurrentContext(mDisplayGL->context());
    int scaled_width = self.frame.size.width * self.contentsScale;
    int scaled_height = self.frame.size.height * self.contentsScale;
    const app::Size scaled_size{
        .width = static_cast<int>(scaled_width),
        .height = static_cast<int>(scaled_height),
    };
    appWindow->onOpenGLActivate(scaled_size);

    return mDisplayGL->context();
}

- (BOOL)canDrawInCGLContext:(CGLContextObj)glContext
                pixelFormat:(CGLPixelFormatObj)pixelFormat
               forLayerTime:(CFTimeInterval)timeInterval
                displayTime:(const CVTimeStamp*)timeStamp {
    return true;
}

- (void)drawInCGLContext:(CGLContextObj)glContext
             pixelFormat:(CGLPixelFormatObj)pixelFormat
            forLayerTime:(CFTimeInterval)timeInterval
             displayTime:(const CVTimeStamp*)timeStamp {
    CGLSetCurrentContext(glContext);

    // TODO: For debugging; remove this.
    // [NSApp terminate:nil];

    int scaled_width = self.frame.size.width * self.contentsScale;
    int scaled_height = self.frame.size.height * self.contentsScale;
    const app::Size scaled_size{
        .width = static_cast<int>(scaled_width),
        .height = static_cast<int>(scaled_height),
    };
    appWindow->onDraw(scaled_size);

    // Calls glFlush() by default.
    [super drawInCGLContext:glContext
                pixelFormat:pixelFormat
               forLayerTime:timeInterval
                displayTime:timeStamp];
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context {
    CGLSetCurrentContext(mDisplayGL->context());

    float scaled_width = self.frame.size.width * self.contentsScale;
    float scaled_height = self.frame.size.height * self.contentsScale;
    const app::Size scaled_size{
        .width = static_cast<int>(scaled_width),
        .height = static_cast<int>(scaled_height),
    };
    appWindow->onResize(scaled_size);
}

// We shouldn't release the CGLContextObj since it isn't owned by this object.
- (void)releaseCGLContext:(CGLContextObj)glContext {
}

// We shouldn't release the CGLPixelFormatObj since it isn't owned by this object.
- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pixelFormat {
}

@end
