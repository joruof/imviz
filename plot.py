import sys
import numpy as np
import imviz as viz


class Demo:

    def __autogui__(s, **kwargs):

        if not viz.wait(powersave=True):
            sys.exit()

        viz.show_imgui_demo(True)
        viz.show_implot_demo(True)
        viz.style_colors_light()

        img = np.zeros((100, 100, 3))
        img[:50, :50, :1] = 1.0
        img[:50, 50:, 1] = 1.0
        img[50:, :50, :2] = 1.0
        img[50:, 50:, 2] = 1.0

        if viz.begin_window("Plot Example"):

            if viz.begin_plot("Plot"):

                viz.setup_axis(viz.Axis.X1, "x")
                viz.setup_axis(viz.Axis.Y1, "y-axis")

                if viz.plot_selection_ended():
                    viz.hard_cancel_plot_selection()

                viz.plot(
                    np.random.rand(100) * 3,
                    fmt="-o",
                    label="Points and stuff",
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

                viz.plot_image("test", img, 0, 0, 10, 1)

                viz.end_plot()

        viz.end_window()


def main():
    viz.dev.launch(Demo, "__autogui__")


if __name__ == "__main__":
    main()
