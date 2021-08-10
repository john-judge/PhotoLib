import numpy as np
import matplotlib.figure as figure
from matplotlib.offsetbox import TextArea, AnnotationBbox


class TraceViewer:
    def __init__(self, data):
        self.data = data
        self.fig = figure.Figure()
        self.axes = []
        self.traces = []
        self.pixel_indices = []
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
        self.regenerate_traces()
        self.populate_figure()

    # Recompute all traces from saved pixel indices.
    # Useful when processing settings have changed
    def regenerate_traces(self):
        self.traces = []
        tmp_color = self.trace_colors
        self.trace_colors = []
        for i in range(len(self.pixel_indices)):
            if len(tmp_color) < i+1:
                tmp_color.append('red')
            trace = self.data.get_display_trace(index=self.pixel_indices[i]['pixel_index'],
                                                fp_index=self.pixel_indices[i]['fp_index'])
            self.traces.append(trace)
            self.trace_colors.append(tmp_color[i])

    def populate_figure(self):
        num_traces = len(self.traces)
        int_pts = self.data.get_int_pts()
        gs = self.fig.add_gridspec(max(1, num_traces), 1)
        self.axes = []
        n = self.data.get_num_pts()
        t = np.linspace(0, n * int_pts, num=n)
        probe_line = self.get_point_line_locations(key='probe')
        region_count = 1

        for i in range(num_traces):
            trace = self.traces[i]

            # Sometimes FP trace length doesn't match image trace length
            if n != trace.shape[0]:
                n = trace.shape[0]
                t = np.linspace(0, n * int_pts, num=n)

            self.axes.append(self.fig.add_subplot(gs[i, 0]))
            self.axes[-1].plot(t, trace, color=self.trace_colors[i])
            if num_traces > 4 and i != num_traces - 1:
                self.axes[-1].get_xaxis().set_visible(False)

            # Annotation trace
            px_ind = self.pixel_indices[i]['pixel_index']
            fp_ind = self.pixel_indices[i]['fp_index']
            trace_annotation_text = None
            if px_ind is None and type(fp_ind) == int:  # FP trace
                trace_annotation_text = TextArea("FP " + str(fp_ind))
            elif len(px_ind) == 1 and len(px_ind[0]) == 2:  # single-px trace
                x_px, y_px = px_ind[0]
                trace_annotation_text = TextArea("(" + str(x_px) + ", " + str(y_px) + ") px")
            elif len(px_ind) > 1 and fp_ind is None:  # region trace
                trace_annotation_text = TextArea("Region #" + str(region_count))
                region_count += 1

            if trace_annotation_text is not None:
                ab = AnnotationBbox(trace_annotation_text,
                                    (0.02, 0.95),
                                    xycoords='axes fraction',
                                    xybox=(0.02, 0.95),
                                    boxcoords=("axes fraction", "axes fraction"),
                                    box_alignment=(0., 1.0))
                self.axes[i].add_artist(ab)

            # Point line (probe type)
            if probe_line is not None:
                self.axes[i].axvline(x=probe_line,
                                     ymin=-20 * int(i == 0),  # only the first trace's line extends
                                     ymax=1 + 10 * int(i == 0),
                                     c="gray",
                                     linewidth=3,
                                     clip_on=False)
                # Point line annotation
                if num_traces > 0:
                    probe_annotation = TextArea(str(probe_line)[:5] + " ms")
                    y_annotate = self.axes[0].get_ylim()[1] * 0.9
                    ab = AnnotationBbox(probe_annotation,
                                        (probe_line, y_annotate),
                                        xycoords='data',
                                        xybox=(0.5, 1.16 - 0.1 * int(num_traces == 0)),
                                        boxcoords=("axes fraction", "axes fraction"),
                                        box_alignment=(0., 0.5),
                                        arrowprops=dict(arrowstyle="->")
                                        )
                    self.axes[0].add_artist(ab)
        self.fig.canvas.draw_idle()

    def add_trace(self, pixel_index=None, color='b', fp_index=None):
        trace = self.data.get_display_trace(index=pixel_index,
                                            fp_index=fp_index)
        if trace is not None:
            self.pixel_indices.append({'pixel_index': pixel_index,
                                       'fp_index': fp_index})
            self.traces.append(trace)
            self.trace_colors.append(color)
            self.update_new_traces()
            return True
        return False

    def clear_figure(self):
        self.fig.clf()

    def clear_traces(self):
        self.pixel_indices = []
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




