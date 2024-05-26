#pragma once

#include "font/directwrite/font_fallback_source.h"
#include "font/directwrite/text_analysis.h"
#include <d2d1.h>
#include <dwrite_3.h>
#include <vector>
#include <wrl/client.h>

#include <format>
#include <iostream>

namespace font {
inline void DrawGlyphRunHelper(ID2D1RenderTarget* target, IDWriteFactory4* factory,
                               IDWriteFontFace* fontFace, DWRITE_GLYPH_RUN* glyphRun,
                               UINT origin_y) {
    HRESULT hr = DWRITE_E_NOCOLOR;

    IDWriteColorGlyphRunEnumerator1* colorLayer;

    IDWriteFontFace2* fontFace2;
    fontFace->QueryInterface(reinterpret_cast<IDWriteFontFace2**>(&fontFace2));
    if (fontFace2->IsColorFont()) {
        DWRITE_GLYPH_IMAGE_FORMATS image_formats = DWRITE_GLYPH_IMAGE_FORMATS_COLR;
        hr = factory->TranslateColorGlyphRun({}, glyphRun, nullptr, image_formats,
                                             DWRITE_MEASURING_MODE_NATURAL, nullptr, 0,
                                             &colorLayer);
    }

    // TODO: Find a way to reuse render target and brushes.
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> black_brush = nullptr;
    target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black, 1.0f), &black_brush);
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> blue_brush = nullptr;
    target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Blue, 1.0f), &blue_brush);

    D2D1_POINT_2F baseline_origin{
        .x = 0,
        .y = static_cast<FLOAT>(origin_y),
    };

    target->BeginDraw();
    if (hr == DWRITE_E_NOCOLOR) {
        target->DrawGlyphRun(baseline_origin, glyphRun, black_brush.Get());
    } else {
        BOOL hasRun;
        const DWRITE_COLOR_GLYPH_RUN1* colorRun;

        while (true) {
            if (FAILED(colorLayer->MoveNext(&hasRun)) || !hasRun) {
                break;
            }
            if (FAILED(colorLayer->GetCurrentRun(&colorRun))) {
                break;
            }

            switch (colorRun->glyphImageFormat) {
            case DWRITE_GLYPH_IMAGE_FORMATS_PNG:
            case DWRITE_GLYPH_IMAGE_FORMATS_JPEG:
            case DWRITE_GLYPH_IMAGE_FORMATS_TIFF:
            case DWRITE_GLYPH_IMAGE_FORMATS_PREMULTIPLIED_B8G8R8A8:
                // std::cerr << "DrawColorBitmapGlyphRun()\n";
                break;

            case DWRITE_GLYPH_IMAGE_FORMATS_SVG:
                // std::cerr << "DrawSvgGlyphRun()\n";
                break;

            case DWRITE_GLYPH_IMAGE_FORMATS_TRUETYPE:
            case DWRITE_GLYPH_IMAGE_FORMATS_CFF:
            case DWRITE_GLYPH_IMAGE_FORMATS_COLR:
            default: {
                // std::cerr << "DrawGlyphRun()\n";

                Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> layer_brush;
                if (colorRun->paletteIndex == 0xFFFF) {
                    layer_brush = blue_brush;
                } else {
                    target->CreateSolidColorBrush(colorRun->runColor, &layer_brush);
                }

                target->DrawGlyphRun(baseline_origin, &colorRun->glyphRun, layer_brush.Get());
                break;
            }
            }
        }
    }
    target->EndDraw();
}

inline void PrintFontFamilyName(IDWriteFont* font) {
    Microsoft::WRL::ComPtr<IDWriteFontFamily> font_family;
    font->GetFontFamily(&font_family);

    Microsoft::WRL::ComPtr<IDWriteLocalizedStrings> family_names;
    font_family->GetFamilyNames(&family_names);

    wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
    int defaultLocaleSuccess = GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH);

    UINT32 index = 0;
    BOOL exists = false;
    family_names->FindLocaleName(localeName, &index, &exists);

    UINT32 length = 0;
    family_names->GetStringLength(index, &length);

    std::wstring name;
    name.resize(length + 1);
    family_names->GetString(index, &name[0], length + 1);
    std::wcerr << std::format(L"family name: {}\n", &name[0]);
}

inline std::wstring ConvertToUTF16(std::string_view utf8_str) {
    // https://stackoverflow.com/a/6693107/14698275
    size_t len = utf8_str.length();
    int required_len = MultiByteToWideChar(CP_UTF8, 0, &utf8_str[0], len, nullptr, 0);

    std::wstring wstr;
    wstr.resize(required_len);
    MultiByteToWideChar(CP_UTF8, 0, &utf8_str[0], len, &wstr[0], required_len);
    return wstr;
}

// https://github.com/linebender/skribo/blob/master/docs/script_matching.md#windows
inline void GetFallbackFont(IDWriteFactory4* factory, std::string_view utf8_str,
                            IDWriteFontFace** selected_font_face,
                            std::vector<UINT16>& glyph_indices) {
    std::wstring wstr = ConvertToUTF16(utf8_str);

    wchar_t locale[] = L"en-us";

    Microsoft::WRL::ComPtr<IDWriteNumberSubstitution> number_substitution;
    factory->CreateNumberSubstitution(DWRITE_NUMBER_SUBSTITUTION_METHOD_NONE, locale, true,
                                      &number_substitution);

    Microsoft::WRL::ComPtr<IDWriteTextAnalysisSource> text_analysis =
        new FontFallbackSource(&wstr[0], wstr.length(), locale, number_substitution.Get());

    Microsoft::WRL::ComPtr<IDWriteFontFallback> fallback;
    factory->GetSystemFontFallback(&fallback);

    UINT32 mapped_len;
    IDWriteFont* mapped_font;
    FLOAT mapped_scale;
    fallback->MapCharacters(text_analysis.Get(), 0, wstr.length(), nullptr, nullptr,
                            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                            DWRITE_FONT_STRETCH_NORMAL, &mapped_len, &mapped_font, &mapped_scale);

    if (mapped_font == nullptr) {
        // If no fallback font is found, don't do anything and leave the glyph index as 0.
        // Let the glyph be rendered as the "tofu" glyph.
        std::cerr
            << "IDWriteFontFallback::MapCharacters() warning: No font can render the text.\n";
        return;
    }

    // PrintFontFamilyName(mapped_font);

    IDWriteFontFace* fallback_font_face;
    mapped_font->CreateFontFace(&fallback_font_face);
    *selected_font_face = fallback_font_face;

    IDWriteTextAnalyzer* text_analyzer;
    factory->CreateTextAnalyzer(&text_analyzer);

    TextAnalysis analysis(&wstr[0], wstr.length(), nullptr,
                          DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
    TextAnalysis::Run* run_head;
    analysis.GenerateResults(text_analyzer, &run_head);

    uint32_t max_glyph_count = 3 * wstr.length() / 2 + 16;

    std::vector<uint16_t> cluster_map(wstr.length());
    std::vector<DWRITE_SHAPING_TEXT_PROPERTIES> text_properties(wstr.length());
    std::vector<uint16_t> out_glyph_indices(max_glyph_count);
    std::vector<DWRITE_SHAPING_GLYPH_PROPERTIES> glyph_properties(max_glyph_count);
    uint32_t glyph_count;

    // https://github.com/harfbuzz/harfbuzz/blob/2fcace77b2137abb44468a04e87d8716294641a9/src/hb-directwrite.cc#L661
    text_analyzer->GetGlyphs(&wstr[0], wstr.length(), *selected_font_face, false, false,
                             &run_head->mScript, locale, nullptr, nullptr, nullptr, 0,
                             max_glyph_count, &cluster_map[0], &text_properties[0],
                             &out_glyph_indices[0], &glyph_properties[0], &glyph_count);

    glyph_indices[0] = out_glyph_indices[0];
}
}
