import numpy as np
import matplotlib.figure as figure
from matplotlib.widgets import Slider
import matplotlib.gridspec as gridspec
import matplotlib.pyplot as plt

from collections import defaultdict

from pyPhoto21.hyperslicer import HyperSlicer


class FrameViewer:
    def __init__(self, data, tv, show_rli=True):
        self.data = data
        self.hyperslicer = None
        self.num_frames = None
        self.ind = 0
        self.binning = 1
        self.tv = tv  # TraceViewer

        # Mouse event state
        self.press = False
        self.moving = False
        self.draw_path = []
        self.path_x_index = defaultdict(list)

        self.trial_index = 0
        self.smax = None
        self.show_rli = None
        self.set_show_rli_flag(show_rli)

        self.fig = figure.Figure(constrained_layout=True)
        self.ax = None
        self.fp_axes = []

        self.current_frame = None
        self.im = None
        self.populate_figure()

        self.update()

    def set_trial_index(self, i):
        self.trial_index = i
        self.update_new_image()

    def get_trial_index(self):
        return self.trial_index

    def populate_figure(self):
        # top row of Field Potential traces
        num_fp = min(9, self.data.get_num_fp())
        num_rows = 6
        gs = self.fig.add_gridspec(num_rows, num_fp)
        self.fp_axes = []

        fp_data = self.data.get_fp_data(trial=self.get_trial_index())
        t = [i * self.data.get_int_pts() for i in range(fp_data.shape[1])]
        for i in range(num_fp):
            self.fp_axes.append(self.fig.add_subplot(gs[0, i]))
            self.fp_axes[i].plot(t, fp_data[i, :])
            self.fp_axes[i].set_title("FP " + str(i))
            if num_fp > 4 and i > 0:
                self.fp_axes[i].get_yaxis().set_visible(False)

        # Rest of the plot is the image
        self.ax = self.fig.add_subplot(gs[1:-1,:]) # leaves last row blank -- for Slider
        axmax = self.fig.add_axes([0.25, 0.01, 0.65, 0.03])
        self.smax = Slider(axmax, 'Frame Selector', 0, np.max(self.num_frames), valinit=self.ind)

        self.current_frame = self.data.get_display_frame(index=self.ind,
                                                         get_rli=self.show_rli)

        self.im = self.ax.imshow(self.current_frame,
                                aspect = 'auto',
                                cmap='jet')

    def get_slider_max(self):
        return self.smax

    def get_fig(self):
        return self.fig

    def change_frame(self, event):
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
            self.clear_waypoints()
            self.add_waypoint(event)
            self.press = True

    def onmove(self, event):
        if self.press:
            self.add_waypoint(event)
            self.moving = True

    def onclick(self, event):
        print('%s click: button=%d, x=%d, y=%d, xdata=%f, ydata=%f' %
              ('double' if event.dblclick else 'single', event.button,
               event.x, event.y, event.xdata, event.ydata))
        if event.button == 3:  # right click
            self.tv.clear_traces()
        elif event.button == 1:  # left click
            pass

    # Identified drag has completed
    def ondrag(self, event):
        self.tv.add_trace(pixel_index=self.draw_path)
        # Seal and determine selected ROI
        # Average traces and add subplot in TraceViewer

    def add_waypoint(self, event):
        if event.xdata is not None and event.ydata is not None:
            x = int(event.xdata)
            y = int(event.ydata)
            # avoid duplicate points
            if x in self.path_x_index and y in self.path_x_index[x]:
                print(x, y, "already in path")
            else:
                self.draw_path.append([x, y])
                self.path_x_index[x].append(y)

    def clear_waypoints(self):
        self.draw_path = []
        self.path_x_index = defaultdict(list)

    def update_new_image(self):
        self.fig.clf()
        self.redraw_slider()
        self.populate_figure()
        self.update()

    def update(self, update_hyperslicer=True):
        print('updating frame...')
        self.current_frame = self.data.get_display_frame(index=self.ind,
                                                         get_rli=self.show_rli,
                                                         binning=self.binning)

        self.im.set_data(self.current_frame)
        self.im.set_clim(vmin=np.min(self.current_frame),
                         vmax=np.max(self.current_frame))

        #self.ax.set_ylabel('slice %s' % self.ind)
        self.im.axes.figure.canvas.draw()
        if self.hyperslicer is not None and update_hyperslicer:
            self.hyperslicer.update_data(show_rli=self.show_rli)

    def get_show_rli_flag(self):
        return self.show_rli

    def redraw_slider(self):
        # Adjust the slider values to match the data dimensions
        self.update_num_frames()
        if self.smax is not None:
            self.smax.valmax = self.num_frames
            self.smax.val = self.ind
            self.smax.ax.set_xlim(self.smax.valmin,
                                  self.smax.valmax)
            self.fig.canvas.draw_idle()

    def update_num_frames(self):
        # choose correct data dimensions for viewer
        if self.show_rli:
            self.num_frames = self.data.get_num_rli_pts()
        else:
            self.num_frames = self.data.get_num_pts()
        self.ind = self.num_frames // 2

    def set_show_rli_flag(self, value, update=False):
        self.show_rli = value
        if update:
            self.update_new_image()
        else:
            self.update_num_frames()

    def set_digital_binning(self, binning):
        if binning != self.binning:
            self.binning = binning
            self.update_new_image()

    def launch_hyperslicer(self):
        self.hyperslicer = HyperSlicer(self.data, show_rli=self.show_rli)

