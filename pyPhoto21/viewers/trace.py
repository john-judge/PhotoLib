import numpy as np
import matplotlib.figure as figure
from matplotlib.offsetbox import TextArea, AnnotationBbox
from sklearn.linear_model import LinearRegression
from numpy.polynomial import polynomial
from scipy.ndimage import gaussian_filter


class Trace:
    def __init__(self, points, int_pts, start_frame=0, end_frame=-1, is_fp_trace=False):
        if end_frame < 0:
            end_frame = len(points)
        if type(points) != np.ndarray:
            points = np.array(points)
        self.points = points
        self.start_frame = start_frame
        self.end_frame = end_frame
        self.int_pts = int_pts
        self.is_fp_trace = is_fp_trace

    def get_data(self):
        t = np.linspace(self.start_frame * self.int_pts,
                        self.end_frame * self.int_pts,
                        self.end_frame - self.start_frame)
        points = self.points[self.start_frame:self.end_frame]
        return t, points

    def apply_inverse(self):
        if self.is_fp_trace:
            return
        self.points = 0 - self.points

    def clip_time_window(self, window):
        self.start_frame, self.end_frame = window

    def baseline_correct_noise(self, fit_type, skip_window):
        """ subtract background drift off of single trace """
        if self.is_fp_trace:
            return
        t, trace = self.get_data()
        n = len(trace)

        # Skip points
        skip_start, skip_end = skip_window
        skip_int = [i for i in range(skip_start, skip_end)]
        trace_skipped = np.delete(trace, skip_int)
        t_skipped = np.delete(t, skip_int)

        poly_powers = {
            'Quadratic': 2,
            'Cubic': 3,
            "Polynomial-8th": 8
        }
        if fit_type in ["Exponential", "Linear"]:
            t = t.reshape(-1, 1)
            t_skipped = t_skipped.reshape(-1, 1)
            min_val = None
            if fit_type == "Exponential":
                min_val = np.min(trace_skipped)
                if min_val <= 0:
                    trace_skipped += (-1 * min_val + 0.01)  # make all positive
                trace_skipped = np.log(trace_skipped)
            trace_skipped = trace_skipped.reshape(-1, 1)
            reg = LinearRegression().fit(t_skipped, trace_skipped).predict(t)
            if fit_type == "Exponential":
                reg = np.exp(reg)
                if min_val <= 0:
                    reg -= (-1 * min_val + 0.01)
        elif fit_type in poly_powers:
            power = poly_powers[fit_type]
            coeffs, stats = polynomial.polyfit(t_skipped, trace_skipped, power, full=True)
            reg = np.array(polynomial.polyval(t, coeffs))
        else:
            self.points = trace
            return trace

        trace = (trace.reshape(-1, 1) - reg.reshape(-1, 1)).reshape(-1)
        self.points = trace
        return trace

    def filter_temporal(self, filter_type, sigma_t):
        """ Temporal filtering: 1-d binomial 8 filter (approx. Gaussian) """
        if self.is_fp_trace:
            return
        t, trace = self.get_data()
        n = len(trace)
        m = None  # filter kernel length

        if filter_type == 'Gaussian':
            trace = gaussian_filter(trace,
                                    sigma=sigma_t)
        elif filter_type == 'Low Pass':  # i.e. Moving Average
            trace = np.convolve(trace,
                                np.ones(int(sigma_t)),
                                'same') / int(sigma_t)
            m = int(sigma_t)
        elif filter_type.startswith('Binomial-'):
            n = int(filter_type[-1])
            binom_coeffs = (np.poly1d([0.5, 0.5]) ** n).coeffs  # normalized binomial filter
            trace = np.convolve(trace,
                                binom_coeffs,
                                'same')
            m = len(binom_coeffs)

        elif filter_type != 'None':
            print("T-Filter", filter_type, "not implemented.")

        # the trace length itself is same, but now
        # apply cropping to valid filtering domain, respecting any existing filtering
        if m is not None:
            self.start_frame += m
            self.end_frame -= m
        self.points = trace


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

    @staticmethod
    def get_display_value_options():
        return ['None', 'RLI', 'Max Amp', 'MaxAmp/SD', ]

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
            if len(tmp_color) < i + 1:
                tmp_color.append('red')
            trace = self.data.get_display_trace(index=self.pixel_indices[i]['pixel_index'],
                                                fp_index=self.pixel_indices[i]['fp_index'])
            _, points = trace.get_data()
            print(points.shape)
            if len(points.shape) == 1:
                self.traces.append(trace)
                self.trace_colors.append(tmp_color[i])
            else:
                print("Invalid trace generated from pix index:", self.pixel_indices[i]['pixel_index'])

    def create_annotation_text(self, region_count, i):
        px_ind = self.pixel_indices[i]['pixel_index']
        fp_ind = self.pixel_indices[i]['fp_index']
        trace_annotation_text = None
        if px_ind is None and type(fp_ind) == int:  # FP trace
            trace_annotation_text = "FP " + str(fp_ind)
        elif len(px_ind) == 1 and len(px_ind[0]) == 2:  # single-px trace
            x_px, y_px = px_ind[0]
            trace_annotation_text = "(" + str(x_px) + ", " + str(y_px) + ") px"
        elif len(px_ind) > 1 and fp_ind is None:  # region trace
            trace_annotation_text = "Region #" + str(region_count)
            region_count += 1
        return trace_annotation_text, region_count

    def create_display_value(self, display_type, i, trace):
        value_to_display = None
        pixel_index = self.pixel_indices[i]['pixel_index']
        if display_type == 'RLI':
            if pixel_index is not None and type(pixel_index) != int:
                rli_frame = self.data.calculate_rli()

                h, w = rli_frame.shape
                mask = self.data.get_frame_mask(h, w, pixel_index)
                if mask is not None:  # region
                    value_to_display = np.average(rli_frame[mask])
                else:  # single pixel
                    value_to_display = rli_frame[pixel_index[0, 1], pixel_index[0, 0]]
            else:  # FP, no RLI
                return ''
        elif display_type == "Max Amp":
            value_to_display = np.max(trace)
        elif display_type == "MaxAmp/SD":
            std = np.std(trace)
            if std == 0:
                return ''
            value_to_display = np.max(trace) / std
        elif display_type != "None":
            print("Displaying", display_type, "in trace viewer not implemented")

        if value_to_display is None:
            return ''

        # String, truncated
        value_to_display = str(value_to_display)
        if len(value_to_display) > 6:
            value_to_display = value_to_display[:6]
        return ", " + display_type + " " + value_to_display

    def populate_figure(self):
        num_traces = len(self.traces)
        int_pts = self.data.get_int_pts()
        gs = self.fig.add_gridspec(max(1, num_traces), 1)
        self.axes = []
        n = self.data.get_num_pts()
        t = np.linspace(0, n * int_pts, num=n)
        probe_line = self.get_point_line_locations(key='probe')
        region_count = 1
        value_type_to_display = self.get_display_value_options()[self.data.get_display_value_option_index()]

        for i in range(num_traces):
            trace = self.traces[i]
            if isinstance(trace, Trace):
                t, trace = trace.get_data()
            # Sometimes FP trace length doesn't match image trace length
            elif n != trace.shape[0]:
                n = trace.shape[0]
                t = np.linspace(0, n * int_pts, num=n)

            self.axes.append(self.fig.add_subplot(gs[i, 0]))
            self.axes[-1].plot(t, trace, color=self.trace_colors[i])
            if num_traces > 4 and i != num_traces - 1:
                self.axes[-1].get_xaxis().set_visible(False)

            # Annotation trace
            trace_annotation_text, region_count = self.create_annotation_text(region_count, i)

            if trace_annotation_text is not None:
                trace_annotation_text += self.create_display_value(value_type_to_display, i, trace)
                trace_annotation_text = TextArea(trace_annotation_text)
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
                                        xybox=(0.5, 1.16 - 0.16 * int(num_traces == 1)),
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
