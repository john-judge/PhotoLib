import numpy as np
import matplotlib.figure as figure


class TraceViewer:
    def __init__(self, data):
        self.data = data
        self.fig = figure.Figure()
        self.ax = self.fig.add_subplot(111)
        self.add_trace()

    def get_fig(self):
        return self.fig

    def add_trace(self):
        x = np.linspace(0, 2 * np.pi)
        y = np.sin(x)
        self.ax.plot(x, y)


