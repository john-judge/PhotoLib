import numpy as np
import matplotlib.figure as figure


class TraceViewer:
    def __init__(self, data):
        self.data = data
        self.fig = figure.Figure()
        self.axes = []
        self.traces = []
        self.trace_colors = []
        self.point_line_locations = None
        self.clear_point_line_locations()

    def get_fig(self):
        return self.fig

    def onpress(self, event):
        if event.button == 2:
            if event.xdata is not None and len(self.axes) > 0:
                x = int(event.xdata)
                self.set_probe_line_location(x)
                self.update_new_traces()

    def update_new_traces(self):
        self.clear_figure()
        self.populate_figure()

    def populate_figure(self):
        num_traces = len(self.traces)
        gs = self.fig.add_gridspec(max(1, num_traces), 1)
        self.axes = []
        n = self.data.get_num_pts()
        t = np.linspace(0, n * self.data.get_int_pts(), num=n)
        probe_line = self.get_point_line_locations(key='probe')

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

            if probe_line is not None:
                self.axes[i].axvline(x=probe_line,
                                     ymin=-20 * int(i == 0),  # only the first trace's line extends
                                     ymax=1 + 10 * int(i == 0),
                                     c="gray",
                                     linewidth=3,
                                     clip_on=False)
        self.fig.canvas.draw_idle()

    def add_trace(self, pixel_index=None, color='b', fp_index=None):
        trace = self.data.get_display_trace(index=pixel_index,
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
        self.clear_point_line_locations()
        self.update_new_traces()

    def get_point_line_locations(self, key=None):
        if key is None or key not in self.point_line_locations:
            return self.point_line_locations
        return self.point_line_locations[key]

    def set_probe_line_location(self, v):
        self.point_line_locations['probe'] = v

    def clear_point_line_locations(self):
        self.point_line_locations = {
            'probe': None,
            'measure_window': []
        }




