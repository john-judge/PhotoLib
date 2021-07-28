import numpy as np
import matplotlib.figure as figure


class TraceViewer:
    def __init__(self, data):
        self.data = data
        self.fig = figure.Figure()
        self.num_traces_plotted = 0
        self.axes = []
        self.add_trace()

    def get_trial_no(self):
        return 0  # replace with self.data data

    def get_fig(self):
        return self.fig

    def add_trace(self, pixel_index=None):
        trace = self.data.get_display_trace(index=pixel_index,
                                            trial=self.get_trial_no())

        print(trace)
        if trace is not None:

            i = self.num_traces_plotted + 1
            self.axes.append(self.fig.add_subplot(i, 1, i))
            n = self.data.get_num_pts()
            t = np.linspace(0, n * self.data.get_int_pts(), num=n)
            self.axes[-1].plot(t, trace)
            self.fig.canvas.draw()

    def clear_traces(self):
        self.fig.clf()



