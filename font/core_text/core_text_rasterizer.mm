#include "base/apple/scoped_cftyperef.h"
#include "base/apple/scoped_cgtyperef.h"
#include "font/font_rasterizer.h"
#include "unicode/SkTFitsIn.h"
#include "unicode/unicode.h"

#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>

// TODO: Debug use; remove this.
#include <iostream>

using base::apple::ScopedCFTypeRef;
using base::apple::ScopedTypeRef;

namespace {

// https://skia.googlesource.com/skia/+/0a7c7b0b96fc897040e71ea3304d9d6a042cda8b/modules/skshaper/src/SkShaper_coretext.cpp#115
class UTF16ToUTF8IndicesMap {
public:
    /** Builds a UTF-16 to UTF-8 indices map; the text is not retained
     * @return true if successful
     */
    bool setUTF8(const char* utf8, size_t size) {
        assert(utf8 != nullptr);

        if (!SkTFitsIn<int32_t>(size)) {
            std::cerr << "UTF16ToUTF8IndicesMap: text too long\n";
            return false;
        }

        auto utf16Size = unicode::UTF8ToUTF16(nullptr, 0, utf8, size);
        if (utf16Size < 0) {
            std::cerr << "UTF16ToUTF8IndicesMap: Invalid utf8 input\n";
            return false;
        }

        // utf16Size+1 to also store the size
        fUtf16ToUtf8Indices = std::vector<size_t>(utf16Size + 1);
        auto utf16 = fUtf16ToUtf8Indices.begin();
        auto utf8Begin = utf8, utf8End = utf8 + size;
        while (utf8Begin < utf8End) {
            *utf16 = utf8Begin - utf8;
            utf16 += unicode::ToUTF16(unicode::NextUTF8(&utf8Begin, utf8End), nullptr);
        }
        *utf16 = size;

        return true;
    }

    size_t mapIndex(size_t index) const {
        assert(index < fUtf16ToUtf8Indices.size());
        return fUtf16ToUtf8Indices[index];
    }

    std::pair<size_t, size_t> mapRange(size_t start, size_t size) const {
        auto utf8Start = mapIndex(start);
        return {utf8Start, mapIndex(start + size) - utf8Start};
    }

private:
    std::vector<size_t> fUtf16ToUtf8Indices;
};

// https://gist.github.com/peter-bloomfield/1b228e2bb654702b1e50ef7524121fb9
inline std::string CFStringToStdString(CFStringRef cf_str) {
    if (!cf_str) return {};

    // Attempt to access the underlying buffer directly. This only works if no conversion or
    // internal allocation is required.
    auto originalBuffer{CFStringGetCStringPtr(cf_str, kCFStringEncodingUTF8)};
    if (originalBuffer) {
        return originalBuffer;
    }

    // Copy the data out to a local buffer.
    auto lengthInUtf16{CFStringGetLength(cf_str)};
    // Leave room for null terminator.
    auto maxLengthInUtf8{CFStringGetMaximumSizeForEncoding(lengthInUtf16, kCFStringEncodingUTF8) +
                         1};
    std::vector<char> buffer(maxLengthInUtf8);

    if (CFStringGetCString(cf_str, buffer.data(), maxLengthInUtf8, maxLengthInUtf8)) {
        return buffer.data();
    }
    return {};
}

}

namespace font {

class FontRasterizer::impl {
public:
    ScopedCFTypeRef<CTFontRef> ct_font;
    std::unordered_map<std::string, size_t> font_postscript_name_to_id;
    std::vector<ScopedCFTypeRef<CTFontRef>> font_id_to_native;

    ScopedCFTypeRef<CTLineRef> createCTLine(std::string_view str8);
};

FontRasterizer::FontRasterizer(const std::string& font_name_utf8, int font_size)
    : pimpl{new impl{}} {
    ScopedCFTypeRef<CFStringRef> ct_font_name{
        CFStringCreateWithCString(nullptr, font_name_utf8.c_str(), kCFStringEncodingUTF8)};
    pimpl->ct_font.reset(CTFontCreateWithName(ct_font_name.get(), font_size, nullptr));

    int ascent = std::ceil(CTFontGetAscent(pimpl->ct_font.get()));
    int descent = std::ceil(CTFontGetDescent(pimpl->ct_font.get()));
    int leading = std::ceil(CTFontGetLeading(pimpl->ct_font.get()));
    int line_height = ascent + descent + leading;

    // TODO: Remove magic numbers that emulate Sublime Text.
    line_height += 1;

    this->line_height = line_height;
    this->descent = -descent;
}

FontRasterizer::~FontRasterizer() {}

FontRasterizer::RasterizedGlyph FontRasterizer::rasterizeUTF8(size_t font_id,
                                                              uint32_t glyph_id) const {
    CTFontRef font_ref = pimpl->font_id_to_native[font_id].get();
    CGGlyph glyph_index = glyph_id;

    if (!font_ref) {
        std::cerr << "FontRasterizer::rasterizeUTF8() error: CTFontRef is null!\n";
        std::abort();
    }

    CGRect bounds;
    CTFontGetBoundingRectsForGlyphs(font_ref, kCTFontOrientationDefault, &glyph_index, &bounds, 1);

    int32_t rasterized_left = std::floor(bounds.origin.x);
    uint32_t rasterized_width = std::ceil(bounds.origin.x - rasterized_left + bounds.size.width);
    int32_t rasterized_descent = std::ceil(-bounds.origin.y);
    int32_t rasterized_ascent = std::ceil(bounds.size.height + bounds.origin.y);
    uint32_t rasterized_height = rasterized_descent + rasterized_ascent;

    int32_t top = std::ceil(bounds.size.height + bounds.origin.y);
    top -= descent;

    // If the font is a color font and the glyph doesn't have an outline, it is
    // a color glyph.
    // https://github.com/sublimehq/sublime_text/issues/3747#issuecomment-726837744
    bool colored_font = CTFontGetSymbolicTraits(font_ref) & kCTFontTraitColorGlyphs;
    bool has_outline = CTFontCreatePathForGlyph(font_ref, glyph_index, nullptr);
    bool colored = colored_font && !has_outline;

    ScopedTypeRef<CGColorSpaceRef> color_space_ref{CGColorSpaceCreateDeviceRGB()};
    ScopedTypeRef<CGContextRef> context{CGBitmapContextCreate(
        nullptr, rasterized_width, rasterized_height, 8, rasterized_width * 4,
        color_space_ref.get(), kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host)};

    CGFloat alpha = colored ? 0.0 : 1.0;
    CGContextSetRGBFillColor(context.get(), 0.0, 0.0, 0.0, alpha);

    CGContextFillRect(context.get(), CGRectMake(0.0, 0.0, rasterized_width, rasterized_height));
    CGContextSetAllowsFontSmoothing(context.get(), true);
    CGContextSetShouldSmoothFonts(context.get(), false);
    CGContextSetAllowsFontSubpixelQuantization(context.get(), true);
    CGContextSetShouldSubpixelQuantizeFonts(context.get(), true);
    CGContextSetAllowsFontSubpixelPositioning(context.get(), true);
    CGContextSetShouldSubpixelPositionFonts(context.get(), true);
    CGContextSetAllowsAntialiasing(context.get(), true);
    CGContextSetShouldAntialias(context.get(), true);

    CGContextSetRGBFillColor(context.get(), 1.0, 1.0, 1.0, 1.0);
    CGPoint rasterization_origin = CGPointMake(-rasterized_left, rasterized_descent);

    CTFontDrawGlyphs(font_ref, &glyph_index, &rasterization_origin, 1, context.get());

    uint8_t* bitmap_data = (uint8_t*)CGBitmapContextGetData(context.get());
    size_t height = CGBitmapContextGetHeight(context.get());
    size_t bytes_per_row = CGBitmapContextGetBytesPerRow(context.get());
    size_t len = height * bytes_per_row;

    size_t pixels = len / 4;
    std::vector<uint8_t> buffer;
    size_t size = colored ? pixels * 4 : pixels * 3;

    // TODO: This assumes little endian; detect and support big endian.
    buffer.reserve(size);
    for (size_t i = 0; i < pixels; ++i) {
        size_t offset = i * 4;
        buffer.emplace_back(bitmap_data[offset + 2]);
        buffer.emplace_back(bitmap_data[offset + 1]);
        buffer.emplace_back(bitmap_data[offset]);
        if (colored) {
            buffer.emplace_back(bitmap_data[offset + 3]);
        }
    }

    double advance =
        CTFontGetAdvancesForGlyphs(font_ref, kCTFontOrientationDefault, &glyph_index, nullptr, 1);

    return {
        .colored = colored,
        .left = rasterized_left,
        .top = top,
        .width = static_cast<int32_t>(rasterized_width),
        .height = static_cast<int32_t>(rasterized_height),
        .advance = static_cast<int32_t>(std::ceil(advance)),
        .buffer = buffer,
    };
}

// https://skia.googlesource.com/skia/+/0a7c7b0b96fc897040e71ea3304d9d6a042cda8b/modules/skshaper/src/SkShaper_coretext.cpp#195
FontRasterizer::LineLayout FontRasterizer::layoutLine(std::string_view str8) const {
    UTF16ToUTF8IndicesMap utf8IndicesMap;
    if (!utf8IndicesMap.setUTF8(&str8[0], str8.length())) {
        std::cerr << "UTF16ToUTF8IndicesMap::setUTF8 error\n";
        std::abort();
    }

    ScopedCFTypeRef<CTLineRef> ct_line = pimpl->createCTLine(str8);

    std::vector<ShapedRun> runs;
    CFArrayRef run_array = CTLineGetGlyphRuns(ct_line.get());
    CFIndex run_count = CFArrayGetCount(run_array);
    for (CFIndex i = 0; i < run_count; ++i) {
        CTRunRef ct_run = (CTRunRef)CFArrayGetValueAtIndex(run_array, i);

        CTFontRef ct_font =
            (CTFontRef)CFDictionaryGetValue(CTRunGetAttributes(ct_run), kCTFontAttributeName);
        CFStringRef ct_font_name = CTFontCopyPostScriptName(ct_font);
        std::string font_name = CFStringToStdString(ct_font_name);

        // Cache font in font map.
        if (!pimpl->font_postscript_name_to_id.contains(font_name)) {
            // TODO: Figure out how to automatically retain using ScopedCFTypeRef.
            ct_font = (CTFontRef)CFRetain(ct_font);

            size_t font_id = pimpl->font_id_to_native.size();
            pimpl->font_id_to_native.push_back(std::move(ct_font));
            pimpl->font_postscript_name_to_id[font_name] = font_id;
        }
        size_t font_id = pimpl->font_postscript_name_to_id[font_name];

        CFIndex glyph_count = CTRunGetGlyphCount(ct_run);
        std::vector<CGGlyph> glyph_ids(glyph_count);
        std::vector<CFIndex> indices(glyph_count);
        std::vector<CGPoint> positions(glyph_count);
        std::vector<CGSize> advances(glyph_count);

        CTRunGetGlyphs(ct_run, {0, glyph_count}, glyph_ids.data());
        CTRunGetStringIndices(ct_run, {0, glyph_count}, indices.data());
        CTRunGetPositions(ct_run, {0, glyph_count}, positions.data());
        CTRunGetAdvances(ct_run, {0, glyph_count}, advances.data());

        std::vector<ShapedGlyph> glyphs;
        glyphs.reserve(glyph_count);
        for (size_t i = 0; i < glyph_count; ++i) {
            ShapedGlyph glyph{
                .glyph_id = glyph_ids[i],
                // TODO: Use subpixel variants instead of rounding.
                .position =
                    Point{
                        .x = static_cast<int>(std::ceil(positions[i].x)),
                        .y = static_cast<int>(std::ceil(positions[i].y)),
                    },
                .advance =
                    Point{
                        .x = static_cast<int>(std::ceil(advances[i].width)),
                        .y = static_cast<int>(std::ceil(advances[i].height)),
                    },
                .index = utf8IndicesMap.mapIndex(indices[i]),
            };
            glyphs.push_back(std::move(glyph));
        }

        runs.emplace_back(ShapedRun{font_id, std::move(glyphs)});
    }

    // TODO: Currently, width != sum of all advances since we round. When we implement subpixel
    // variants, this should no longer be an issue.
    double width = CTLineGetTypographicBounds(ct_line.get(), nullptr, nullptr, nullptr);
    return {
        .width = static_cast<int>(std::ceil(width)),
        .length = str8.length(),
        .runs = std::move(runs),
    };
}

ScopedCFTypeRef<CTLineRef> FontRasterizer::impl::createCTLine(std::string_view str8) {
    ScopedCFTypeRef<CFStringRef> text_string{
        CFStringCreateWithBytesNoCopy(kCFAllocatorDefault, (const uint8_t*)&str8[0], str8.length(),
                                      kCFStringEncodingUTF8, false, kCFAllocatorNull)};

    ScopedCFTypeRef<CFMutableDictionaryRef> attr{CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks)};
    CFDictionaryAddValue(attr.get(), kCTFontAttributeName, ct_font.get());

    ScopedCFTypeRef<CFAttributedStringRef> attr_string{
        CFAttributedStringCreate(kCFAllocatorDefault, text_string.get(), attr.get())};

    return CTLineCreateWithAttributedString(attr_string.get());
}

}
