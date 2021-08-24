import numpy as np
import matplotlib.figure as figure
from matplotlib.offsetbox import TextArea, AnnotationBbox
from sklearn.linear_model import LinearRegression
from numpy.polynomial import polynomial
from scipy.ndimage import gaussian_filter


class Trace:
    def __init__(self, points, int_pts, start_frame=0, end_frame=-1, is_fp_trace=False):
        if end_frame < 0:
            end_frame += len(points)
        if type(points) != np.ndarray:
            points = np.array(points)
        self.points = points
        self.start_frame = start_frame
        self.end_frame = end_frame
        self.int_pts = int_pts
        self.is_fp_trace = is_fp_trace

    def get_data(self):
        start = min(self.start_frame, self.end_frame)
        end = max(self.start_frame, self.end_frame)
        t = np.linspace(start * self.int_pts,
                        end * self.int_pts,
                        end - start)
        points = self.points[start:end]
        return t, points

    def get_data_unclipped(self):
        return self.points

    def get_data_clipped(self):
        start = min(self.start_frame, self.end_frame)
        end = max(self.start_frame, self.end_frame)
        return self.points[start:end]

    def get_start_point(self):
        return self.start_frame

    def get_end_point(self):
        return self.end_frame

    def apply_inverse(self):
        if self.is_fp_trace:
            return
        self.points = 0 - self.points

    def normalize(self, zoom_factor=1.0):
        # normalize amplitudes to [0, 1], clipping dependent
        # The zoom factor is the max amplitude
        pts = self.get_data_clipped()
        amp_max = np.max(pts)
        amp_min = np.min(pts)
        self.points = (self.points - amp_min)
        if amp_max > amp_min:
            self.points /= (amp_max - amp_min)
        if zoom_factor != 1:
            self.points *= zoom_factor
        return self.points

    # apply crop window, w/o overriding existing window
    def clip_time_window(self, window):
        n = len(self.points)
        start, end = window
        if end < 0:
            end += n
        self.start_frame = max(self.start_frame, start)
        self.end_frame = min(self.end_frame, end)

    def clear_crop_window(self):
        self.start_frame = 0
        self.end_frame = len(self.points)

    def baseline_correct_noise(self, fit_type, skip_window):
        """ subtract background drift off of single trace """
        if self.is_fp_trace:
            return

        # The baseline should use clipped data (unlike t-filter)
        t, trace = self.get_data()
        full_t = np.linspace(0, len(self.points) * self.int_pts, len(self.points))
        full_trace = self.get_data_unclipped()

        # Skip points
        skip_start, skip_end = skip_window

        # Translate skip window by clip window
        skip_start = max(0, skip_start - self.start_frame)
        skip_end = min(self.end_frame, skip_end - self.start_frame)

        skip_int = [i for i in range(skip_start, skip_end) if i < trace.shape[0]]
        trace_skipped = trace
        t_skipped = t
        if len(skip_int) > 0:
            trace_skipped = np.delete(trace, skip_int)
            t_skipped = np.delete(t, skip_int)

        poly_powers = {
            'Quadratic': 2,
            'Cubic': 3,
            "Polynomial-8th": 8
        }
        if fit_type in ["Exponential", "Linear"]:
            full_t = full_t.reshape(-1, 1)
            t_skipped = t_skipped.reshape(-1, 1)
            min_val = None
            if fit_type == "Exponential":
                min_val = np.min(trace_skipped)
                if min_val <= 0:
                    trace_skipped += (-1 * min_val + 0.01)  # make all positive
                trace_skipped = np.log(trace_skipped)
            trace_skipped = trace_skipped.reshape(-1, 1)
            reg = LinearRegression().fit(t_skipped, trace_skipped).predict(full_t)
            if fit_type == "Exponential":
                reg = np.exp(reg)
                if min_val <= 0:
                    reg -= (-1 * min_val + 0.01)
        elif fit_type in poly_powers:
            power = poly_powers[fit_type]
            coeffs, stats = polynomial.polyfit(t_skipped, trace_skipped, power, full=True)
            reg = np.array(polynomial.polyval(full_t, coeffs))
        else:
            return trace

        full_trace = (full_trace.reshape(-1, 1) - reg.reshape(-1, 1)).reshape(-1)
        self.points[:] = full_trace[:]
        return trace

    def filter_temporal(self, filter_type, sigma_t):
        """ Temporal filtering: 1-d binomial 8 filter (approx. Gaussian) """
        if self.is_fp_trace:
            return
        trace = self.get_data_unclipped()
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
            self.clip_time_window([m, n - m])
        self.points[:] = trace[:]


class TraceViewer:
    def __init__(self, gui):
        self.data = gui.data
        self.gui = gui
        self.fig = figure.Figure()
        self.ax = None
        self.traces = []
        self.pixel_indices = []
        self.trace_colors = []
        self.point_line_locations = None
        self.clear_point_line_locations()
        self.zoom_factor = 1.0
        self.zoom_bounds = [0.1, 20.0]

    def get_traces(self):
        return self.traces

    @staticmethod
    def get_display_value_options():
        return ['None', 'RLI', 'Max Amp', 'MaxAmp/SD', ]

    def get_fig(self):
        return self.fig

    def onpress(self, event):
        if event.button == 2:
            if event.xdata is not None and self.ax is not None:
                x = int(event.xdata)
                self.set_probe_line_location(x)
                self.update_new_traces()
        elif event.button == 1:  # left mouse
            print( event)
        elif event.button == 3:  # right mouse
            self.delete_trace(int(event.ydata))

    def onscroll(self, event):
        if event.button == 'up':
            self.increase_zoom()
        elif event.button == 'down':
            self.decrease_zoom()

    def decrease_zoom(self):
        tmp = self.zoom_factor
        zoom_int = int(self.zoom_factor) ** 2
        self.zoom_factor = max(self.zoom_bounds[0], self.zoom_factor - zoom_int)
        self.update_new_traces()
        if tmp != self.zoom_factor:
            self.update_new_traces()

    def increase_zoom(self):
        tmp = self.zoom_factor
        zoom_int = int(self.zoom_factor) ** 2
        self.zoom_factor = min(self.zoom_bounds[1], self.zoom_factor + zoom_int)
        if tmp != self.zoom_factor:
            self.update_new_traces()

    def delete_trace(self, ind):
        # y_plot is the y-location, rounded down to the nearest int.
        # trace at index i is located between [i, i+1)
        if 0 <= ind < len(self.traces):
            self.traces.pop(ind)
            self.pixel_indices.pop(ind)
            self.trace_colors.pop(ind)
            self.gui.fv.delete_shape(ind)
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
                                                fp_index=self.pixel_indices[i]['fp_index'],
                                                zoom_factor=self.zoom_factor)
            _, points = trace.get_data()
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
        return "\n" + display_type + " " + value_to_display

    def populate_figure(self):
        num_traces = len(self.traces)
        gs = self.fig.add_gridspec(1, 1)  # previously had max(1, num_traces) rows, each trace in own subplot
        self.ax = self.fig.add_subplot(gs[0, 0])
        self.ax.get_yaxis().set_visible(False)  # intensity units are arbitrary
        times = None
        probe_line = self.get_point_line_locations(key='probe')
        region_count = 1
        value_type_to_display = self.get_display_value_options()[self.data.get_display_value_option_index()]

        # For our plot, we assume all traces are normalized to [0,1] (see get_display_trace in data.py)
        for i in range(num_traces):
            trace = self.traces[i]
            if isinstance(trace, Trace):
                trace.clip_time_window(self.data.get_crop_window())
                times, trace = trace.get_data()
            else:
                print("Not a Trace object:", type(trace))

            if times.shape[0] != trace.shape[0]:
                print("time and trace shape mismatch:", times.shape, trace.shape)
            else:
                trace += i  # add constant to plot trace in its own space.
                self.ax.plot(times, trace, color=self.trace_colors[i])

            # Annotation trace
            trace_annotation_text, region_count = self.create_annotation_text(region_count, i)

            if trace_annotation_text is not None:
                trace_annotation_text += self.create_display_value(value_type_to_display, i, trace)
                trace_annotation_text = TextArea(trace_annotation_text)
                ab = AnnotationBbox(trace_annotation_text,
                                    (-0.14, (i + 0.5) / num_traces),
                                    xycoords='axes fraction',
                                    xybox=(-0.14, (i + 0.5) / num_traces),
                                    boxcoords=("axes fraction", "axes fraction"),
                                    box_alignment=(0., 1.0))
                self.ax.add_artist(ab)

        # Point line (probe type) -- one for the whole plot, now
        if probe_line is not None:
            self.ax.axvline(x=probe_line,
                            ymin=-20,
                            ymax=1 + 10,
                            c="gray",
                            linewidth=3,
                            clip_on=False)
            # Point line annotation
            if num_traces > 0:
                probe_annotation = TextArea(str(probe_line)[:5] + " ms")
                y_annotate = self.ax.get_ylim()[1] * 0.9
                ab = AnnotationBbox(probe_annotation,
                                    (probe_line, y_annotate),
                                    xycoords='data',
                                    xybox=(0.5, 1.1),
                                    boxcoords=("axes fraction", "axes fraction"),
                                    box_alignment=(0., 0.5),
                                    arrowprops=dict(arrowstyle="->")
                                    )
                self.ax.add_artist(ab)
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
