import numpy as np
import matplotlib.figure as figure
from matplotlib.widgets import Slider
import matplotlib.gridspec as gridspec
import matplotlib.pyplot as plt

from pyPhoto21.hyperslicer import HyperSlicer


class FrameViewer:
    def __init__(self, data, show_rli=True):
        self.data = data
        self.hyperslicer = None
        self.num_frames = None
        self.ind = 0

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
        self.update_new_traces()

    def get_trial_index(self):
        return self.trial_index

    def populate_figure(self):
        # top row of Field Potential traces
        num_fp = min(9, self.data.get_num_fp())
        num_rows = 4
        gs = self.fig.add_gridspec(num_rows, num_fp)
        self.fp_axes = []


        fp_data = self.data.get_fp_data(trial=self.get_trial_index())
        print(fp_data)
        print(np.sum(fp_data != 0))
        t = [i * self.data.get_int_pts() for i in range(self.data.get_num_pts())]
        for i in range(num_fp):
            self.fp_axes.append(self.fig.add_subplot(gs[0, i]))
            self.fp_axes[i].plot(t, fp_data[i, :])
            self.fp_axes[i].set_title("FP " + str(i))
            #self.fp_axes[i].get_xaxis().set_visible(False)
            #self.fp_axes[i].get_yaxis().set_visible(False)

        # Rest of the plot is the image
        self.ax = self.fig.add_subplot(gs[1:,:])
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

    def onclick(self, event):
        print('%s click: button=%d, x=%d, y=%d, xdata=%f, ydata=%f' %
              ('double' if event.dblclick else 'single', event.button,
               event.x, event.y, event.xdata, event.ydata))

    def update_new_image(self):
        self.fig.clf()
        self.populate_figure()
        self.update()

    def update(self, update_hyperslicer=True):
        print('updating frame...')
        self.current_frame = self.data.get_display_frame(index=self.ind,
                                                         get_rli=self.show_rli)

        self.im.set_data(self.current_frame)
        self.im.set_clim(vmin=np.min(self.current_frame),
                         vmax=np.max(self.current_frame))

        #self.ax.set_ylabel('slice %s' % self.ind)
        self.im.axes.figure.canvas.draw()
        if self.hyperslicer is not None and update_hyperslicer:
            self.hyperslicer.update_data(show_rli=self.show_rli)

    def get_show_rli_flag(self):
        return self.show_rli

    def set_show_rli_flag(self, value, update=False):
        self.show_rli = value

        # choose correct data dimensions for viewer
        if self.show_rli:
            self.num_frames = self.data.get_num_rli_pts()
        else:
            self.num_frames = self.data.get_num_pts()
        self.ind = self.num_frames // 2

        # Adjust the slider values to match the data dimensions
        if self.smax is not None:
            self.smax.valmax = self.num_frames
            self.smax.val = self.ind
            self.smax.ax.set_xlim(self.smax.valmin,
                                  self.smax.valmax)
            self.fig.canvas.draw_idle()
        if update:
            self.update_new_image()


    def launch_hyperslicer(self):
        self.hyperslicer = HyperSlicer(self.data, show_rli=self.show_rli)

