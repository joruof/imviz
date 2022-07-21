import sys
import imviz as viz
import numpy as np


class Vertex:

    def __init__(self):

        self.idx = 0
        self.pos = np.array([0.0, 0.0])
        self.uv = np.array([0.0, 0.0])

    def __eq__(self, o):

        return ((self.pos == o.pos).all()
                and (self.uv == o.uv).all())

    def __hash__(self):

        return hash((*self.pos, *self.uv))


class Polygon:

    def __init__(self):

        self.color = ""
        self.alpha = 0.0
        self.vertices = []

        self.text = ""
        self.font_size = 0.0
        self.advance = 0.0
        self.last_char_x = 0.0
        self.last_char_y = 0.0
        self.vertical_text = False

    def __str__(self):

        if len(self.text) == 0:

            svg_txt = '<polygon points="'

            for i, v in enumerate(self.vertices):
                svg_txt += f'{v.pos[0]:.3f},{v.pos[1]:.3f}'
                if i != len(self.vertices) - 1:
                    svg_txt += " "

            svg_txt += f'" fill="{self.color}" fill-opacity="{self.alpha}" />'

        else:
            np_vtx = np.array([v.pos for v in self.vertices])
            box_min = np_vtx.min(axis=0)
            box_max = np_vtx.max(axis=0)
            box_dims = box_max - box_min

            char_size = self.font_size * 0.8

            if self.vertical_text:
                char_x = box_min[0] + self.font_size * 0.55
                char_y = box_min[1] + box_dims[1]
                svg_txt = '<text '
                svg_txt += f'transform="translate({char_x:.3f}, {char_y:.3f}) rotate(-90)" '
            else:
                char_x = box_min[0]
                char_y = box_min[1] + self.font_size * 0.55
                svg_txt = f'<text x="{char_x:.3f}" y="{char_y:.3f}" '

            svg_txt += f'fill="{self.color}" fill-opacity="{self.alpha}" '
            svg_txt += 'style="font-family: Source Sans Pro; '
            svg_txt += f'font-size: {char_size}px" '
            svg_txt += '>'
            svg_txt += self.text
            svg_txt += '</text>'

        return svg_txt


class Demo:

    def __autogui__(s, **kwargs):

        if not viz.wait(powersave=True):
            sys.exit()

        viz.show_imgui_demo(True)
        viz.show_implot_demo(True)
        viz.style_colors_light()

        if viz.begin_window("Plot Example"):

            if viz.begin_plot("Plot"):

                viz.disable_aa()

                dl = viz.get_window_drawlist()

                viz.setup_axis(viz.Axis.X1, "x")
                viz.setup_axis(viz.Axis.Y1, "y-axis")

                if viz.plot_selection_ended():
                    viz.hard_cancel_plot_selection()

                viz.plot(
                    np.random.rand(100) * 3,
                    fmt="-o",
                    label="POINTS AND stuff",
                    line_weight=2
                )

                viz.plot(
                    [1.0, 100.0],
                    [1.0, 1.0],
                    fmt="-",
                    label="Other stuff",
                    line_weight=2,
                    shade=[1.0, 1.0]
                )

                viz.end_plot()

            cmds = dl.get_cmds()
            idxs = dl.get_indices()
            verts = dl.get_verts()

            if viz.button("save"):

                poly_groups = []

                for cmd in cmds:

                    polygons = []
                    merged_polys = []

                    # convert into polygons

                    start = cmd.idx_offset
                    end = cmd.idx_offset + cmd.elem_count

                    for i0, i1, i2 in zip(idxs[start:end:3],
                                          idxs[start+1:end:3],
                                          idxs[start+2:end:3]):

                        c0 = verts[i0].col
                        c1 = verts[i1].col
                        c2 = verts[i2].col

                        alpha0 = ((c0 & 0xff000000) >> 24) / 255
                        alpha1 = ((c1 & 0xff000000) >> 24) / 255
                        alpha2 = ((c2 & 0xff000000) >> 24) / 255

                        alpha = round(max([alpha0, alpha1, alpha2]), 3)

                        blue = (c0 & 0x00ff0000) >> 16
                        green = (c0 & 0x0000ff00) >> 8
                        red = c0 & 0x000000ff

                        v0 = Vertex()
                        v0.idx = i0
                        v0.pos = verts[i0].pos
                        v0.uv = verts[i0].uv

                        v1 = Vertex()
                        v1.idx = i1
                        v1.pos = verts[i1].pos
                        v1.uv = verts[i1].uv

                        v2 = Vertex()
                        v2.idx = i2
                        v2.pos = verts[i2].pos
                        v2.uv = verts[i2].uv

                        p = Polygon()
                        p.vertices = [v0, v1, v2]
                        p.color = f"#{red:02x}{green:02x}{blue:02x}"
                        p.alpha = alpha

                        polygons.append(p)

                    # merge polygons

                    if len(polygons) > 0:

                        p = polygons[0]

                        for o in polygons[1:]:

                            if p.color != o.color:
                                merged_polys.append(p)
                                p = o
                                continue

                            intersect = list(set(o.vertices) & set(p.vertices))

                            if len(intersect) != 2:
                                merged_polys.append(p)
                                p = o
                                continue

                            # detected matching side, joining

                            p_i0 = p.vertices.index(intersect[0])
                            p_i1 = p.vertices.index(intersect[1])
                            if p_i1 < p_i0:
                                p_i1, p_i0 = p_i0, p_i1

                            excluded_vtxs = list(set(o.vertices) - set(intersect))[0]

                            if p_i1 - p_i0 == 1:
                                p.vertices.insert(p_i0+1, excluded_vtxs)
                            else:
                                p.vertices.append(excluded_vtxs)

                        merged_polys.append(p)

                    poly_groups.append((cmd, merged_polys))

                # build a character lookup table based on
                # uv coordinates of the font atlas texture

                fonts = viz.get_font_atlas().get_fonts()
                uv_to_char = {}
                for font in fonts:
                    for g in font.get_glyphs():
                        k = str(g.u0) + str(g.v0)
                        c = bytearray([g.codepoint, 0]).decode("utf16")

                        v = viz.bundle()
                        v.text = c
                        v.font_size = font.font_size
                        v.advance = g.advance_x

                        uv_to_char[k] = v

                # iterate all groups to identify characters

                txt_tex_id = viz.get_font_atlas().get_texture_id()

                for i, (cmd, polys) in enumerate(poly_groups):

                    if txt_tex_id != cmd.texture_id:
                        continue

                    new_polys = []

                    for p in polys:

                        if len(p.vertices) != 4:
                            new_polys.append(p)
                            continue

                        if (p.vertices[0].uv == p.vertices[1].uv).all():
                            new_polys.append(p)
                            continue

                        # found something with a font texture

                        uv0 = np.min(np.array(
                            [v.uv for v in p.vertices]), axis=0)

                        k = str(float(uv0[0])) + str(float(uv0[1]))

                        try:
                            char_info = uv_to_char[k]
                        except KeyError:
                            new_polys.append(p)
                            continue

                        # seems to be a character

                        p.text = char_info.text
                        p.font_size = char_info.font_size
                        p.advance = char_info.advance
                        p.last_char_x, p.last_char_y = np.array(
                                [v.pos for v in p.vertices]).min(axis=0)

                        # check if we have vertical text

                        min_vtx = min(p.vertices,
                                      key=lambda v: v.pos[0]**2 + v.pos[1]**2)
                        min_uv = min(p.vertices,
                                     key=lambda v: v.uv[0]**2 + v.uv[1]**2)

                        p.vertical_text = min_vtx.idx != min_uv.idx

                        # correct y position in case of vertical text

                        if p.vertical_text:
                            _, p.last_char_y = np.array(
                                    [v.pos for v in p.vertices]).max(axis=0)

                        # check if we can merge it with a previous char

                        if len(new_polys) == 0:
                            new_polys.append(p)
                            continue

                        pp = new_polys[-1]

                        if len(pp.text) == 0:
                            new_polys.append(p)
                            continue

                        if p.vertical_text:
                            adv_ratio = (pp.last_char_y - p.last_char_y) / pp.advance
                            baseline_ok = abs(p.last_char_x - pp.last_char_x) < p.font_size / 2
                        else:
                            adv_ratio = (p.last_char_x - pp.last_char_x) / pp.advance
                            baseline_ok = abs(p.last_char_y - pp.last_char_y) < p.font_size / 2

                        can_be_joined = (adv_ratio > 0.5 and adv_ratio < 1.5
                                         and baseline_ok
                                         and (pp.font_size == p.font_size)
                                         and (pp.color == p.color)
                                         and (pp.alpha == p.alpha)
                                         and (pp.vertical_text == p.vertical_text))

                        if not can_be_joined:
                            new_polys.append(p)
                            continue

                        # join and continue

                        if adv_ratio > 1.2:
                            p.text = " " + p.text

                        pp.text += p.text
                        pp.vertices += p.vertices
                        pp.advance = p.advance
                        pp.last_char_x = p.last_char_x
                        pp.last_char_y = p.last_char_y

                    poly_groups[i] = (cmd, new_polys)

                # output svg text

                svg_txt = '<svg version="1.1" \
                           xmlns="http://www.w3.org/2000/svg" \
                           xmlns:xlink="http://www.w3.org/1999/xlink">\n'

                # write clip rects

                svg_txt += "<defs>\n"

                for i, (cmd, _) in enumerate(poly_groups):
                    cx = cmd.clip_rect[0]
                    cy = cmd.clip_rect[1]
                    cw = cmd.clip_rect[2] - cx
                    ch = cmd.clip_rect[3] - cy
                    svg_txt += f'<clipPath id="clip_rect_{i}">'
                    svg_txt += f'<rect x="{cx}" \
                                       y="{cy}" \
                                       width="{cw}" \
                                       height="{ch}" />'
                    svg_txt += '</clipPath>\n'

                svg_txt += "</defs>\n"

                # write out polygon groups

                for i, (cmd, polys) in enumerate(poly_groups):
                    svg_txt += f'<g clip-path="url(#clip_rect_{i})">\n'
                    for p in polys:
                        svg_txt += str(p) + "\n"
                    svg_txt += '</g>\n'

                # close and write to file

                svg_txt += '</svg>\n'

                with open("test.svg", "w+") as fd:
                    fd.write(svg_txt)

        viz.end_window()


def main():
    viz.dev.launch(Demo, "__autogui__")


if __name__ == "__main__":
    main()
