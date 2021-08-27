import numpy as np
import matplotlib.figure as figure
from matplotlib.widgets import Slider
from imantics import Polygons, Mask

from collections import defaultdict

from pyPhoto21.analysis.hyperslicer import HyperSlicer


class FrameViewer:
    def __init__(self, data, tv):
        self.data = data
        self.hyperslicer = None
        self.num_frames = None
        self.ind = 0
        self.tv = tv  # TraceViewer
        self.show_processed_data = False

        # Mouse event state
        self.press = False
        self.moving = False
        self.draw_path = []
        self.path_x_index = defaultdict(list)
        self.colors = ['red', 'green', 'cyan', 'magenta', 'yellow', 'black', 'blue']

        # Key event state
        self.control_key_down = False

        self.slider_enabled = self.should_use_frame_selector()
        self.smax = None

        self.fig = figure.Figure(constrained_layout=True)
        self.ax = None
        self.fp_axes = []

        self.current_frame = None
        self.im = None
        self.livefeed_im = None
        self.update_num_frames()
        self.populate_figure()

        self.update()

    def should_use_frame_selector(self):
        bg_name = self.data.get_background_options()[self.data.get_background_option_index()]
        return (not self.data.db.meta.show_rli) and self.data.bg_uses_frame_selector(bg_name)

    def get_current_frame(self):
        return self.current_frame

    @staticmethod
    def get_color_map_options():
        return ['jet', 'gray', 'viridis', 'hot', 'Spectral', 'turbo', 'rainbow']

    def get_color_map_option_index(self):
        return self.data.meta.color_map_option

    def get_color_map_option_name(self):
        return self.get_color_map_options()[self.get_color_map_option_index()]

    def set_color_map_option_name(self, **kwargs):
        v = kwargs['values']
        opt_ind = self.get_color_map_options().index(v)
        self.set_color_map_option_index(opt_ind)

    def set_color_map_option_index(self, v):
        tmp = self.data.meta.color_map_option
        self.data.meta.color_map_option = v
        if tmp != v:
            self.update_new_image()

    def populate_figure(self):
        # top row of Field Potential traces
        num_fp = min(9, self.data.get_num_fp())
        num_rows = 6
        gs = self.fig.add_gridspec(num_rows, num_fp)
        self.fp_axes = []

        fp_data = self.data.get_fp_data()
        n = fp_data.shape[0]
        t = np.linspace(0, n * self.data.get_int_pts(), num=n)
        for i in range(num_fp):
            self.fp_axes.append(self.fig.add_subplot(gs[0, i]))
            self.fp_axes[i].plot(t, fp_data[:, i])
            self.fp_axes[i].set_title("FP " + str(i))
            if num_fp > 4 and i > 0:  # for many FP, only left-most plot needs labelled y-axis
                self.fp_axes[i].get_yaxis().set_visible(False)

        # Rest of the plot is the image
        self.ax = self.fig.add_subplot(gs[1:-1, :])  # leaves last row blank -- for Slider
        if self.slider_enabled:
            axmax = self.fig.add_axes([0.25, 0.01, 0.65, 0.03])

            self.smax = Slider(axmax,
                               'Frame Selector',
                               0,
                               self.num_frames,
                               valinit=self.ind)

        self.refresh_current_frame()
        if self.current_frame is not None:
            self.im = self.ax.imshow(self.current_frame,
                                     aspect='auto',
                                     cmap=self.get_color_map_option_name())

    def enable_disable_slider(self):
        self.slider_enabled = self.should_use_frame_selector()
        self.update_new_image()

    def get_slider_max(self):
        if self.slider_enabled:
            return self.smax
        return None

    def get_fig(self):
        return self.fig

    def change_frame(self, event):
        if not self.get_show_rli_flag() and self.slider_enabled:
            new_ind = int(self.smax.val) % self.num_frames
            if new_ind != self.ind:
                self.ind = new_ind
                self.update()

    def onrelease(self, event):
        if self.press and not self.moving:
            self.change_frame(event)
            self.onclick(event)
        else:
            self.ondrag(event)
        self.press = False
        self.moving = False

    def onpress(self, event):
        if not self.press:
            if event.button == 1 or event.button == 3:
                self.clear_waypoints()
                self.add_waypoint(event)
            self.press = True

    def onmove(self, event):
        if self.press and event.button in [1, 3]:  # left or right mouse
            self.add_waypoint(event)
            self.moving = True

    def onkeypress(self, event):
        if event.key == 'control':
            self.control_key_down = True

    def onkeyrelease(self, event):
        if event.key == 'control':
            self.control_key_down = False

    def is_control_key_held(self):
        return self.control_key_down

    def onclick(self, event):
        if event.button == 3:  # right click
            self.tv.clear_traces()
            self.update_new_image()
        elif event.button == 1:  # left click
            if event.inaxes == self.ax:
                # clicked frame
                self.ondrag(event)
            else:
                # clicked on a FP trace. Move to TraceViewer.
                for i in range(len(self.fp_axes)):
                    if event.inaxes == self.fp_axes[i]:
                        self.tv.add_trace(fp_index=i)

    def get_next_color(self):
        return self.colors[(len(self.tv.traces) - 1) % len(self.colors)]

    # Identified drag has completed
    def ondrag(self, event):
        is_deletion = (event.button == 3)
        ctrl = self.is_control_key_held()
        color = self.get_next_color()
        draw = np.array(self.draw_path)
        success = False
        if is_deletion:
            self.tv.remove_from_last_trace(pixel_index=draw)
            success = True
        elif ctrl:
            success = self.tv.append_to_last_trace(pixel_index=draw)
        if not success:  # if append failed, try to add.
            success = self.tv.add_trace(pixel_index=draw, color=color)

        self.update_new_image()  # pulls the new mask from TraceViewer

        if not success:
            print('No trace created for this ROI selection.')
        self.draw_path = []
        # Seal and determine selected ROI
        # Average traces and add subplot in TraceViewer

    def add_waypoint(self, event):
        if event.xdata is not None and event.ydata is not None:
            x = int(event.xdata)
            y = int(event.ydata)
            # avoid duplicate points
            if x in self.path_x_index and y in self.path_x_index[x]:
                pass
            else:
                if len(self.draw_path) > 0:
                    last_point = self.draw_path[-1]
                    self.draw_line(last_point, [x, y], event.button == 3)
                self.draw_path.append([x, y])
                self.path_x_index[x].append(y)

    def draw_line(self, p1, p2, is_deletion):
        xs = [p1[0], p2[0]]
        ys = [p1[1], p2[1]]
        color = 'white'
        if is_deletion:
            color = 'red'
        self.ax.plot(xs, ys, color)
        self.fig.canvas.draw_idle()

    # convert mask to a list of x-y points, array of shape (k, 2)
    @staticmethod
    def convert_masks_to_polygons(masks):
        polygons_points = []
        for mask in masks:
            polygons = Mask(mask).polygons()
            if len(polygons.points) == 1:
                points = np.array(polygons.points)
                if points.size % 2 == 0:
                    points = points.reshape(-1, 2)
                    polygons_points.append(points)
                else:
                    print("Excluding bad polygon with vertex conversion:", points)
            else:
                for poly in polygons.points:
                    points = np.array(poly)
                    if points.size % 2 == 0:
                        points = points.reshape(-1, 2)
                        polygons_points.append(points)
                    else:
                        print("Excluding bad polygon with vertex conversion:", points)
        return polygons_points

    def plot_all_shapes(self):
        for tr in self.tv.traces:
            if not tr.is_fp_trace and tr.master_mask is not None:
                polygons = self.convert_masks_to_polygons(tr.masks)
                for polygon_pts in polygons:
                    try:
                        self.ax.fill(polygon_pts[:, 0],
                                     polygon_pts[:, 1],
                                     tr.color,
                                     alpha=0.5,
                                     edgecolor='white',
                                     linestyle="-",
                                     linewidth=2)
                    except ValueError as e:
                        print(polygon_pts)
                        print(e)

    def clear_waypoints(self):
        self.draw_path = []
        self.path_x_index = defaultdict(list)

    def update_new_image(self):
        self.fig.clf()
        if self.data.get_is_livefeed_enabled():
            self.start_livefeed_animation()
        else:
            self.redraw_slider()
            self.populate_figure()
            self.update()
            self.plot_all_shapes()

    def refresh_current_frame(self):
        self.current_frame = self.data.get_display_frame(index=self.ind,
                                                         get_rli=self.get_show_rli_flag(),
                                                         binning=self.get_digital_binning())
        # See, contrast scaling applied only to visualization, not to exported data.
        contrast_scaling = self.data.get_contrast_scaling()
        new_vmax = np.max(self.current_frame) / contrast_scaling
        # purposely induce saturation:
        self.current_frame[self.current_frame >= new_vmax] = new_vmax

    def start_livefeed_animation(self):
        print("Starting livefeed animation...")
        self.refresh_current_frame()
        self.ax = self.fig.add_subplot(1, 1, 1)

        self.livefeed_im = self.ax.imshow(self.current_frame.astype(np.uint16),
                                          interpolation='nearest',
                                          aspect='auto',
                                          cmap=self.get_color_map_option_name())
        self.fig.canvas.draw_idle()

    def end_livefeed_animation(self):
        self.livefeed_im = None
        self.update_new_image()

    def update(self, update_hyperslicer=True):
        self.refresh_current_frame()

        if self.data.get_is_livefeed_enabled():
            self.livefeed_im.set_data(self.current_frame)
            update_hyperslicer = False

        elif self.current_frame is not None:
            self.im.set_data(self.current_frame)
            self.im.set_clim(vmin=np.min(self.current_frame),
                             vmax=np.max(self.current_frame))

        # self.ax.set_ylabel('slice %s' % self.ind)
        self.fig.canvas.draw_idle()
        if self.hyperslicer is not None and update_hyperslicer and not self.get_show_rli_flag():
            self.hyperslicer.update_data()

    def get_show_rli_flag(self):
        return self.data.db.meta.show_rli

    def redraw_slider(self):
        # Adjust the slider values to match the data dimensions
        if not self.slider_enabled:
            return
        self.update_num_frames()
        if self.smax is not None:
            self.smax.valmax = self.num_frames
            self.smax.val = self.ind
            self.smax.ax.set_xlim(self.smax.valmin,
                                  self.smax.valmax)
            self.fig.canvas.draw_idle()

    def update_num_frames(self):
        # choose correct data dimensions for viewer
        refresh_ind = False
        if self.get_show_rli_flag():
            refresh_ind = False
            self.num_frames = 1
        else:
            n_frames = self.data.get_num_pts()
            refresh_ind = (n_frames != self.num_frames)
            self.num_frames = n_frames
        if refresh_ind:
            self.ind = self.num_frames // 2

    def set_show_rli_flag(self, value, update=False):
        self.data.db.meta.show_rli = value
        if update:
            self.update_new_image()
        else:
            self.update_num_frames()

    def set_digital_binning(self, binning):
        if binning != self.data.db.meta.binning:
            self.data.db.meta.binning = binning
            self.update_new_image()

    def get_digital_binning(self):
        return self.data.db.meta.binning

    def launch_hyperslicer(self):
        self.hyperslicer = HyperSlicer(self.data, show_rli=self.get_show_rli_flag())
