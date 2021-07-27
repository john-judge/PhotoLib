import numpy as np
import os.path
import PySimpleGUI as sg
import matplotlib
import sys
from matplotlib.widgets import RectangleSelector
import matplotlib.figure as figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
import matplotlib.pyplot as plt
from tkinter import *
import tkinter as Tk
from webbrowser import open as open_browser

from pyPhoto21.frame import FrameViewer
from pyPhoto21.trace import TraceViewer
from pyPhoto21.layouts import *

from mpl_interactions import image_segmenter
from matplotlib.widgets import Slider

# import io


class GUI:

    def __init__(self, data, hardware, file, production_mode=True):
        matplotlib.use("TkAgg")
        sg.theme('DarkBlue12')
        self.data = data
        self.hardware = hardware
        self.file = file
        self.production_mode = production_mode
        self.fv = None  # FrameViewer object
        self.tv = None  # TraceViewer object
        self.layouts = Layouts(data)
        self.window = None

        # general state/settings
        self.title = "Photo21"
        self.event_mapping = None
        self.define_event_mapping()  # event callbacks used in event loops
        # kickoff workflow
        if self.production_mode:
            self.introduction()
        self.main_workflow()

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

    def main_workflow(self):
        right_col = self.layouts.create_right_column()
        left_col = self.layouts.create_left_column()
        toolbar_menu = self.layouts.create_menu()

        layout = [[toolbar_menu],
                  [sg.Column(left_col),
                   sg.VSeperator(),
                   sg.Column(right_col)]]

        self.window = sg.Window(self.title,
                                layout,
                                finalize=True,
                                element_justification='center',
                                resizable=True,
                                font='Helvetica 18')
        self.plot_frame()
        self.plot_trace()
        self.main_workflow_loop()
        self.window.close()

    def main_workflow_loop(self, history_debug=False):
        events = ''
        while True:
            event, values = self.window.read()
            if history_debug and event is not None:
                events += str(event) + '\n'
            if event == "Exit" or event == sg.WIN_CLOSED:
                break
            elif event not in self.event_mapping or self.event_mapping[event] is None:
                print("Not Implemented:", event)
            else:
                ev = self.event_mapping[event]
                if event in values:
                    ev['args']['values'] = values[event]
                ev['function'](**ev['args'])
        if history_debug:
            print("**** History of Events ****\n", events)

    def plot_trace(self):
        self.tv = TraceViewer(self.data)
        fig = self.tv.get_fig()

        self.draw_figure_w_toolbar(self.window['trace_canvas'].TKCanvas,
                                   fig,
                                   self.window['trace_canvas_controls'].TKCanvas)

    def plot_frame(self):

        self.fv = FrameViewer(self.data)
        fig = self.fv.get_fig()
        s_max = self.fv.get_slider_max()
        canvas_toolbar = self.window['frame_canvas_controls'].TKCanvas
        canvas = self.window['frame_canvas'].TKCanvas

        figure_canvas_agg = FigureCanvasTkAgg(fig, master=canvas)

        figure_canvas_agg.get_tk_widget().pack(fill="both", expand=True)
        # figure_canvas_agg.mpl_connect('scroll_event', self.fv.onscroll) # currently scroll not used.
        figure_canvas_agg.mpl_connect('button_release_event', self.fv.change_frame)
        figure_canvas_agg.mpl_connect('button_press_event', self.fv.onclick)
        toolbar = Toolbar(figure_canvas_agg, canvas_toolbar)
        toolbar.update()
        figure_canvas_agg.draw()
        s_max.on_changed(self.fv.change_frame)

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

    def record(self, **kwargs):
        if self.data.get_is_loaded_from_file():
            self.data.resize_image_memory()
        # TO DO: loop over trials similar to MainController::acqui
        self.hardware.record(images=self.data.get_acqui_memory(), fp_data=self.data.get_fp_data())
        self.fv.update_new_image()
        self.data.set_is_loaded_from_file(False)

    def take_rli(self, **kwargs):
        if self.data.get_is_loaded_from_file():
            self.data.resize_image_memory()
        self.hardware.take_rli(images=self.data.get_rli_memory())
        self.data.set_is_loaded_from_file(False)
        if self.fv.get_show_rli_flag():
            self.fv.update_new_image()

    def set_camera_program(self, **kwargs):
        program_name = kwargs['values']
        program_index = self.data.display_camera_programs.index(program_name)
        self.data.set_camera_program(program_index)

    def save_to_file(self):
        self.file.save_to_file(self.data.get_acqui_images(), self.data.get_rli_images())

    def launch_hyperslicer(self):
        self.fv.launch_hyperslicer()

    def toggle_show_rli(self, **kwargs):
        self.fv.set_show_rli_flag(kwargs['values'], update=True)

    def load_zda_file(self):
        file_window = sg.Window('File Browser',
                                self.layouts.create_file_browser(),
                                finalize=True,
                                element_justification='center',
                                resizable=True,
                                font='Helvetica 18')
        file = None
        # file browser event loop
        while True:
            event, values = file_window.read()
            if event == sg.WIN_CLOSED or event == "Exit":
                break
            elif event == "file_window.open":
                file = values["file_window.browse"]
                break
        file_window.close()
        self.data.clear_data_memory()
        print("Loading from file:", file)
        self.file.load_from_file(file)
        self.data.set_is_loaded_from_file(True)
        self.fv.update_new_image()

    @staticmethod
    def launch_github_page():
        open_browser('https://github.com/john-judge/PhotoLib', new=2)

    def set_digital_binning(self, **kwargs):
        binning = kwargs['values']
        if not binning.isnumeric():
            self.window['Digital Binning'].update('')
            return
        elif len(binning) > 3:
            binning = binning[:-1]
            self.window['Digital Binning'].update(binning)
        binning = int(binning)
        self.fv.set_digital_binning(binning)

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
                    'function': self.save_to_file,
                    'args': {}
                },
                'Launch Hyperslicer': {
                    'function': self.launch_hyperslicer,
                    'args': {},
                },
                "-CAMERA PROGRAM-": {
                    'function': self.set_camera_program,
                    'args': {},
                },
                "Show RLI": {
                    'function': self.toggle_show_rli,
                    'args': {},
                },
                "Open (.zda)": {
                    'function': self.load_zda_file,
                    'args': {},
                },
                '-github-': {
                    'function': self.launch_github_page,
                    'args': {},
                },
                'Digital Binning': {
                    'function': self.set_digital_binning,
                    'args': {},
                }
            }


class Toolbar(NavigationToolbar2Tk):
    def __init__(self, *args, **kwargs):
        super(Toolbar, self).__init__(*args, **kwargs)
