import numpy as np
import matplotlib.figure as figure
from matplotlib.widgets import Slider

from pyPhoto21.hyperslicer import HyperSlicer


class FrameViewer:
    def __init__(self, data, show_rli=True):
        self.data = data
        self.hyperslicer = None
        self.num_frames = None
        self.show_rli = show_rli
        if show_rli:
            self.num_frames = self.data.get_num_rli_pts()
        else:
            self.num_frames = self.data.get_num_pts()
        self.ind = self.num_frames // 2

        self.fig = figure.Figure()
        self.ax = None
        self.smax = None
        self.current_frame = None
        self.im = None
        self.populate_figure()

    def populate_figure(self):
        self.ax = self.fig.add_subplot(111)
        axmax = self.fig.add_axes([0.25, 0.01, 0.65, 0.03])
        self.smax = Slider(axmax, 'Frame Selector', 0, np.max(self.num_frames), valinit=self.ind)

        self.current_frame = self.data.get_display_frame(index=self.ind,
                                                         get_rli=self.show_rli)

        self.im = self.ax.imshow(self.current_frame,
                                aspect = 'auto',
                                cmap='jet')
        self.update()

    def get_slider_max(self):
        return self.smax

    def get_fig(self):
        return self.fig

    def change_frame(self, event):

        self.ind = int(self.smax.val) % self.num_frames
        print('Changing frame to ', self.ind)
        self.update()

    def onclick(self, event):
        print('%s click: button=%d, x=%d, y=%d, xdata=%f, ydata=%f' %
              ('double' if event.dblclick else 'single', event.button,
               event.x, event.y, event.xdata, event.ydata))

    def update_new_image_size(self):
        print('updating frame size...')
        self.fig.clf()
        self.populate_figure()

    def update(self):
        print('updating frame...')
        self.current_frame = self.data.get_display_frame(index=self.ind,
                                                         get_rli=self.show_rli)

        self.im.set_data(self.current_frame)
        self.im.set_clim(vmin=np.min(self.current_frame),
                         vmax=np.max(self.current_frame))

        #self.ax.set_ylabel('slice %s' % self.ind)
        self.im.axes.figure.canvas.draw()
        if self.hyperslicer is not None:
            self.hyperslicer.update_data(show_rli=self.show_rli)

    def get_show_rli_flag(self):
        return self.show_rli

    def launch_hyperslicer(self):
        self.hyperslicer = HyperSlicer(self.data, show_rli=self.show_rli)

