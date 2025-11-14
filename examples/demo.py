'''
Inspired by the imgui/implot demo files this file demonstrates the usage of imviz by example.
'''

from dataclasses import dataclass

import imviz as viz
import numpy as np


@dataclass
class Demo:
	vec_2d = [0, 0]

	def __autogui__(self, name, **kwargs):
		viz.set_main_window_title('Demo')
		viz.show_imgui_demo(True)
		viz.show_implot_demo(True)
		
		if viz.begin_window('Style Editor'):
			viz.style_editor()
		viz.end_window()

		# Debug
		if (viz.tree_node('Fullscreen', viz.TreeNodeFlags.COLLAPSING_HEADER)):
			if viz.button('Toggle Fullscreen'):
				viz.toggle_fullscreent()

		if (viz.tree_node('Input System', viz.TreeNodeFlags.COLLAPSING_HEADER)):
			viz.text('Press [CTRL+K]!')
			for e in viz.get_key_events():
				if e.key == viz.KEY_K:
					if e.action == viz.PRESS and e.mod == viz.MOD_CONTROL:
						viz.open_popup('##pressed')
			
			if viz.begin_popup('##pressed'):
				viz.text('You pressed it!')
				viz.end_popup()
		
		if (viz.tree_node('2D Input', viz.TreeNodeFlags.COLLAPSING_HEADER)):
			if viz.begin_plot('2D Input', flags=viz.PlotFlags.CANVAS_ONLY | viz.PlotFlags.CROSSHAIRS):
				viz.setup_axes_limits(-1, 1, -1, 1)
				viz.setup_axes('', '', viz.PlotAxisFlags.LOCK, viz.PlotAxisFlags.LOCK)

				self.vec_2d = viz.drag_point('point', self.vec_2d)
				self.vec_2d = np.clip(self.vec_2d, -1, 1)
				viz.plot_annotation(self.vec_2d[0], self.vec_2d[1], 'P(%.2f, %.2f)' % tuple(self.vec_2d), offset=[0, 5], clamp=True)
				viz.end_plot()
			viz.set_next_item_width(-2)
			self.vec_2d[1] = viz.slider('##y', self.vec_2d[1], -1, 1, 'Y = %.2f')
			viz.set_next_item_width(-2)
			self.vec_2d[0] = viz.slider('##x', self.vec_2d[0], -1, 1, 'X = %.2f')
		
		if (viz.tree_node('Font Scaling', viz.TreeNodeFlags.COLLAPSING_HEADER)):
			viz.text('Main window, before scaling.')
			if (viz.begin_child('Child', [0, 200], border=True)):
				viz.set_window_font_scale()
				viz.text('Child window, before scaling.')
				viz.set_window_font_scale(2)
				viz.text('Child window, after scaling.')
				viz.end_child()
			viz.text('Main window, after scaling.')


if __name__ == '__main__':
	viz.set_main_window_size([1440, 960])

	demo = Demo()
	while viz.wait():
		viz.autogui(demo)
