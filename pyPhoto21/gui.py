import numpy as np
import os.path
import PySimpleGUI as sg
import matplotlib
from matplotlib.widgets import RectangleSelector
import matplotlib.figure as figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
import matplotlib.pyplot as plt
from tkinter import *
import tkinter as Tk

from pyPhoto21.frame import FrameViewer
from pyPhoto21.trace import TraceViewer

#from mpl_interactions import image_segmenter, hyperslicer
from matplotlib.widgets import Slider
#import io
#import requests

global_state = {
    'frame_canvas': {
        'redraw': False,
        'fig': None,
        'image': None,
        'cid': None,  # matplotlib connection
    },
    'trace_canvas': {
        'redraw': False,
        'fig': None,
        'traces': [],
        'cid': None,  # matplotlib connection
    },
    'show_rli': True,
}

class GUI:

    def __init__(self, data, hardware, file):
        matplotlib.use("TkAgg")
        sg.theme('DarkBlue12')
        self.data = data
        self.hardware = hardware
        self.file = file
        self.fv = None
        self.window = None

        # general state/settings
        self.title = "Photo21"
        self.event_mapping = None
        self.define_event_mapping() # event callbacks used in event loops
        # kickoff workflow
        self.introduction()
        self.main_workflow()

    def __del__(self):
        for name in global_state:
            try:
                global_state[name]['fig'].canvas.mpl_disconnect(global_state[name]['cid'])
            except:
                pass

    def introduction(self):
        layout = [[
                sg.Column([[sg.Image(key="-IMAGE-")]]),
                [sg.Text("Welcome to Photo21! \n\tCheck that your camera and \n\tNI-USB are turned on.")],
                [sg.Button("OK")]
            ]]
        intro_window = sg.Window(self.title, layout, finalize=True)
        self.intro_event_loop(intro_window)
        intro_window.close()

    @staticmethod
    def intro_event_loop(window, filename='art/meyer.png'):
        window["-IMAGE-"].update(filename=filename)
        while True:
            event, values = window.read()
            # End intro when user closes window or
            # presses the OK button
            if event == "OK" or event == sg.WIN_CLOSED:
                break

    def create_left_column(self):
        acquisition_layout = [
            [sg.Button("STOP!", button_color=('black', 'yellow')),
             sg.Button("Take RLI", button_color=('brown', 'gray'))],
            [sg.Button("Live Feed", button_color=('black', 'gray')),
             sg.Button("Record", button_color=('black', 'red'))],
            [sg.Button("Save Processed Data", button_color=('black', 'green')),
             sg.Button("Save", button_color=('black', 'green'))],
        ]
        analysis_layout = [[
            sg.Button("Launch Hyperslicer", button_color=('gray', 'blue')),
            sg.Button("Record", button_color=('gray', 'red')),
            sg.Button("Save", button_color=('gray', 'green')),
            # sg.In(size=(25, 1), enable_events=True, key="-FOLDER-"),
            # sg.FolderBrowse(),
        ]]

        tab_group = [[sg.TabGroup([[
            sg.Tab('Acquisition', acquisition_layout),
            sg.Tab('Analysis', analysis_layout),
        ]])]]

        frame_viewer_layout = [
            [sg.Canvas(key='frame_canvas_controls')],
            [sg.Column(
                layout=[
                    [sg.Canvas(key='frame_canvas',
                               size=(600, 600)
                               )]
                ],
                background_color='#DAE0E6',
                pad=(0, 0))]]

        return frame_viewer_layout + tab_group

    def create_right_column(self):
        file_list_layout = [[
                sg.Listbox(
                    values=[],
                    enable_events=True,
                    size=(20, 10),
                    key="-FILE LIST-"
                )
            ]]
        trace_viewer_layout = [
            [sg.Canvas(key='trace_canvas_controls')],
            [sg.Column(
                layout=[
                    [sg.Canvas(key='trace_canvas',
                               size=(600, 600)
                               )]
                ],
                background_color='#DAE0E6',
                pad=(0, 0))]]
        return trace_viewer_layout #+ file_list_layout

    def main_workflow(self):
        right_col = self.create_right_column()
        left_col = self.create_left_column()

        layout = [
            [
                sg.Column(left_col),
                sg.VSeperator(),
                sg.Column(right_col),
            ]
        ]

        self.window = sg.Window(self.title,
                           layout,
                           finalize=True,
                           element_justification='center',
                           font='Helvetica 18')
        self.plot_frame()
        self.plot_trace()
        self.main_workflow_loop()
        self.window.close()

    def main_workflow_loop(self, history=False):
        global global_state
        events = ''
        while True:
            event, values = self.window.read()
            if history and event is not None:
                events += str(event) + '\n'
            if event == "Exit" or event == sg.WIN_CLOSED:
                break
            if global_state['frame_canvas']['redraw']:
                self.plot_frame(self.window)
            if global_state['trace_canvas']['redraw']:
                print("Redraw Trace cv!")
                self.plot_trace(self.window)
            elif event not in self.event_mapping or self.event_mapping[event] is None:
                print("Not Implemented:", event)
            else:
                try:
                    ev = self.event_mapping[event]
                    ev['function'](**ev['args'])
                except Exception as e:
                    print("exception while calling", self.event_mapping[event])
                    print(str(e))

        if history:
            print(events)

    def plot_trace(self):
        fig = figure.Figure()
        ax = fig.add_subplot(111)
        x = np.linspace(0, 2 * np.pi)
        y = np.sin(x)
        line, = ax.plot(x, y)

        self.draw_figure_w_toolbar(self.window['trace_canvas'].TKCanvas,
                                   fig,
                                   self.window['trace_canvas_controls'].TKCanvas)

    def plot_frame(self):

        fig = figure.Figure()
        ax = fig.add_subplot(111)

        axmax = fig.add_axes([0.25, 0.01, 0.65, 0.03])
        smax = Slider(axmax, 'Max', 0, np.max(self.data.get_num_pts()), valinit=50)
        self.fv = FrameViewer(self.data, ax)
        #canvas_toolbar = self.window['frame_canvas_controls'].TKCanvas
        canvas = self.window['frame_canvas'].TKCanvas

        figure_canvas_agg = FigureCanvasTkAgg(fig, master=canvas)

        figure_canvas_agg.get_tk_widget().pack(fill="both", expand=True)
        figure_canvas_agg.mpl_connect('scroll_event', self.fv.onscroll)
        figure_canvas_agg.mpl_connect('button_release_event', self.fv.contrast)  # add this for contrast change
        figure_canvas_agg.mpl_connect('button_press_event', self.fv.onclick)
        figure_canvas_agg.draw()
        smax.on_changed(self.fv.contrast)

    # update system state so that frame is redrawn in event loop
    def redraw_frame(self):
        global_state['redraw']['frame_canvas'] = True
        # .. choose frame: select, average, retrieve from self.data ...
        raise NotImplementedError

    # update system state so that traces are redrawn in event loop
    def redraw_trace(self, x_click, y_click, x_release, y_release):
        global_state['redraw']['trace_canvas'] = True
        print("Redraw Trace:", x_click, y_click, x_release, y_release)
        # ... find traces to draw by looking at system state
        print("NotImplementedError -- trace")

    # include a matplotlib figure in a Tkinter canvas
    @staticmethod
    def draw_figure_w_toolbar(canvas, fig, canvas_toolbar):
        if canvas.children:
            for child in canvas.winfo_children():
                child.destroy()
        if canvas_toolbar.children:
            for child in canvas_toolbar.winfo_children():
                child.destroy()
        figure_canvas_agg = FigureCanvasTkAgg(fig, master=canvas)
        figure_canvas_agg.draw()
        toolbar = Toolbar(figure_canvas_agg, canvas_toolbar)
        toolbar.update()
        figure_canvas_agg.get_tk_widget().pack(side='right', fill='both', expand=False)

    def launch_hyperslicer(self):
        print("NotImplemented")

    def record(self, **kwargs):
        self.hardware.record(images=self.data.get_acqui_images())
        self.data.process_acqui_images()
        self.fv.update()

    def take_rli(self, **kwargs):
        self.hardware.take_rli(images=self.data.get_rli_images())
        self.data.process_rli_images()
        if global_state['show_rli']:
            self.fv.update(rli=True)

    def define_event_mapping(self):
        if self.event_mapping is None:
            self.event_mapping = {
                'Record': {
                    'function': self.record,
                    'args': {}
                },
                'Take RLI': {
                    'function': self.take_rli,
                    'args': {}
                },
                'Save': {
                    'function': self.file.save_to_file,
                    'args': {'images': self.data.get_acqui_images()}
                },
                'Launch Hyperslicer': {
                    'function': self.launch_hyperslicer,
                    'args': {},
                }
            }


class Toolbar(NavigationToolbar2Tk):
    def __init__(self, *args, **kwargs):
        super(Toolbar, self).__init__(*args, **kwargs)

