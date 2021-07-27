import numpy as np
import matplotlib.figure as figure


class TraceViewer:
    def __init__(self, data):
        self.data = data
        self.fig = figure.Figure()
        self.num_traces_plotted = 0
        self.axes = []
        self.add_trace()

    def get_fig(self):
        return self.fig

    def add_trace(self):
        i = self.num_traces_plotted + 1
        self.axes.append(self.fig.add_subplot(i * 100 + 10 + i))
        x = np.linspace(0, 2 * np.pi)
        y = np.sin(x)
        self.axes[-1].plot(x, y)


