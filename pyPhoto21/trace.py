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

    def update_new_traces(self):
        self.clear_figure()
        self.populate_figure()

    def populate_figure(self):
        num_traces = len(self.traces)
        gs = self.fig.add_gridspec(max(1, num_traces), 1)
        self.axes = []
        n = self.data.get_num_pts()
        t = np.linspace(0, n * self.data.get_int_pts(), num=n)

        for i in range(num_traces):
            trace = self.traces[i]

            # Sometimes FP trace length doesn't match image trace length
            if n != trace.shape[0]:
                n = trace.shape[0]
                t = np.linspace(0, n * self.data.get_int_pts(), num=n)

            self.axes.append(self.fig.add_subplot(gs[i, 0]))
            self.axes[-1].plot(t, trace, color=self.trace_colors[i])
            if num_traces > 4 and i != num_traces - 1:
                self.axes[-1].get_xaxis().set_visible(False)
        self.fig.canvas.draw_idle()

    def add_trace(self, pixel_index=None, color='b', fp_index=None):
        trace = self.data.get_display_trace(index=pixel_index,
                                            trial=self.get_trial_no(),
                                            fp_index=fp_index)
        if trace is not None:
            self.traces.append(trace)
            self.trace_colors.append(color)
            self.update_new_traces()
            return True
        return False

    def clear_figure(self):
        self.fig.clf()

    def clear_traces(self):
        self.traces = []
        self.trace_colors = []
        self.update_new_traces()



