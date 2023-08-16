"""
This contains functions to export plots in various formats.
"""


import io
import os
import sys
import time
import base64
import pickle
import subprocess
import numpy as np

from PIL import Image


pdf_avail = subprocess.call("which inkscape",
                            shell=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE) == 0


try:
    import cppimviz as viz
except ModuleNotFoundError:
    sys.path.append(os.path.join(os.path.dirname(
        os.path.abspath(__file__)), "../build"))
    import cppimviz as viz


from imviz.common import bundle


class Vertex:

    def __init__(self):

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

        # only used, if the polygon represents one or many characters

        self.text = ""
        self.font_size = 0.0
        self.advance = 0.0
        self.last_char_x = 0.0
        self.last_char_y = 0.0
        self.vertical_text = False
        self.start_x = 0.0
        self.start_y = 0.0
        self.x0 = 0.0
        self.y0 = 0.0
        self.x1 = 0.0
        self.y1 = 0.0

        # only use if we have an image in the polygon

        self.image = None


class Polyline:

    def __init__(self):

        self.points = []
        self.color = ""
        self.alpha = 0.0


class DrawListState:

    def __init__(self):

        self.draw_cmds = []
        self.polygon_groups = []
        self.line_groups = []

        self.canvas_pos = np.array([0.0, 0.0])
        self.canvas_size = np.array([0.0, 0.0])


def export_polygons(state, dl):

    idxs = dl.get_indices()
    verts = dl.get_verts()

    for cmd in state.draw_cmds:

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
            v0.pos = verts[i0].pos
            v0.uv = verts[i0].uv

            v1 = Vertex()
            v1.pos = verts[i1].pos
            v1.uv = verts[i1].uv

            v2 = Vertex()
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

                excluded_vtxs = list(set(o.vertices) - set(intersect))

                if len(excluded_vtxs) != 1:
                    merged_polys.append(p)
                    p = o
                    continue

                excluded_vtxs = excluded_vtxs[0]

                if p_i1 - p_i0 == 1:
                    p.vertices.insert(p_i0+1, excluded_vtxs)
                else:
                    p.vertices.append(excluded_vtxs)

            merged_polys.append(p)

        state.polygon_groups.append(merged_polys)


def merge_polygons_to_lines(state):

    def get_mid_points(p):

        axis_0 = (p.vertices[3].pos - p.vertices[0].pos) / 2.0
        axis_1 = (p.vertices[1].pos - p.vertices[0].pos) / 2.0
        center = p.vertices[0].pos + axis_0 + axis_1

        mp0 = center + axis_0 
        mp1 = center + axis_1
        mp2 = center - axis_0 
        mp3 = center - axis_1

        return [mp0, mp1, mp2, mp3]

    for pg in state.polygon_groups:

        prev_p = None

        line_group = []

        line_points = []
        remove_list = []
        counter_point = None

        for p in pg:

            if len(p.vertices) != 4:
                prev_p = None 
                continue

            if prev_p is None:
                prev_p = p
                continue

            p_mid_points = get_mid_points(p)
            pp_mid_points = get_mid_points(prev_p)

            if prev_p.color != p.color:
                if len(line_points) > 0:
                    line_points.append(counter_point)
                    pl = Polyline()
                    pl.color = remove_list[-1].color
                    pl.alpha = remove_list[-1].alpha
                    pl.points = line_points
                    line_group.append(pl)
                    line_points = []
                prev_p = None
                continue

            if prev_p.alpha != p.alpha:
                if len(line_points) > 0:
                    line_points.append(counter_point)
                    pl = Polyline()
                    pl.color = remove_list[-1].color
                    pl.alpha = remove_list[-1].alpha
                    pl.points = line_points
                    line_group.append(pl)
                    line_points = []
                prev_p = None
                continue

            matched = False

            for i, mp in enumerate(p_mid_points):
                for j, mpp in enumerate(pp_mid_points):
                    if np.linalg.norm(mp - mpp) < 1.0:
                        matched = True
                        break

            if matched:
                if len(line_points) == 0:
                    line_points.append(pp_mid_points[(j + 2) % 4])
                    remove_list.append(prev_p)
                line_points.append(mp)
                remove_list.append(p)
                counter_point = p_mid_points[(i + 2) % 4]
            elif len(line_points) > 0:
                line_points.append(counter_point)
                pl = Polyline()
                pl.color = remove_list[-1].color
                pl.alpha = remove_list[-1].alpha
                pl.points = line_points
                line_group.append(pl)
                line_points = []

            prev_p = p

        for p in remove_list:
            pg.remove(p)

        if len(line_points) > 0:

            line_points.append(p_mid_points[(i + 2) % 4])
            pl = Polyline()
            pl.color = remove_list[-1].color
            pl.alpha = remove_list[-1].alpha
            pl.points = line_points
            line_group.append(pl)
            line_points = []

        state.line_groups.append(line_group)


def export_text_polygons(state):

    # build a character lookup table based on
    # uv coordinates of the font atlas texture

    fonts = viz.get_font_atlas().get_fonts()
    uv_to_char = {}

    for font in fonts:
        for g in font.get_glyphs():
            k = str(g.u0) + str(g.v0)
            c = bytearray([g.codepoint, 0]).decode("utf16")

            v = bundle()
            v.text = c
            v.font_size = font.font_size
            v.advance = g.advance_x
            v.x0 = g.x0
            v.y0 = g.y0
            v.x1 = g.x1
            v.y1 = g.y1

            uv_to_char[k] = v

    # iterate all groups to identify characters

    txt_tex_id = viz.get_font_atlas().get_texture_id()

    pp = None

    for i, (cmd, polys) in enumerate(
            zip(state.draw_cmds, state.polygon_groups)):

        if txt_tex_id != cmd.texture_id:
            continue

        new_polys = []
        half_space = False

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
            p.x0 = char_info.x0
            p.y0 = char_info.y0
            p.x1 = char_info.x1
            p.y1 = char_info.y1

            # check if we have vertical text

            min_vtx = min(p.vertices, key=lambda v: v.pos[0]**2 + v.pos[1]**2)
            min_uv = min(p.vertices, key=lambda v: v.uv[0]**2 + v.uv[1]**2)

            p.vertical_text = min_vtx is not min_uv

            # determine position

            min_x, min_y = np.array([v.pos for v in p.vertices]).min(axis=0)
            max_x, max_y = np.array([v.pos for v in p.vertices]).max(axis=0)

            if p.vertical_text:
                p.last_char_x = min_x
                p.last_char_y = max_y
                p.start_x = max_x - p.y1 + p.font_size/2
                p.start_y = max_y + p.x0
            else:
                p.last_char_x = min_x
                p.last_char_y = min_y
                p.start_x = min_x - p.x0
                p.start_y = min_y - p.y0 + p.font_size/2

            # check if we can merge it with a previous char

            if len(new_polys) == 0:
                new_polys.append(p)
                pp = new_polys[-1]
                continue

            if pp is None:
                new_polys.append(p)
                pp = new_polys[-1]
                continue

            if p.vertical_text:
                adv_dist = (pp.last_char_y - p.x0) - (p.last_char_y - pp.x0)
                base_dist = (p.last_char_x - p.y0) - (pp.last_char_x - pp.y0)
            else:
                adv_dist = (p.last_char_x - p.x0) - (pp.last_char_x - pp.x0)
                base_dist = (p.last_char_y - p.y0) - (pp.last_char_y - pp.y0)

            space_size = 3.1821829676628113
            space_step = abs(adv_dist - pp.advance) / space_size
            round_space_steps = round(space_step)

            if abs(space_step - round_space_steps) < 1e-3:
                pp.text += " " * round_space_steps

            can_be_joined = (abs(adv_dist - pp.advance) < p.font_size
                             and abs(base_dist) < 1e-4
                             and (pp.font_size == p.font_size)
                             and (pp.color == p.color)
                             and (pp.alpha == p.alpha)
                             and (pp.vertical_text == p.vertical_text))

            if not can_be_joined:
                new_polys.append(p)
                pp = new_polys[-1]
                continue

            # join and continue

            pp.text += p.text
            pp.vertices += p.vertices
            pp.advance = p.advance
            pp.last_char_x = p.last_char_x
            pp.last_char_y = p.last_char_y
            pp.x0 = p.x0
            pp.y0 = p.y0

        state.polygon_groups[i] = new_polys


def export_images(state):

    txt_tex_id = viz.get_font_atlas().get_texture_id()

    for cmd, pg in zip(state.draw_cmds, state.polygon_groups):

        if cmd.texture_id == txt_tex_id:
            continue

        # found something with a different texture
        # we need to get that texture!

        texture = viz.get_texture(cmd.texture_id)

        for p in pg:
            p.image = texture


def export_canvas(state):

    canvas_min = np.ones(2) * float("inf")
    canvas_max = np.ones(2) * -float("inf")

    for cmd in state.draw_cmds:
        canvas_min[0] = min(canvas_min[0], cmd.clip_rect[0])
        canvas_min[1] = min(canvas_min[1], cmd.clip_rect[1])
        canvas_max[0] = max(canvas_max[0], cmd.clip_rect[2])
        canvas_max[1] = max(canvas_max[1], cmd.clip_rect[3])

    state.canvas_pos = canvas_min
    state.canvas_size = canvas_max - canvas_min


def export_drawlist_state(dl):

    state = DrawListState()
    state.draw_cmds = dl.get_cmds()

    export_polygons(state, dl)
    export_text_polygons(state)
    # merge_polygons_to_lines(state)
    export_images(state)
    export_canvas(state)

    return state


def polygon_to_svg(p):

    if p.image is not None:

        np_vtx = np.array([v.pos for v in p.vertices])
        box_min = np_vtx.min(axis=0)
        box_max = np_vtx.max(axis=0)
        box_dims = box_max - box_min

        with io.BytesIO() as fd:
            img = Image.fromarray(p.image)
            img.save(fd, "png")
            fd.seek(0)
            str_img = base64.b64encode(fd.read()).decode("utf8")

        svg_txt = f'<image x="{box_min[0]:.3f}" y="{box_min[1]:.3f}" '
        svg_txt += f'width="{box_dims[0]:.3f}" height="{box_dims[1]:.3f}" '
        svg_txt += 'preserveAspectRatio="none" '
        svg_txt += f'xlink:href="data:image/png;base64,{str_img}" '
        svg_txt += '/>'

        return svg_txt

    if len(p.text) == 0:
        svg_txt = '<polygon points="'

        for i, v in enumerate(p.vertices):
            svg_txt += f'{v.pos[0]:.3f},{v.pos[1]:.3f}'
            if i != len(p.vertices) - 1:
                svg_txt += " "

        svg_txt += f'" fill="{p.color}" fill-opacity="{p.alpha}" />'
    else:
        # no idea why this is necessary, but it works
        char_size = p.font_size * 0.8

        np_vtx = np.array([v.pos for v in p.vertices])
        box_min = np_vtx.min(axis=0)
        box_max = np_vtx.max(axis=0)
        box_mean = box_min + (box_max - box_min) * 0.5

        t_x, t_y = box_mean[0], box_mean[1]

        if p.vertical_text:
            svg_txt = ('<text '
                       + 'transform="translate('
                       + f'{(t_x + 5):.3f}, {t_y:.3f}) '
                       + 'rotate(-90)" ')
        else:
            svg_txt = f'<text x="{t_x:.3f}" y="{(t_y + 5):.3f}" '

        svg_txt += (f'fill="{p.color}" fill-opacity="{p.alpha}" '
                    + 'style="font-family: Source Sans Pro; '
                    + f'font-size: {char_size}px; '
                    + f'text-anchor: middle;" '
                    + f'>{p.text}</text>')

    return svg_txt


def line_to_svg(ln):

    svg_txt = '<polyline points="'
    for p in ln.points:
        svg_txt += f'{p[0]:.3f}, {p[1]:.3f} '
    svg_txt += f'" fill="none" stroke="{ln.color}" stroke-opacity="{ln.alpha}" />'

    return svg_txt


def drawlist_state_to_svg(state):

    # output svg text

    svg_txt = ('<svg version="1.1" '
               + 'viewBox="'
               + f'{state.canvas_pos[0]} '
               + f'{state.canvas_pos[1]} '
               + f'{state.canvas_size[0]} '
               + f'{state.canvas_size[1]}" '
               + 'xmlns="http://www.w3.org/2000/svg" '
               + 'xmlns:xlink="http://www.w3.org/1999/xlink">\n')

    # write clip rects

    svg_txt += "<defs>\n"

    for i, cmd in enumerate(state.draw_cmds):
        cx = cmd.clip_rect[0]
        cy = cmd.clip_rect[1]
        cw = cmd.clip_rect[2] - cx
        ch = cmd.clip_rect[3] - cy
        svg_txt += (f'<clipPath id="clip_rect_{i}">'
                    + f'<rect x="{cx}" y="{cy}" '
                    + f'width="{cw}" height="{ch}" />'
                    + '</clipPath>\n')

    svg_txt += "</defs>\n"

    # write out polygon and line groups

    for i, pg in enumerate(state.polygon_groups):
        svg_txt += f'<g clip-path="url(#clip_rect_{i})">\n'
        for p in pg:
            svg_txt += polygon_to_svg(p) + "\n"
        svg_txt += '</g>\n'

        #svg_txt += f'<g clip-path="url(#clip_rect_{i})">\n'
        #for ln in state.line_groups[i]:
        #    svg_txt += line_to_svg(ln) + "\n"
        #svg_txt += '</g>\n'

    # close and return

    svg_txt += '</svg>\n'

    return svg_txt


class PlotExport:

    plot_id = -1
    countdown = 0
    filetype = ""
    path = os.path.abspath(os.getcwd())
    csv_data = {}


def wrap_begin(begin_func):

    def inner(*args, **kwargs):

        res = begin_func(*args, **kwargs)

        if (PlotExport.plot_id == viz.get_plot_id()
                and PlotExport.countdown < 1
                and PlotExport.filetype != "png"):
            viz.disable_aa()
            if PlotExport.filetype == "csv":
                csv_data = {}

        return res

    return inner


def wrap_plot(plot_func):

    def inner(*args, **kwargs):

        plot_func(*args, **kwargs)

        if (PlotExport.plot_id == viz.get_plot_id()
                and PlotExport.countdown < 1
                and PlotExport.filetype == "csv"):
            if "label" in kwargs:
                data_key = kwargs["label"]
            else:
                data_key = str(len(PlotExport.csv_data))

            if len(args) == 1:
                y_data = np.array(args[0])
                x_data = np.arange(len(args[0]))
            elif len(args) == 2:
                y_data = np.array(args[1])
                x_data = np.array(args[0])

            l = min(x_data.shape[0], y_data.shape[0])
            PlotExport.csv_data[data_key] = np.vstack((x_data[:l], y_data[:l])).T

    return inner


def wrap_end(end_func):

    def inner():

        current_plot_id = viz.get_plot_id()

        if current_plot_id is None:
            end_func()
            return

        dl = viz.get_window_drawlist()

        file_dialog_requested = False

        viz.push_override_id(current_plot_id)
        if viz.begin_popup("##PlotContext"):
            if viz.begin_menu("Export"):
                if viz.menu_item("As csv"):
                    PlotExport.filetype = "csv"
                    file_dialog_requested = True
                if viz.menu_item("As pdf", enabled=pdf_avail):
                    PlotExport.filetype = "pdf"
                    file_dialog_requested = True
                if viz.menu_item("As pdf + latex", enabled=pdf_avail):
                    PlotExport.filetype = "pdf_tex"
                    file_dialog_requested = True
                if viz.menu_item("As png"):
                    PlotExport.filetype = "png"
                    file_dialog_requested = True
                if viz.menu_item("As svg"):
                    PlotExport.filetype = "svg"
                    file_dialog_requested = True
                viz.end_menu()
            viz.separator()
            viz.end_popup()

        if file_dialog_requested:
            viz.open_popup("Select export path")

        # create export path chooser

        PlotExport.path = viz.file_dialog_popup(
                "Select export path",
                PlotExport.path,
                "Export")

        export_requested = viz.mod()

        viz.pop_id()

        plot_pos = viz.get_window_pos()
        plot_size = viz.get_window_size()

        end_func()

        # do actual export

        if PlotExport.plot_id == current_plot_id:
            if PlotExport.countdown < 1:

                os.makedirs(PlotExport.path, exist_ok=True)

                if os.path.isdir(PlotExport.path):
                    PlotExport.path += "/"
                if PlotExport.filetype != "csv" and PlotExport.path.endswith("/"):
                    PlotExport.path += f"plot_{int(time.time() * 10**9)}"

                if PlotExport.filetype == "csv":

                    for k, v in PlotExport.csv_data.items():
                        np.savetxt(os.path.join(PlotExport.path, f"{k}.csv"),
                                   v,
                                   delimiter=",",
                                   header="x,y",
                                   comments="")

                elif PlotExport.filetype == "svg":

                    if not PlotExport.path.endswith(".svg"):
                        PlotExport.path += ".svg"

                    dl_state = export_drawlist_state(dl)
                    svg_txt = drawlist_state_to_svg(dl_state)

                    with open(PlotExport.path, "w+") as fd:
                        fd.write(svg_txt)

                elif PlotExport.filetype == "png":

                    if not PlotExport.path.endswith(".png"):
                        PlotExport.path += ".png"

                    pixels = viz.get_pixels(plot_pos[0],
                                            plot_pos[1],
                                            plot_size[0],
                                            plot_size[1])

                    Image.fromarray(pixels).save(PlotExport.path)

                elif PlotExport.filetype == "pdf":

                    # first export as svg

                    tmp_path = f"/tmp/imviz_exp_{int(time.time() * 10**9)}.svg"

                    dl_state = export_drawlist_state(dl)
                    svg_txt = drawlist_state_to_svg(dl_state)

                    with open(tmp_path, "w+") as fd:
                        fd.write(svg_txt)

                    # then convert to pdf

                    if not PlotExport.path.endswith(".pdf"):
                        PlotExport.path += ".pdf"

                    exp_cmd = (f'inkscape --file="{tmp_path}" '
                               + '--export-area-drawing '
                               + '--without-gui '
                               + f'--export-pdf="{PlotExport.path}"')

                    subprocess.call(exp_cmd,
                                    shell=True,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)

                elif PlotExport.filetype == "pdf_tex":

                    # first export as svg

                    tmp_path = f"/tmp/imviz_exp_{int(time.time() * 10**9)}.svg"

                    dl_state = export_drawlist_state(dl)
                    svg_txt = drawlist_state_to_svg(dl_state)

                    with open(tmp_path, "w+") as fd:
                        fd.write(svg_txt)

                    # then convert to pdf

                    if not PlotExport.path.endswith(".pdf"):
                        PlotExport.path += ".pdf"

                    exp_cmd = (f'inkscape --file="{tmp_path}" '
                               + '--export-area-drawing '
                               + '--without-gui '
                               + '--export-latex '
                               + f'--export-pdf="{PlotExport.path}"')

                    subprocess.call(exp_cmd,
                                    shell=True,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)

                PlotExport.plot_id = -1
                PlotExport.filetype = ""

            else:
                PlotExport.countdown -= 1

        if export_requested:
            PlotExport.plot_id = current_plot_id
            PlotExport.countdown += 2

    return inner


begin_plot = wrap_begin(viz.begin_plot)
begin_figure = wrap_begin(viz.begin_figure)

plot = wrap_plot(viz.plot)

end_plot = wrap_end(viz.end_plot)
end_figure = wrap_end(viz.end_figure)
