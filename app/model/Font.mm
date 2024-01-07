#import "Font.h"
#import "util/CGFloatUtil.h"
#import "util/LogUtil.h"

Font::Font(CFStringRef name, CGFloat size) {
    fontRef = CTFontCreateWithName(name, size, nullptr);
}

Metrics Font::metrics() {
    CGGlyph glyph = get_glyph(@"0");
    double average_advance =
        CTFontGetAdvancesForGlyphs(fontRef, kCTFontOrientationDefault, &glyph, nullptr, 1);

    CGFloat ascent = CGFloat_round(CTFontGetAscent(fontRef));
    CGFloat descent = CGFloat_round(CTFontGetDescent(fontRef));
    CGFloat leading = CGFloat_round(CTFontGetLeading(fontRef));
    CGFloat line_height = ascent + descent + leading;

    return Metrics{average_advance, line_height};
}

CGGlyph Font::get_glyph(NSString* characterString) {
    unichar characters[1];
    [characterString getCharacters:characters range:NSMakeRange(0, 1)];
    CGGlyph glyphs[1];
    if (CTFontGetGlyphsForCharacters(fontRef, characters, glyphs, 1)) {
        logDefault(@"Font", @"got glyphs! %d", glyphs[0]);
    } else {
        logDefault(@"Font", @"could not get glyphs for characters");
    }
    return glyphs[0];
}
