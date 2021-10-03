import numpy as np
import matplotlib.figure as figure
from matplotlib.offsetbox import TextArea, AnnotationBbox


# Visualize the record and stimulate timeline
class DAQViewer:
    def __init__(self, data):
        self.data = data
        self.num_stim_channels = 2
        self.fig = figure.Figure()
        self.ax = None
        self.y_labels = None

    def get_fig(self):
        return self.fig

    def onpress(self, event):
        if event.button == 2:
            if event.xdata is not None:
                x = int(event.xdata)
                self.update()

    def update(self):
        self.clear_figure()
        self.populate_figure()

    def plot_shutter(self):
        shutter_onset = self.data.hardware.get_shutter_onset()
        acqui_onset = self.data.get_acqui_onset()
        acqui_duration = self.data.get_acqui_duration()
        end_time = acqui_onset + acqui_duration
        if shutter_onset < end_time:
            self.ax.broken_barh([(shutter_onset, end_time-shutter_onset)],
                                (10 * (self.num_stim_channels+2), 9),
                                facecolors='tab:blue')

    def plot_stimulator(self, channel, col):
        if channel in range(1, self.num_stim_channels+1):
            stim_onset = self.data.get_stim_onset(channel)
            stim_duration = self.data.get_stim_duration(channel)
            stim_num_pulses = self.data.get_num_pulses(channel)
            stim_num_bursts = self.data.hardware.get_num_bursts(channel=channel)
            stim_int_pulses = self.data.hardware.get_int_pulses(channel=channel)
            stim_int_bursts = self.data.hardware.get_int_bursts(channel=channel)
            interval_tuples = []
            for k in range(stim_num_bursts):
                for j in range(stim_num_pulses):
                    start = stim_onset + j * stim_int_pulses + k * stim_int_bursts
                    interval_tuples.append((start, stim_duration))
            self.ax.broken_barh(interval_tuples,
                                (10 * channel, 9),
                                facecolors='tab:' + col)

    def create_y_labels(self):
        self.y_labels = ['Stim #' + str(ch+1) for ch in range(self.num_stim_channels)]
        self.y_labels.append('Acquisition')
        self.y_labels.append('Shutter')

    def populate_figure(self):
        self.ax = self.fig.add_subplot(1, 1, 1)
        if self.y_labels is None:
            self.create_y_labels()

        # plot acquisition
        acqui_onset = self.data.get_acqui_onset()
        acqui_duration = self.data.get_acqui_duration()
        self.ax.broken_barh([(acqui_onset, acqui_duration)],
                            (10 * (self.num_stim_channels+1), 9),
                            facecolors='tab:blue')

        # plot stimulators' settings
        stim_colors = ['green', 'red']
        for ch in range(1, self.num_stim_channels+1):
            self.plot_stimulator(ch, stim_colors[ch-1])

        self.plot_shutter()
        self.ax.set_xlabel("Time (ms)")
        self.ax.set_yticks([10*i for i in range(1, self.num_stim_channels+3)])
        self.ax.set_yticklabels(self.y_labels)

        self.fig.canvas.draw_idle()

    def clear_figure(self):
        self.fig.clf()
