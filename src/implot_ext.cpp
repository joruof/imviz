/*
This is an extended version of the implot plotting function.
The original source code was released under the following license:

MIT License

Copyright (c) 2020 Evan Pezent

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "implot_ext.hpp"

// this is stupid ... i like it so much
#include "implot_items.cpp"

#include <sstream>
#include <iostream>

namespace ImPlot {

IMPLOT_INLINE void PrimLineCol2(ImDrawList& draw_list, const ImVec2& P1, const ImVec2& P2, float half_weight, ImU32 col0, ImU32 col1, const ImVec2& tex_uv0, const ImVec2 tex_uv1) {
    float dx = P2.x - P1.x;
    float dy = P2.y - P1.y;
    IMPLOT_NORMALIZE2F_OVER_ZERO(dx, dy);
    dx *= half_weight;
    dy *= half_weight;
    draw_list._VtxWritePtr[0].pos.x = P1.x + dy;
    draw_list._VtxWritePtr[0].pos.y = P1.y - dx;
    draw_list._VtxWritePtr[0].uv    = tex_uv0;
    draw_list._VtxWritePtr[0].col   = col0;
    draw_list._VtxWritePtr[1].pos.x = P2.x + dy;
    draw_list._VtxWritePtr[1].pos.y = P2.y - dx;
    draw_list._VtxWritePtr[1].uv    = tex_uv0;
    draw_list._VtxWritePtr[1].col   = col1;
    draw_list._VtxWritePtr[2].pos.x = P2.x - dy;
    draw_list._VtxWritePtr[2].pos.y = P2.y + dx;
    draw_list._VtxWritePtr[2].uv    = tex_uv1;
    draw_list._VtxWritePtr[2].col   = col1;
    draw_list._VtxWritePtr[3].pos.x = P1.x - dy;
    draw_list._VtxWritePtr[3].pos.y = P1.y + dx;
    draw_list._VtxWritePtr[3].uv    = tex_uv1;
    draw_list._VtxWritePtr[3].col   = col0;
    draw_list._VtxWritePtr += 4;
    draw_list._IdxWritePtr[0] = (ImDrawIdx)(draw_list._VtxCurrentIdx);
    draw_list._IdxWritePtr[1] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 1);
    draw_list._IdxWritePtr[2] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 2);
    draw_list._IdxWritePtr[3] = (ImDrawIdx)(draw_list._VtxCurrentIdx);
    draw_list._IdxWritePtr[4] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 2);
    draw_list._IdxWritePtr[5] = (ImDrawIdx)(draw_list._VtxCurrentIdx + 3);
    draw_list._IdxWritePtr += 6;
    draw_list._VtxCurrentIdx += 4;
}

template <class _Getter>
struct CustomRendererLineStrip : RendererBase {
    CustomRendererLineStrip(const _Getter& getter, ImU32 col, float weight) :
        RendererBase(getter.Count - 1, 6, 4),
        Getter(getter),
        Col(col),
        HalfWeight(ImMax(1.0f,weight)*0.5f)
    {
        P1 = this->Transformer(Getter(0));
    }
    void Init(ImDrawList& draw_list) const {
        GetLineRenderProps(draw_list, HalfWeight, UV0, UV1);
    }
    IMPLOT_INLINE bool Render(ImDrawList& draw_list, const ImRect& cull_rect, int prim) const {
        ImVec2 P2 = this->Transformer(Getter(prim + 1));
        if (!cull_rect.Overlaps(ImRect(ImMin(P1, P2), ImMax(P1, P2)))) {
            P1 = P2;
            return false;
        }
        ImU32 col0 = ImGui::ColorConvertFloat4ToU32(colors[prim]);
        ImU32 col1 = ImGui::ColorConvertFloat4ToU32(colors[prim+1]);
        PrimLineCol2(draw_list,P1,P2,HalfWeight,col0,col1,UV0,UV1);
        P1 = P2;
        return true;
    }
    const _Getter& Getter;
    const ImU32 Col;
    mutable float HalfWeight;
    mutable ImVec2 P1;
    mutable ImVec2 UV0;
    mutable ImVec2 UV1;

    inline static ImVec4* colors;
};

template <class _Getter>
struct CustomRendererMarkersFill : RendererBase {
    CustomRendererMarkersFill(const _Getter& getter, const ImVec2* marker, int count, float size, ImU32 col) :
        RendererBase(getter.Count, (count-2)*3, count),
        Getter(getter),
        Marker(marker),
        Count(count),
        Size(size),
        Col(col)
    { }
    void Init(ImDrawList& draw_list) const {
        UV = draw_list._Data->TexUvWhitePixel;
    }
    IMPLOT_INLINE bool Render(ImDrawList& draw_list, const ImRect& cull_rect, int prim) const {
        ImVec2 p = this->Transformer(Getter(prim));
        ImU32 col0 = ImGui::ColorConvertFloat4ToU32(colors[prim]);
        if (p.x >= cull_rect.Min.x && p.y >= cull_rect.Min.y && p.x <= cull_rect.Max.x && p.y <= cull_rect.Max.y) {
            for (int i = 0; i < Count; i++) {
                draw_list._VtxWritePtr[0].pos.x = p.x + Marker[i].x * Size;
                draw_list._VtxWritePtr[0].pos.y = p.y + Marker[i].y * Size;
                draw_list._VtxWritePtr[0].uv = UV;
                draw_list._VtxWritePtr[0].col = col0;

                draw_list._VtxWritePtr++;
            }
            for (int i = 2; i < Count; i++) {
                draw_list._IdxWritePtr[0] = (ImDrawIdx)(draw_list._VtxCurrentIdx);
                draw_list._IdxWritePtr[1] = (ImDrawIdx)(draw_list._VtxCurrentIdx + i - 1);
                draw_list._IdxWritePtr[2] = (ImDrawIdx)(draw_list._VtxCurrentIdx + i);
                draw_list._IdxWritePtr += 3;
            }
            draw_list._VtxCurrentIdx += (ImDrawIdx)Count;
            return true;
        }
        return false;
    }
    const _Getter& Getter;
    const ImVec2* Marker;
    const int Count;
    const float Size;
    const ImU32 Col;
    mutable ImVec2 UV;

    inline static ImVec4* colors;
};

template <class _Getter>
struct CustomRendererMarkersLine : RendererBase {
    CustomRendererMarkersLine(const _Getter& getter, const ImVec2* marker, int count, float size, float weight, ImU32 col) :
        RendererBase(getter.Count, count/2*6, count/2*4),
        Getter(getter),
        Marker(marker),
        Count(count),
        HalfWeight(ImMax(1.0f,weight)*0.5f),
        Size(size),
        Col(col)
    { }
    void Init(ImDrawList& draw_list) const {
        GetLineRenderProps(draw_list, HalfWeight, UV0, UV1);
    }
    IMPLOT_INLINE bool Render(ImDrawList& draw_list, const ImRect& cull_rect, int prim) const {
        ImVec2 p = this->Transformer(Getter(prim));
        ImU32 col0 = ImGui::ColorConvertFloat4ToU32(colors[prim]);
        if (p.x >= cull_rect.Min.x && p.y >= cull_rect.Min.y && p.x <= cull_rect.Max.x && p.y <= cull_rect.Max.y) {
            for (int i = 0; i < Count; i = i + 2) {
                ImVec2 p1(p.x + Marker[i].x * Size, p.y + Marker[i].y * Size);
                ImVec2 p2(p.x + Marker[i+1].x * Size, p.y + Marker[i+1].y * Size);
                PrimLine(draw_list, p1, p2, HalfWeight, col0, UV0, UV1);
            }
            return true;
        }
        return false;
    }
    const _Getter& Getter;
    const ImVec2* Marker;
    const int Count;
    mutable float HalfWeight;
    const float Size;
    const ImU32 Col;
    mutable ImVec2 UV0;
    mutable ImVec2 UV1;

    inline static ImVec4* colors;
};

template <typename _Getter>
void CustomRenderMarkers(const _Getter& getter, ImPlotMarker marker, float size, bool rend_fill, ImU32 col_fill, bool rend_line, ImU32 col_line, float weight) {
    if (rend_fill) {
        switch (marker) {
            case ImPlotMarker_Circle  : RenderPrimitives1<CustomRendererMarkersFill>(getter,MARKER_FILL_CIRCLE,10,size,col_fill); break;
            case ImPlotMarker_Square  : RenderPrimitives1<CustomRendererMarkersFill>(getter,MARKER_FILL_SQUARE, 4,size,col_fill); break;
            case ImPlotMarker_Diamond : RenderPrimitives1<CustomRendererMarkersFill>(getter,MARKER_FILL_DIAMOND,4,size,col_fill); break;
            case ImPlotMarker_Up      : RenderPrimitives1<CustomRendererMarkersFill>(getter,MARKER_FILL_UP,     3,size,col_fill); break;
            case ImPlotMarker_Down    : RenderPrimitives1<CustomRendererMarkersFill>(getter,MARKER_FILL_DOWN,   3,size,col_fill); break;
            case ImPlotMarker_Left    : RenderPrimitives1<CustomRendererMarkersFill>(getter,MARKER_FILL_LEFT,   3,size,col_fill); break;
            case ImPlotMarker_Right   : RenderPrimitives1<CustomRendererMarkersFill>(getter,MARKER_FILL_RIGHT,  3,size,col_fill); break;
        }
    }
    if (rend_line) {
        switch (marker) {
            case ImPlotMarker_Circle    : RenderPrimitives1<CustomRendererMarkersLine>(getter,MARKER_LINE_CIRCLE, 20,size,weight,col_line); break;
            case ImPlotMarker_Square    : RenderPrimitives1<CustomRendererMarkersLine>(getter,MARKER_LINE_SQUARE,  8,size,weight,col_line); break;
            case ImPlotMarker_Diamond   : RenderPrimitives1<CustomRendererMarkersLine>(getter,MARKER_LINE_DIAMOND, 8,size,weight,col_line); break;
            case ImPlotMarker_Up        : RenderPrimitives1<CustomRendererMarkersLine>(getter,MARKER_LINE_UP,      6,size,weight,col_line); break;
            case ImPlotMarker_Down      : RenderPrimitives1<CustomRendererMarkersLine>(getter,MARKER_LINE_DOWN,    6,size,weight,col_line); break;
            case ImPlotMarker_Left      : RenderPrimitives1<CustomRendererMarkersLine>(getter,MARKER_LINE_LEFT,    6,size,weight,col_line); break;
            case ImPlotMarker_Right     : RenderPrimitives1<CustomRendererMarkersLine>(getter,MARKER_LINE_RIGHT,   6,size,weight,col_line); break;
            case ImPlotMarker_Asterisk  : RenderPrimitives1<CustomRendererMarkersLine>(getter,MARKER_LINE_ASTERISK,6,size,weight,col_line); break;
            case ImPlotMarker_Plus      : RenderPrimitives1<CustomRendererMarkersLine>(getter,MARKER_LINE_PLUS,    4,size,weight,col_line); break;
            case ImPlotMarker_Cross     : RenderPrimitives1<CustomRendererMarkersLine>(getter,MARKER_LINE_CROSS,   4,size,weight,col_line); break;
        }
    }
}

void customPlot(
        const char* label,
        PlotArrayInfo& pai,
        py::handle color,
        bool noLine,
        ImPlotFlags flags) {

    GetterXY<IndexerIdx<double>, IndexerIdx<double>> getter(
             IndexerIdx<double>(pai.xDataPtr, pai.count, 0, sizeof(double)),
             IndexerIdx<double>(pai.yDataPtr, pai.count, 0, sizeof(double)),
             pai.count);

    if (BeginItemEx(
                label,
                Fitter1(getter),
                flags,
                ImPlotCol_Line)) {

        array_like<float> colArr = array_like<float>::ensure(color);

        if ((size_t)colArr.shape(0) != pai.count) {
            std::stringstream ss;
            ss << "color array size ("
               << colArr.shape(0)
               << ") != number of points ("
               << pai.count
               << ")";
            throw py::value_error(ss.str());
        }

        CustomRendererLineStrip<decltype(getter)>::colors = (ImVec4*)colArr.mutable_data(0);
        CustomRendererMarkersFill<decltype(getter)>::colors = (ImVec4*)colArr.mutable_data(0);
        CustomRendererMarkersLine<decltype(getter)>::colors = (ImVec4*)colArr.mutable_data(0);

        const ImPlotNextItemData& s = ImPlot::GetItemData();

        if (pai.count > 1 && s.RenderLine && noLine == false) {
            const ImU32 col_line = ImGui::GetColorU32(s.Colors[ImPlotCol_Line]);
            RenderPrimitives1<CustomRendererLineStrip>(getter, col_line, s.LineWeight);
        }
        // render markers
        if (s.Marker != ImPlotMarker_None) {
            const ImU32 col_line = ImGui::GetColorU32(s.Colors[ImPlotCol_MarkerOutline]);
            const ImU32 col_fill = ImGui::GetColorU32(s.Colors[ImPlotCol_MarkerFill]);
            CustomRenderMarkers<decltype(getter)>(
                    getter,
                    s.Marker,
                    s.MarkerSize,
                    s.RenderMarkerFill, 
                    col_fill,
                    s.RenderMarkerLine,
                    col_line,
                    s.MarkerWeight);
        }
        
        ImPlot::EndItem();
    }
}

}
