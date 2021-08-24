import numpy as np
import matplotlib.figure as figure
from matplotlib.widgets import Slider
from matplotlib.animation import FuncAnimation
import matplotlib.pyplot as plt

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
        self.shapes = []

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
            if event.button == 1:
                self.clear_waypoints()
                self.add_waypoint(event)
            self.press = True

    def onmove(self, event):
        if self.press and event.button == 1:  # left mouse
            self.add_waypoint(event)
            self.moving = True

    def onclick(self, event):
        if event.button == 3:  # right click
            self.tv.clear_traces()
            self.clear_shapes()
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
        return self.colors[(len(self.shapes) - 1) % len(self.colors)]

    # Identified drag has completed
    def ondrag(self, event):
        color = self.get_next_color()
        draw = np.array(self.draw_path)
        success = self.tv.add_trace(pixel_index=draw,
                                    color=color)
        if success:
            self.add_shape(draw, color)
        else:
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
                self.draw_path.append([x, y])
                self.path_x_index[x].append(y)

    # The points are a Nx2 numpy array of x,y coordinates representing a polygon path
    def add_shape(self, points, color):
        self.shapes.append(points)
        self.ax.fill(points[:, 0],
                     points[:, 1],
                     color,
                     alpha=0.5,
                     edgecolor='white',
                     linestyle="-",
                     linewidth=2)
        self.fig.canvas.draw_idle()

    def delete_shape(self, ind):
        if 0 <= ind < len(self.shapes):
            self.shapes.pop()
            self.update_new_image()

    def clear_shapes(self):
        self.shapes = []
        self.update_new_image()

    def plot_all_shapes(self):
        for i in range(len(self.shapes)):
            points = self.shapes[i]
            col = self.tv.trace_colors[i]
            self.ax.fill(points[:, 0],
                         points[:, 1],
                         col,
                         alpha=0.5,
                         edgecolor='white',
                         linestyle="-",
                         linewidth=2)

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
