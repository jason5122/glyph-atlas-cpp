#import "EditorView.h"
#import "base/buffer.h"
#import "base/syntax_highlighter.h"
#import "ui/renderer/rect_renderer.h"
#import "ui/renderer/text_renderer.h"
#import "util/file_util.h"
#import "util/profile_util.h"
#import <chrono>
#import <fstream>
#import <iostream>
#import <limits>
#import <sstream>
#import <string>
#import <thread>
#import <vector>

@interface OpenGLLayer : CAOpenGLLayer {
@public
    CGFloat scroll_x;
    CGFloat scroll_y;
    CGFloat cursor_start_x;
    CGFloat cursor_start_y;
    CGFloat cursor_end_x;
    CGFloat cursor_end_y;

    float editor_offset_x;
    float editor_offset_y;

    // @private
    TextRenderer text_renderer;
    Buffer buffer;

@private
    RectRenderer rect_renderer;
    SyntaxHighlighter highlighter;
}

- (void)insertUTF8String:(const char*)str bytes:(size_t)bytes;

- (void)parseBuffer;

- (void)editBuffer:(size_t)bytes;

- (void)setRendererCursorPositions;

- (CGFloat)maxScrollX;

- (CGFloat)maxScrollY;

@end

@interface EditorView () {
@public
    OpenGLLayer* openGLLayer;

@private
    bool isDragging;
}
@end

@implementation EditorView

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        openGLLayer = [OpenGLLayer layer];
        // openGLLayer.needsDisplayOnBoundsChange = true;
        // openGLLayer.asynchronous = true;
        self.layer = openGLLayer;

        // Fixes blurriness on HiDPI displays.
        // https://bugzilla.gnome.org/show_bug.cgi?id=765194
        self.layer.contentsScale = NSScreen.mainScreen.backingScaleFactor;

        NSTrackingAreaOptions options =
            NSTrackingMouseMoved | NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow;
        trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds
                                                    options:options
                                                      owner:self
                                                   userInfo:nil];
        [self addTrackingArea:trackingArea];

        isDragging = false;
    }
    return self;
}

- (void)updateTrackingAreas {
    [super updateTrackingAreas];
    [self removeTrackingArea:trackingArea];

    NSTrackingAreaOptions options =
        NSTrackingMouseMoved | NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow;
    trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds
                                                options:options
                                                  owner:self
                                               userInfo:nil];
    [self addTrackingArea:trackingArea];
}

// FIXME: Also set cursor style when clicking window to focus.
// This can be reproduced by opening another window in front of this one, and clicking on this
// without moving the mouse.
- (void)setCursorStyle:(NSEvent*)event {
    CGFloat mouse_x = event.locationInWindow.x;
    CGFloat mouse_y = event.locationInWindow.y;
    mouse_y = openGLLayer.frame.size.height - mouse_y;  // Set origin at top left.

    if (mouse_x < openGLLayer->editor_offset_x) {
        [NSCursor.resizeLeftRightCursor set];
    } else if (mouse_y < openGLLayer->editor_offset_y) {
        [NSCursor.arrowCursor set];
    } else {
        [NSCursor.IBeamCursor set];
    }
}

- (void)mouseMoved:(NSEvent*)event {
    [self setCursorStyle:event];
}

- (void)mouseEntered:(NSEvent*)event {
    [self setCursorStyle:event];
}

// We need to override `cursorUpdate` to stop the event from being passed up in the chain.
// Without this, our `mouseEntered` NSCursor set will be overridden.
// https://stackoverflow.com/a/20197686
- (void)cursorUpdate:(NSEvent*)event {
}

- (void)mouseExited:(NSEvent*)event {
    [NSCursor.arrowCursor set];
}

- (void)scrollWheel:(NSEvent*)event {
    if (event.type == NSEventTypeScrollWheel) {
        CGFloat dx = -event.scrollingDeltaX;
        CGFloat dy = -event.scrollingDeltaY;

        // https://linebender.gitbook.io/linebender-graphics-wiki/mouse-wheel#macos
        if (!event.hasPreciseScrollingDeltas) {
            dx *= 16;
            dy *= 16;
        }

        openGLLayer->scroll_x =
            std::clamp(openGLLayer->scroll_x + dx, 0.0, [openGLLayer maxScrollX]);
        openGLLayer->scroll_y =
            std::clamp(openGLLayer->scroll_y + dy, 0.0, [openGLLayer maxScrollY]);

        // https://developer.apple.com/documentation/appkit/nsevent/1527943-pressedmousebuttons?language=objc
        if (NSEvent.pressedMouseButtons & (1 << 0)) {
            CGFloat mouse_x = event.locationInWindow.x;
            CGFloat mouse_y = event.locationInWindow.y;
            mouse_y = openGLLayer.frame.size.height - mouse_y;  // Set origin at top left.

            mouse_x -= openGLLayer->editor_offset_x;
            mouse_y -= openGLLayer->editor_offset_y;

            CGFloat max_mouse_x =
                (openGLLayer->text_renderer.longest_line_x / openGLLayer.contentsScale) -
                (openGLLayer.frame.size.width - mouse_x);
            CGFloat max_mouse_y =
                (openGLLayer->buffer.lineCount() * openGLLayer->text_renderer.line_height) /
                openGLLayer.contentsScale;

            openGLLayer->cursor_end_x = std::clamp(openGLLayer->cursor_end_x + dx, mouse_x,
                                                   max_mouse_x + openGLLayer->editor_offset_x);

            // Unlike cursor_end_x, our mouse could be well below the cursor.
            // This is only possible if scrolling past the end is enabled.
            // For upwards scrolling, don't actually scroll the cursor until our mouse meets it.
            if (!(dy < 0 && mouse_y + openGLLayer->scroll_y > openGLLayer->cursor_end_y)) {
                openGLLayer->cursor_end_y =
                    std::clamp(openGLLayer->cursor_end_y + dy, mouse_y, max_mouse_y);
            }

            [openGLLayer setRendererCursorPositions];
        }

        [self.layer setNeedsDisplay];
    }
}

const char* hex(char c) {
    const char REF[] = "0123456789ABCDEF";
    static char output[3] = "XX";
    output[0] = REF[0x0f & c >> 4];
    output[1] = REF[0x0f & c];
    return output;
}

- (void)keyDown:(NSEvent*)event {
    const char* str = event.characters.UTF8String;
    size_t bytes = strlen(str);

    if (bytes > 0) {
        for (size_t i = 0; str[i] != '\0'; i++) {
            std::cerr << hex(str[i]) << " ";
        }
        std::cerr << '\n';

        if (str[0] == 0x0D) {
            std::cerr << "new line inserted\n";
            str = "\n";
        }

        [openGLLayer insertUTF8String:str bytes:bytes];
        [self.layer setNeedsDisplay];
    }
}

- (void)mouseDown:(NSEvent*)event {
    CGFloat mouse_x = event.locationInWindow.x;
    CGFloat mouse_y = event.locationInWindow.y;
    mouse_y = openGLLayer.frame.size.height - mouse_y;  // Set origin at top left.

    if (mouse_x >= openGLLayer->editor_offset_x && mouse_y >= openGLLayer->editor_offset_y) {
        mouse_x -= openGLLayer->editor_offset_x;
        mouse_y -= openGLLayer->editor_offset_y;

        openGLLayer->cursor_start_x = mouse_x + openGLLayer->scroll_x;
        openGLLayer->cursor_start_y = mouse_y + openGLLayer->scroll_y;
        openGLLayer->cursor_end_x = mouse_x + openGLLayer->scroll_x;
        openGLLayer->cursor_end_y = mouse_y + openGLLayer->scroll_y;
        [openGLLayer setRendererCursorPositions];
    } else if (mouse_x < openGLLayer->editor_offset_x) {
        isDragging = true;
    }
}

- (void)mouseUp:(NSEvent*)event {
    isDragging = false;
}

- (void)mouseDragged:(NSEvent*)event {
    CGFloat mouse_x = event.locationInWindow.x;
    CGFloat mouse_y = event.locationInWindow.y;
    mouse_y = openGLLayer.frame.size.height - mouse_y;  // Set origin at top left.

    if (isDragging) {
        openGLLayer->editor_offset_x += event.deltaX;
        [openGLLayer setNeedsDisplay];
    } else if (mouse_x >= openGLLayer->editor_offset_x &&
               mouse_y >= openGLLayer->editor_offset_y) {
        mouse_x -= openGLLayer->editor_offset_x;
        mouse_y -= openGLLayer->editor_offset_y;

        openGLLayer->cursor_end_x = mouse_x + openGLLayer->scroll_x;
        openGLLayer->cursor_end_y = mouse_y + openGLLayer->scroll_y;
        [openGLLayer setRendererCursorPositions];
    }
}

- (void)rightMouseDown:(NSEvent*)event {
    NSMenu* contextMenu = [[NSMenu alloc] initWithTitle:@"Contextual Menu"];
    [contextMenu addItemWithTitle:@"Insert test string"
                           action:@selector(insertTestString)
                    keyEquivalent:@""];
    [contextMenu popUpMenuPositioningItem:nil atLocation:event.locationInWindow inView:self];
}

- (void)insertTestString {
    [openGLLayer insertUTF8String:"∆" bytes:3];
    [self.layer setNeedsDisplay];
}

// TODO: Implement light/dark mode detection.
- (void)viewDidChangeEffectiveAppearance {
    std::cerr << "viewDidChangeEffectiveAppearance\n";
}

@end

@implementation OpenGLLayer

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
    CGLPixelFormatAttribute attribs[] = {
        kCGLPFADisplayMask,
        static_cast<CGLPixelFormatAttribute>(mask),
        kCGLPFAColorSize,
        static_cast<CGLPixelFormatAttribute>(24),
        kCGLPFAAlphaSize,
        static_cast<CGLPixelFormatAttribute>(8),
        kCGLPFAAccelerated,
        kCGLPFANoRecovery,
        kCGLPFADoubleBuffer,
        kCGLPFAAllowOfflineRenderers,
        kCGLPFAOpenGLProfile,
        static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_3_2_Core),
        static_cast<CGLPixelFormatAttribute>(0),
    };

    CGLPixelFormatObj pixelFormat = nullptr;
    GLint numFormats = 0;
    CGLChoosePixelFormat(attribs, &pixelFormat, &numFormats);
    return pixelFormat;
}

static const char* read(void* payload, uint32_t byte_index, TSPoint position,
                        uint32_t* bytes_read) {
    Buffer* buffer = (Buffer*)payload;
    if (position.row >= buffer->lineCount()) {
        *bytes_read = 0;
        return "";
    }

    const size_t BUFSIZE = 256;
    static char buf[BUFSIZE];

    std::string line_str;
    buffer->getLineContent(&line_str, position.row);

    size_t len = line_str.size();
    size_t bytes_copied = std::min(len - position.column, BUFSIZE);

    memcpy(buf, &line_str[0] + position.column, bytes_copied);
    *bytes_read = (uint32_t)bytes_copied;
    if (bytes_copied < BUFSIZE) {
        // Add the final \n.
        // If it didn't fit, read() will be called again on the same line with the column advanced.
        buf[bytes_copied] = '\n';
        (*bytes_read)++;
    }
    return buf;
}

- (void)parseBuffer {
    TSInput input = {&buffer, read, TSInputEncodingUTF8};
    highlighter.parse(input);
    [self setNeedsDisplay];
}

- (void)editBuffer:(size_t)bytes {
    size_t start_byte =
        buffer.byteOfLine(text_renderer.cursor_end_line) + text_renderer.cursor_end_col_offset;
    size_t old_end_byte =
        buffer.byteOfLine(text_renderer.cursor_end_line) + text_renderer.cursor_end_col_offset;
    size_t new_end_byte = buffer.byteOfLine(text_renderer.cursor_end_line) +
                          text_renderer.cursor_end_col_offset + bytes;
    highlighter.edit(start_byte, old_end_byte, new_end_byte);
}

- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat {
    CGLContextObj glContext = nullptr;
    CGLCreateContext(pixelFormat, nullptr, &glContext);
    if (glContext || (glContext = [super copyCGLContextForPixelFormat:pixelFormat])) {
        CGLSetCurrentContext(glContext);

        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);

        CGFloat fontSize = 16 * self.contentsScale;
        float scaled_width = self.frame.size.width * self.contentsScale;
        float scaled_height = self.frame.size.height * self.contentsScale;
        text_renderer.setup(scaled_width, scaled_height, "Source Code Pro", fontSize);
        rect_renderer.setup(scaled_width, scaled_height);
        highlighter.setLanguage("source.scheme");

        buffer.setContents(ReadFile(ResourcePath() / "sample_files/sort.scm"));

        std::thread parse_thread([&] {
            {
                PROFILE_BLOCK("Tree-sitter only parse");
                [self parseBuffer];
            }
        });
        parse_thread.detach();

        [self addObserver:self forKeyPath:@"bounds" options:0 context:nil];

        editor_offset_x = 200;
        editor_offset_y = 30;
    }
    return glContext;
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

    {
        PROFILE_BLOCK("draw");
        // [NSThread sleepForTimeInterval:0.02];  // Simulate lag.

        float scaled_scroll_x = scroll_x * self.contentsScale;
        float scaled_scroll_y = scroll_y * self.contentsScale;
        float width = self.frame.size.width * self.contentsScale;
        float height = self.frame.size.height * self.contentsScale;
        float scaled_editor_offset_x = editor_offset_x * self.contentsScale;
        float scaled_editor_offset_y = editor_offset_y * self.contentsScale;

        glClearColor(253 / 255.0, 253 / 255.0, 253 / 255.0, 1.0);
        // glClearColor(0.988f, 0.992f, 0.992f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR);
        text_renderer.resize(width, height);
        text_renderer.renderText(scaled_scroll_x, scaled_scroll_y, buffer, highlighter,
                                 scaled_editor_offset_x, scaled_editor_offset_y);

        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
        rect_renderer.resize(width, height);
        rect_renderer.draw(scaled_scroll_x, scaled_scroll_y, text_renderer.cursor_end_x,
                           text_renderer.cursor_end_line, text_renderer.line_height,
                           buffer.lineCount(), text_renderer.longest_line_x,
                           scaled_editor_offset_x, scaled_editor_offset_y);

        // Calls glFlush() by default.
        [super drawInCGLContext:glContext
                    pixelFormat:pixelFormat
                   forLayerTime:timeInterval
                    displayTime:timeStamp];
    }
}

- (void)insertUTF8String:(const char*)str bytes:(size_t)bytes {
    {
        PROFILE_BLOCK("buffer.insert()");
        buffer.insert(text_renderer.cursor_end_line, text_renderer.cursor_end_col_offset, str);
    }

    {
        PROFILE_BLOCK("editBuffer + parseBuffer");
        // [self editBuffer:bytes];
        // [self parseBuffer];

        // if (strcmp(str, "\n") == 0) {
        //     text_renderer.cursor_start_line++;
        //     text_renderer.cursor_end_line++;

        //     text_renderer.cursor_start_col_offset = 0;
        //     text_renderer.cursor_start_x = 0;
        //     text_renderer.cursor_end_col_offset = 0;
        //     text_renderer.cursor_end_x = 0;
        // } else {
        //     float advance = text_renderer.getGlyphAdvance(std::string(str));
        //     text_renderer.cursor_start_col_offset += bytes;
        //     text_renderer.cursor_start_x += advance;
        //     text_renderer.cursor_end_col_offset += bytes;
        //     text_renderer.cursor_end_x += advance;
        // }
    }
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context {
    scroll_x = std::clamp(scroll_x, 0.0, [self maxScrollX]);
    scroll_y = std::clamp(scroll_y, 0.0, [self maxScrollY]);
    [self setNeedsDisplay];
}

- (void)setRendererCursorPositions {
    CGFloat scale = self.contentsScale;
    text_renderer.setCursorPositions(buffer, cursor_start_x * scale, cursor_start_y * scale,
                                     cursor_end_x * scale, cursor_end_y * scale);
    [self setNeedsDisplay];
}

- (CGFloat)maxScrollX {
    // TODO: Formulate max_cursor_x without the need for division.
    CGFloat max_cursor_x = text_renderer.longest_line_x / self.contentsScale;
    max_cursor_x -= self.frame.size.width - self->editor_offset_x;
    if (max_cursor_x < 0) max_cursor_x = 0;
    return max_cursor_x;
}

- (CGFloat)maxScrollY {
    size_t line_count = buffer.lineCount();
    // line_count -= 1;  // TODO: Merge this with RectRenderer.
    CGFloat max_y = line_count * text_renderer.line_height;
    // TODO: Formulate max_y without the need for division.
    max_y /= self.contentsScale;
    return max_y;
}

- (void)releaseCGLContext:(CGLContextObj)glContext {
    [super releaseCGLContext:glContext];
}

- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pixelFormat {
    [super releaseCGLPixelFormat:pixelFormat];
}

@end
