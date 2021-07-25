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

from mpl_interactions import image_segmenter
from matplotlib.widgets import Slider

# import io


class GUI:

    def __init__(self, data, hardware, file):
        matplotlib.use("TkAgg")
        sg.theme('DarkBlue12')
        self.data = data
        self.hardware = hardware
        self.file = file
        self.fv = None  # FrameViewer object
        self.tv = None  # TraceViewer object
        self.window = None

        # general state/settings
        self.title = "Photo21"
        self.event_mapping = None
        self.define_event_mapping()  # event callbacks used in event loops
        # kickoff workflow
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

    def create_left_column(self):
        camera_programs = self.data.display_camera_programs
        acquisition_layout = [
            [sg.Checkbox('Show RLI', default=True, enable_events=True, key="Show RLI")],
            [sg.Button("STOP!", button_color=('black', 'yellow')),
             sg.Button("Take RLI", button_color=('brown', 'gray'))],
            [sg.Button("Live Feed", button_color=('black', 'gray')),
             sg.Button("Record", button_color=('black', 'red'))],
            [sg.Button("Save Processed Data", button_color=('black', 'green')),
             sg.Button("Save", button_color=('black', 'green'))],
            [sg.Combo(camera_programs,
                      enable_events=True,
                      default_value=camera_programs[self.hardware.get_camera_program()],
                      key="-CAMERA PROGRAM-")]
        ]

        analysis_layout = [[
            sg.Button("Launch Hyperslicer", button_color=('gray', 'blue')),
            sg.Button("Record", button_color=('gray', 'red')),
            sg.Button("Save", button_color=('gray', 'green')),
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

    @staticmethod
    def create_right_column():
        file_list_layout = [[
            sg.Listbox(
                values=[],
                enable_events=True,
                size=(20, 10),
                key="-FILE LIST-"
            ),
            sg.In(size=(25, 1), enable_events=True, key="-FOLDER-"),
            sg.FolderBrowse(),
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
        return trace_viewer_layout  # + file_list_layout

    def main_workflow(self):
        right_col = self.create_right_column()
        left_col = self.create_left_column()

        layout = [[
            sg.Column(left_col),
            sg.VSeperator(),
            sg.Column(right_col)]]

        self.window = sg.Window(self.title,
                                layout,
                                finalize=True,
                                element_justification='center',
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
        self.hardware.record(images=self.data.get_acqui_memory(), fp_data=self.data.get_fp_data())
        self.fv.update_new_image()
        print(self.data.get_fp_data())

    def take_rli(self, **kwargs):
        self.hardware.take_rli(images=self.data.get_rli_memory())
        if self.fv.get_show_rli_flag():
            self.fv.update_new_image()

    def set_camera_program(self, **kwargs):
        program_name = kwargs['values']
        program_index = self.data.display_camera_programs.index(program_name)
        self.data.set_camera_program(program_index)

    def save_to_file(self):
        self.file.save_to_file(self.get_acqui_images(), self.get_rli_images())

    def launch_hyperslicer(self):
        self.fv.launch_hyperslicer()

    def toggle_show_rli(self, **kwargs):
        self.fv.set_show_rli_flag(kwargs['values'], update=True)

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
            }


class Toolbar(NavigationToolbar2Tk):
    def __init__(self, *args, **kwargs):
        super(Toolbar, self).__init__(*args, **kwargs)
