import sys
import imviz as viz


class Demo:
    def __autogui__(s, **kwargs):
        if not viz.wait():
            sys.exit()

        viz.set_main_window_title("Plot Example")
        viz.set_main_window_size((1024, 768))
        if viz.begin_window(
            "Plot Example",
            size=(1024, 768),
            position=(0, 0),
            resize=False,
        ):
            if viz.begin_plot("Plot"):
                viz.setup_axis(viz.Axis.X1, "x")
                viz.setup_axis(viz.Axis.Y1, "y")

                if viz.plot_selection_ended():
                    viz.hard_cancel_plot_selection()

                viz.plot(
                    [1, 2, 3],
                    [2, 4, 6],
                    fmt=".o",
                    label="points",
                )
                viz.end_plot()
            viz.end_window()


def main():
    viz.dev.launch(Demo, "__autogui__")


if __name__ == "__main__":
    main()
