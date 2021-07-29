import numpy as np
import matplotlib.figure as figure


class TraceViewer:
    def __init__(self, data):
        self.data = data
        self.fig = figure.Figure()
        self.axes = []
        self.traces = []
        self.trace_colors = []

    def get_trial_no(self):
        return 0  # replace with self.data data

    def get_fig(self):
        return self.fig

    def populate_figure(self):
        num_traces = len(self.traces)
        gs = self.fig.add_gridspec(max(1, num_traces), 1)
        self.axes = []
        n = self.data.get_num_pts()
        t = np.linspace(0, n * self.data.get_int_pts(), num=n)

        for i in range(num_traces):
            trace = self.traces[i]

            self.axes.append(self.fig.add_subplot(gs[i, 0]))
            self.axes[-1].plot(t, trace, color=self.trace_colors[i])
            if num_traces > 4 and i != num_traces - 1:
                self.axes[-1].get_xaxis().set_visible(False)
        self.fig.canvas.draw()

    def add_trace(self, pixel_index=None, color='b'):
        trace = self.data.get_display_trace(index=pixel_index,
                                            trial=self.get_trial_no())
        if trace is not None:
            self.clear_figure()
            self.traces.append(trace)
            self.trace_colors.append(color)
            self.populate_figure()
            return True
        return False

    def clear_figure(self):
        self.fig.clf()

    def clear_traces(self):
        self.clear_figure()
        self.traces = []
        self.trace_colors = []
        self.populate_figure()



