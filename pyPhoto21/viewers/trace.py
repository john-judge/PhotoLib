import numpy as np
import matplotlib.figure as figure
from matplotlib.offsetbox import TextArea, AnnotationBbox
from sklearn.linear_model import LinearRegression
from numpy.polynomial import polynomial
from scipy.ndimage import gaussian_filter

from pyPhoto21.viewers.viewer import Viewer


class Trace:
    def __init__(self, points, int_pts, start_frame=0, end_frame=-1, is_fp_trace=False,
                 pixel_indices=None, fp_index=None, masks=None, master_mask=None,
                 annotation=None):
        if end_frame < 0:
            end_frame += len(points)
        if type(points) != np.ndarray:
            points = np.array(points)
        self.points = points
        self.start_frame = start_frame
        self.end_frame = end_frame
        self.int_pts = int_pts
        self.is_fp_trace = is_fp_trace

        self.pixel_indices = pixel_indices
        self.color = 'b'
        self.annotation = annotation
        self.masks = masks
        self.master_mask = master_mask
        if master_mask is None and masks is not None and len(masks) > 0:
            self.master_mask = masks[0]
            for m in masks[1:]:
                self.master_mask = np.logical_or(self.master_mask, m)
        self.fp_index = fp_index

    def merge_masks(self, masks):
        if self.is_fp_trace or self.master_mask is None:
            return  # can't merge mask into FP traces
        if self.masks is None:
            self.masks = [self.master_mask]
        # intersection detection: if the mask's pixel counts are less than the sum,
        # we have only 1 shape and not 2
        # note that we need a union-find algorithm because the number of merges could
        # be any number. See https://en.wikipedia.org/wiki/Disjoint-set_data_structure
        # We utilize the assumption that masks in each mask list are pairwise disjoint,
        # having been union-find merged earlier.
        for new_mask in masks:
            self.master_mask = np.logical_or(self.master_mask, new_mask)
            indices_intersects_with = []
            for i in range(len(self.masks)):
                is_intersect, union = self.do_masks_intersect(new_mask, self.masks[i])
                if is_intersect:
                    new_mask = union
                    indices_intersects_with.append(i)
            for ind in indices_intersects_with[::-1]:  # iterate backwards since we are deleting elements
                self.masks.pop(ind)
            self.masks.append(new_mask)

    def get_pixel_count(self):
        if self.is_fp_trace or self.master_mask is None:
            return 0
        if self.master_mask is None:
            return 0
        return np.sum(self.master_mask)

    def subtract_mask(self, mask):
        if self.is_fp_trace or self.master_mask is None:
            return  # N/A for FP traces
        self.master_mask = self.mask_subtraction(self.master_mask, mask)
        if np.sum(self.master_mask) == 0:
            self.masks = []
            return
        for i in range(len(self.masks) - 1, -1, -1):
            self.masks[i] = self.mask_subtraction(self.masks[i], mask)
            if np.sum(self.masks[i]) == 0:
                self.masks.pop(i)  # delete empty masks

    # returns True if masks intersect, and also returns union of masks
    @staticmethod
    def do_masks_intersect(mask1, mask2):
        union = np.logical_or(mask1, mask2)
        return np.sum(mask1) + np.sum(mask2) > np.sum(union), union

    # remove mask2 intersect mask1 from mask1
    @staticmethod
    def mask_subtraction(mask1, mask2):
        return np.logical_and(mask1, np.logical_xor(mask1, mask2))

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
        # normalize amplitudes to [-0.5, 0.5], clipping dependent
        # The zoom factor is the max amplitude
        pts = self.get_data_clipped()
        amp_max = np.max(pts)
        amp_min = np.min(pts)
        self.points = (self.points - amp_min).astype(np.float32)
        if amp_max > amp_min:
            self.points /= float(amp_max - amp_min)
        else:
            return self.points

        # [0,1] -> [-0.5, 0.5]
        self.points -= 0.5

        # apply y-zoom factor
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
        if t_skipped.size < 1:
            return trace

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


class TraceViewer(Viewer):
    def __init__(self, gui):
        super().__init__()

        self.data = gui.data
        self.gui = gui
        self.fig = figure.Figure()
        self.ax = None

        # Arrays whose indices align (better to refactor to a list of TracePlot images)
        self.traces = []

        self.probe_line_location = None
        self.clear_probe_line_location()
        self.y_zoom_factor = 1.0
        self.y_zoom_bounds = [0.1, 20.0]
        self.x_zoom_factor = 2.0
        self.x_zoom_bounds = [0.5, 20.0]
        self.x_pan_start = None
        self.x_pan_offset = 0.0

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
            self.x_pan_start = event.x
            self.press = True
        elif event.button == 3:  # right mouse
            if self.gui.fv.is_control_key_held():
                self.reset_trace_view()
            else:
                self.delete_trace(int(event.ydata))
            self.gui.fv.update_new_image()
        else:
            print("button:", event.button)

    def onmove(self, event):
        if self.press and event.button == 1 and self.x_pan_start is not None:
            if event.xdata is not None:
                width_in_px = self.get_width_in_pixels(self.ax, self.fig)
                curr_x_lims = self.ax.get_xlim()
                self.x_pan_offset += (self.x_pan_start - event.x) / width_in_px \
                                     * (curr_x_lims[1] - curr_x_lims[0])
            self.x_pan_start = event.x
            self.moving = True
            self.update_new_traces()

    def onrelease(self, event):
        if self.press and not self.moving:
            self.onpress(event)
        self.press = False
        self.moving = False
        self.x_pan_start = None
        self.x_pan_offset = 0.0

    def onscroll(self, event):
        if self.gui.fv.is_control_key_held():  # frameviewer has the focus and gets the keypresses
            if event.button == 'up':
                self.increase_x_zoom()
            elif event.button == 'down':
                self.decrease_x_zoom()
        else:
            if event.button == 'up':
                self.increase_y_zoom()
            elif event.button == 'down':
                self.decrease_y_zoom()

    def get_current_x_pan_offset(self):
        return self.x_pan_offset

    def reset_trace_view(self):
        self.x_pan_offset = 0.0
        self.x_zoom_factor = 2.0
        self.y_zoom_factor = 1.0

    @staticmethod
    def get_zoom_int():
        return 0.2

    def decrease_y_zoom(self):
        tmp = self.y_zoom_factor
        zoom_int = self.get_zoom_int()
        self.y_zoom_factor = max(self.y_zoom_bounds[0], self.y_zoom_factor - zoom_int)
        self.update_new_traces()
        if tmp != self.y_zoom_factor:
            self.update_new_traces()

    def increase_y_zoom(self):
        tmp = self.y_zoom_factor
        zoom_int = self.get_zoom_int()
        self.y_zoom_factor = min(self.y_zoom_bounds[1], self.y_zoom_factor + zoom_int)
        if tmp != self.y_zoom_factor:
            self.update_new_traces()

    def decrease_x_zoom(self):
        tmp = self.x_zoom_factor
        zoom_int = self.get_zoom_int()
        self.x_zoom_factor = max(self.x_zoom_bounds[0], self.x_zoom_factor - zoom_int)
        self.update_new_traces()
        if tmp != self.x_zoom_factor:
            self.update_new_traces()

    def increase_x_zoom(self):
        tmp = self.x_zoom_factor
        zoom_int = self.get_zoom_int()
        self.x_zoom_factor = min(self.x_zoom_bounds[1], self.x_zoom_factor + zoom_int)
        if tmp != self.x_zoom_factor:
            self.update_new_traces()

    def delete_trace(self, ind):
        # y_plot is the y-location, rounded down to the nearest int.
        # trace at index i is located between [i, i+1)
        if 0 <= ind < len(self.traces):
            self.traces.pop(ind)
            self.update_new_traces()

    def update_new_traces(self):
        self.clear_figure()
        self.regenerate_traces()
        self.populate_figure()

    # Recompute all traces from saved pixel indices.
    # Useful when processing settings have changed
    def regenerate_traces(self):
        for i in range(len(self.traces)):
            col = self.traces[i].color
            trace = self.data.get_display_trace(index=self.traces[i].pixel_indices,
                                                fp_index=self.traces[i].fp_index,
                                                zoom_factor=self.y_zoom_factor,
                                                masks=self.traces[i].masks)
            if trace is not None:
                trace.annotation = self.traces[i].annotation
                trace.color = col
                _, points = trace.get_data()
                if len(points.shape) == 1:
                    trace.color = col
                    self.traces[i] = trace
                else:
                    print("Invalid trace generated from pix index:", self.traces[i].pixel_indices)

    def create_annotation_text(self, region_count, i):
        px_ind = self.traces[i].pixel_indices
        fp_ind = self.traces[i].fp_index
        trace_annotation_text = None
        if px_ind is None and type(fp_ind) == int:  # FP trace
            trace_annotation_text = "FP " + str(fp_ind)
        elif len(px_ind) == 1 and len(px_ind[0]) == 2:  # single-px trace
            x_px, y_px = px_ind[0]
            trace_annotation_text = "(" + str(x_px) + ", " + str(y_px) + ") px"
        elif len(px_ind) > 1 and fp_ind is None:  # region trace
            trace_annotation_text = "Region " + str(region_count)
            region_count += 1
        return trace_annotation_text, region_count

    def create_display_value(self, display_type, i, trace):
        value_to_display = None
        pixel_index = self.traces[i].pixel_indices
        if display_type == 'RLI':
            if pixel_index is not None and type(pixel_index) != int:
                rli_frame = self.data.calculate_rli()

                h, w = rli_frame.shape
                mask = self.data.get_frame_mask(h, w, pixel_index)
                if mask is not None:  # region
                    try:
                        value_to_display = np.average(rli_frame[mask])
                    except IndexError:
                        return ''
                else:  # single pixel
                    if pixel_index[0, 1] < h and pixel_index[0, 0] < w:
                        value_to_display = rli_frame[pixel_index[0, 1], pixel_index[0, 0]]
                    else:
                        return ''
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
        probe_line = self.get_probe_line_locations()
        region_count = 1
        value_type_to_display = self.get_display_value_options()[self.data.get_display_value_option_index()]
        view_window = self.data.get_artifact_exclusion_window()
        # For our plot, we assume all traces are normalized to [0,1] (see get_display_trace in data.py)
        for i in range(num_traces):
            trace = self.traces[i]
            points = np.array([])
            if isinstance(trace, Trace):
                trace.clip_time_window(view_window)
                times, points = trace.get_data()
            else:
                print("Not a Trace object:", type(trace))

            if times.shape[0] != points.shape[0]:
                print("time and trace shape mismatch:", times.shape, points.shape)
            else:
                points += i  # add constant to plot trace in its own space.
                self.ax.plot(times, points, color=trace.color)

            # Annotate trace
            trace_annotation_text = trace.annotation
            if trace_annotation_text is None:
                trace_annotation_text, region_count = self.create_annotation_text(region_count, i)

            if trace_annotation_text is not None:
                trace_annotation_text += self.create_display_value(value_type_to_display, i, points)
                trace_annotation_text = TextArea(trace_annotation_text)
                ab = AnnotationBbox(trace_annotation_text,
                                    (-0.14, (i + 0.5) / num_traces),
                                    xycoords='axes fraction',
                                    xybox=(-0.14, (i + 0.5) / num_traces),
                                    boxcoords=("axes fraction", "axes fraction"),
                                    box_alignment=(0., 1.0))
                self.ax.add_artist(ab)

        # y-lim must be set for zoom factor to work
        self.ax.set_ylim([-1, num_traces])

        # x-lim is used to create zoom factor effect
        n = self.data.get_num_pts()
        trace_duration = abs(view_window[1] % n - view_window[0] % n) * self.data.get_int_pts()
        trace_mid_point = view_window[0] % n * self.data.get_int_pts() + trace_duration / 2
        zoom_x_radius = trace_duration / self.x_zoom_factor
        x_center = trace_mid_point + self.get_current_x_pan_offset()  # x window offset from center
        self.ax.set_xlim([x_center - zoom_x_radius,
                          x_center + zoom_x_radius])

        # Point line (probe type) -- one for the whole plot, now
        if probe_line is not None:
            self.ax.axvline(x=probe_line,
                            ymin=-2,
                            ymax=2,
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
                                    xybox=(0.5, 1.05),
                                    boxcoords=("axes fraction", "axes fraction"),
                                    box_alignment=(0., 0.5),
                                    arrowprops=dict(arrowstyle="->"))
                self.ax.add_artist(ab)

        # Measure window visualization
        if num_traces > 0:
            self.draw_measure_window()
            self.draw_baseline_window()

        self.fig.canvas.draw_idle()

    def draw_baseline_window(self):
        bl_start, bl_end = self.data.core.get_baseline_skip_window()
        self.draw_window(bl_start, bl_end, "Baseline Skip", 'red', -0.1)

    def draw_measure_window(self):
        measure_start, measure_end = self.data.get_measure_window()
        self.draw_window(measure_start, measure_end, "Measure", 'blue', 1.1)

    def draw_window(self, start, end, name, color, y_loc):
        start, end = self.normalize_and_restrict_window(start,
                                                        end,
                                                        self.data.get_artifact_exclusion_window())
        start *= self.data.get_int_pts()
        end *= self.data.get_int_pts()
        if start < end:
            self.ax.axvspan(start,
                            end,
                            color=color,
                            alpha=0.2)
            box_x_loc = (start + end) / 2
            ab = AnnotationBbox(TextArea(name),
                                (start, y_loc),
                                xycoords=("data", "axes fraction"),
                                xybox=(box_x_loc, y_loc),
                                boxcoords=("data", "axes fraction"),
                                box_alignment=(0., 0.5),
                                arrowprops=dict(arrowstyle="->"))
            self.ax.add_artist(ab)
            ab = AnnotationBbox(TextArea(name),
                                (end, y_loc),
                                xycoords=("data", "axes fraction"),
                                xybox=(box_x_loc, y_loc),
                                boxcoords=("data", "axes fraction"),
                                box_alignment=(0., 0.5),
                                arrowprops=dict(arrowstyle="->"))
            self.ax.add_artist(ab)

    # make window positive and restrict to within view window
    def normalize_and_restrict_window(self, start, end, view_window):
        n = self.data.get_num_pts()
        if end == -1:
            end = n
        start = max(view_window[0] % n, start % n)
        end = min(view_window[1] % n, end % n)
        if end < start:
            end, start = start, end
        return start, end

    def add_trace(self, pixel_index=None, color='b', fp_index=None):
        trace = self.data.get_display_trace(index=pixel_index,
                                            fp_index=fp_index)
        if trace is not None:
            self.traces.append(trace)
            trace.color = color
            self.update_new_traces()
            return True
        return False

    def append_to_last_trace(self, pixel_index=None):
        i = self.get_last_region_index()
        if i is None:
            return False
        h = self.data.get_display_height()
        w = self.data.get_display_width()
        mask = self.data.get_frame_mask(h, w, index=pixel_index)

        if mask is not None:
            self.traces[i].merge_masks([mask])  # need to merge traces.
            self.update_new_traces()
            return True
        return False

    def remove_from_last_trace(self, pixel_index=None):
        i = self.get_last_region_index()
        if i is None:
            return False
        h = self.data.get_display_height()
        w = self.data.get_display_width()
        mask = self.data.get_frame_mask(h, w, index=pixel_index)

        if mask is not None:
            self.traces[i].subtract_mask(mask)  # need to merge traces.
            if self.traces[i].get_pixel_count() < 1:
                self.traces.pop(i)
            self.update_new_traces()
            return True
        return False

    def clear_figure(self):
        self.fig.clf()

    def clear_traces(self):
        self.traces = []
        self.clear_probe_line_location()
        self.update_new_traces()

    # Ignoring FP traces
    def get_last_region_index(self):
        ind = len(self.traces) - 1
        if ind < 0:
            return None
        while ind >= 0 and (self.traces[ind].is_fp_trace or self.traces[ind].master_mask is None):
            ind -= 1
        if self.traces[ind] is None or ind < 0:
            return None
        return ind

    def get_probe_line_locations(self):
        return self.probe_line_location

    def set_probe_line_location(self, v):
        self.probe_line_location = v

    def clear_probe_line_location(self):
        self.probe_line_location = None
