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
from pyPhoto21.analysis.roi import ROI
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
        self.auto_save_enabled = True
        self.schedule_rli_enabled = True
        self.tv = TraceViewer(self.data)
        self.fv = FrameViewer(self.data, self.tv)
        self.roi = ROI(self.data)
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
        self.plot_trace()
        self.plot_frame()
        self.main_workflow_loop()
        self.window.close()

    def main_workflow_loop(self, history_debug=False, window=None, exit_event="Exit"):
        if window is None:
            window = self.window
        events = ''
        while True:
            event, values = window.read()
            if history_debug and event is not None:
                events += str(event) + '\n'
            if event == exit_event or event == sg.WIN_CLOSED:
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
        fig = self.tv.get_fig()

        self.draw_figure_w_toolbar(self.window['trace_canvas'].TKCanvas,
                                   fig,
                                   self.window['trace_canvas_controls'].TKCanvas)

    def plot_frame(self):
        fig = self.fv.get_fig()
        s_max = self.fv.get_slider_max()
        # canvas_toolbar = self.window['frame_canvas_controls'].TKCanvas
        canvas = self.window['frame_canvas'].TKCanvas

        figure_canvas_agg = FigureCanvasTkAgg(fig, master=canvas)

        figure_canvas_agg.get_tk_widget().pack()  # fill="none", expand=False)
        # figure_canvas_agg.mpl_connect('scroll_event', self.fv.onscroll) # currently scroll not used.
        figure_canvas_agg.mpl_connect('button_release_event', self.fv.onrelease)
        figure_canvas_agg.mpl_connect('button_press_event', self.fv.onpress)
        figure_canvas_agg.mpl_connect('motion_notify_event', self.fv.onmove)
        # toolbar = Toolbar(figure_canvas_agg, canvas_toolbar)
        # toolbar.update()
        figure_canvas_agg.draw_idle()
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
        figure_canvas_agg.draw_idle()
        toolbar = Toolbar(figure_canvas_agg, canvas_toolbar)
        toolbar.update()
        figure_canvas_agg.get_tk_widget().pack(fill='none', expand=False)

    def record(self, **kwargs):
        if self.get_is_schedule_rli_enabled():
            self.take_rli()
        if self.data.get_is_loaded_from_file():
            self.data.resize_image_memory()
        # TO DO: loop over trials similar to MainController::acqui
        self.hardware.record(images=self.data.get_acqui_memory(), fp_data=self.data.get_fp_data())
        self.fv.update_new_image()
        self.data.set_is_loaded_from_file(False)
        if self.get_is_auto_save_enabled():
            # self.file.save_to_compressed_file()
            self.file.increment_run()

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

    def launch_hyperslicer(self):
        self.fv.launch_hyperslicer()

    def toggle_show_rli(self, **kwargs):
        self.fv.set_show_rli_flag(kwargs['values'], update=True)

    @staticmethod
    def notify_window(title, message):
        layout = [[sg.Column([
            [sg.Text(message)],
            [sg.Button("OK")]])]]
        wind = sg.Window(title, layout, finalize=True)
        while True:
            event, values = wind.read()
            # End intro when user closes window or
            # presses the OK button
            if event == "OK" or event == sg.WIN_CLOSED:
                break
        wind.close()

    def choose_save_dir(self):
        # Spawn a folder browser
        print("choose_save_dir not implemented")

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
                file_window.close()
                return
            elif event == "file_window.open":
                file = values["file_window.browse"]
                file_ext = file.split('.')
                if len(file_ext) > 0:
                    file_ext = file_ext[-1]
                else:
                    file_ext = ''
                if file_ext not in ['zda', 'pbz2']:
                    self.notify_window("File Type", "Unsupported file type.\nSelect .zda or .pbz2")
                else:
                    break
        file_window.close()
        self.data.clear_data_memory()
        print("Loading from file:", file, "\nThis will take a few seconds...")
        self.file.load_from_file(file)
        print("File Loaded.")
        self.fv.update_new_image()

    @staticmethod
    def launch_github_page():
        open_browser('https://github.com/john-judge/PhotoLib', new=2)

    # Returns True if string s is a valid numeric input
    @staticmethod
    def validate_numeric_input(s, non_zero=False, max_digits=None):
        return type(s) == str \
               and s.isnumeric() \
               and (max_digits is None or len(s) <= max_digits) \
               and (not non_zero or int(s) != 0)

    def set_digital_binning(self, **kwargs):
        binning = kwargs['values']
        if not self.validate_numeric_input(binning, non_zero=True):
            self.window['Digital Binning'].update('')
            return
        elif len(binning) > 3:
            binning = binning[:-1]
            self.window['Digital Binning'].update(binning)
        binning = int(binning)
        self.fv.set_digital_binning(binning)

    def get_is_auto_save_enabled(self):
        return self.auto_save_enabled

    def set_is_auto_save_enabled(self, value):
        self.auto_save_enabled = value

    def get_is_schedule_rli_enabled(self):
        return self.schedule_rli_enabled

    def set_is_schedule_rli_enabled(self, value):
        self.schedule_rli_enabled = value

    def set_light_on_onset(self, **kwargs):
        v = kwargs['values']
        self.data.set_light_on_onset(v)

    def set_light_on_duration(self, **kwargs):
        v = kwargs['values']
        self.data.set_light_on_duration(v)

    def set_acquisition_onset(self, **kwargs):
        v = kwargs['values']
        self.data.set_acquisition_onset(v)

    def set_acquisition_duration(self, **kwargs):
        v = kwargs['values']
        self.data.set_acquisition_duration(v)

    def set_stimulator_onset(self, **kwargs):
        v = kwargs['values']
        ch = kwargs['channel']
        self.data.set_stimulator_onset(v, ch)

    def set_stimulator_duration(self, **kwargs):
        v = kwargs['values']
        ch = kwargs['channel']
        self.data.set_stimulator_duration(v, ch)

    def validate_and_pass(self, **kwargs):
        fn_to_call = kwargs['call']
        v = kwargs['values']
        ch = kwargs['channel']
        if self.validate_numeric_input(v, max_digits=6):
            fn_to_call(value=int(v), channel=ch)
            print("called:", fn_to_call)

    def launch_roi_settings(self, **kwargs):
        w = sg.Window('ROI Identification Settings',
                      self.layouts.create_roi_settings_form(),
                      finalize=True,
                      element_justification='center',
                      resizable=True,
                      font='Helvetica 18')
        # roi settings event loop
        self.main_workflow_loop(window=w, exit_event="Exit ROI")
        w.close()

    def enable_roi_identification(self, **kwargs):
        if kwargs['values']:
            self.roi.enable_roi_identification()
        else:
            self.roi.disable_roi_identification()
        self.fv.update_new_image()

    def set_cutoff(self, **kwargs):
        self.roi.set_cutoff(kwargs['kind'],
                            kwargs['form'],
                            kwargs['values'])

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
                    'function': self.file.save_to_compressed_file,
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
                "Open": {
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
                },
                "Choose Save Directory": {
                    'function': self.choose_save_dir,
                    'args': {},
                },
                'Light On Onset': {
                    'function': self.set_light_on_onset,
                    'args': {},
                },
                'Light On Duration': {
                    'function': self.set_light_on_duration,
                    'args': {},
                },
                'Acquisition Onset': {
                    'function': self.set_acquisition_onset,
                    'args': {},
                },
                'Acquisition Duration': {
                    'function': self.set_acquisition_duration,
                    'args': {},
                },
                'Stimulator #1 Onset': {
                    'function': self.set_stimulator_onset,
                    'args': {'channel': 1},
                },
                'Stimulator #2 Onset': {
                    'function': self.set_stimulator_onset,
                    'args': {'channel': 2},
                },
                'num_pulses Stim #1': {
                    'function': self.validate_and_pass,
                    'args': {'channel': 1, 'call': self.data.hardware.set_num_pulses},
                },
                'num_pulses Stim #2': {
                    'function': self.validate_and_pass,
                    'args': {'channel': 2, 'call': self.data.hardware.set_num_pulses},
                },
                'int_pulses Stim #1': {
                    'function': self.validate_and_pass,
                    'args': {'channel': 1, 'call': self.data.hardware.set_int_pulses},
                },
                'int_pulses Stim #2': {
                    'function': self.validate_and_pass,
                    'args': {'channel': 2, 'call': self.data.hardware.set_int_pulses},
                },
                'num_bursts Stim #1': {
                    'function': self.validate_and_pass,
                    'args': {'channel': 1, 'call': self.data.hardware.set_num_bursts},
                },
                'num_bursts Stim #2': {
                    'function': self.validate_and_pass,
                    'args': {'channel': 2, 'call': self.data.hardware.set_num_bursts},
                },
                'int_bursts Stim #1': {
                    'function': self.validate_and_pass,
                    'args': {'channel': 1, 'call': self.data.hardware.set_int_bursts},
                },
                'int_bursts Stim #2': {
                    'function': self.validate_and_pass,
                    'args': {'channel': 2, 'call': self.data.hardware.set_int_bursts},
                },
                "ROI Identifier Config": {
                    'function': self.launch_roi_settings,
                    'args': {},
                },
                "Identify ROI": {
                    'function': self.enable_roi_identification,
                    'args': {}
                },
                'Pixel-wise SNR cutoff Value': {
                    'function': self.set_cutoff,
                    'args': {'form': 'value',
                             'kind': 'pixel'}
                },
                'Pixel-wise SNR cutoff Percentile': {
                    'function': self.set_cutoff,
                    'args': {'form': 'percentile',
                             'kind': 'pixel'}
                },
                'Cluster-wise SNR cutoff Value': {
                    'function': self.set_cutoff,
                    'args': {'form': 'value',
                             'kind': 'cluster'}
                },
                'Cluster-wise SNR cutoff Percentile': {
                    'function': self.set_cutoff,
                    'args': {'form': 'percentile',
                             'kind': 'cluster'}
                },
                'ROI-wise SNR cutoff Value': {
                    'function': self.set_cutoff,
                    'args': {'form': 'value',
                             'kind': 'roi_snr'}
                },
                'ROI-wise SNR cutoff Percentile': {
                    'function': self.set_cutoff,
                    'args': {'form': 'percentile',
                             'kind': 'roi_snr'}
                },
                'ROI-wise Amplitude cutoff Value': {
                    'function': self.set_cutoff,
                    'args': {'form': 'value',
                             'kind': 'roi_amplitude'}
                },
                'ROI-wise Amplitude cutoff Percentile': {
                    'function': self.set_cutoff,
                    'args': {'form': 'percentile',
                             'kind': 'roi_amplitude'}
                }
            }



class Toolbar(NavigationToolbar2Tk):
    def __init__(self, *args, **kwargs):
        super(Toolbar, self).__init__(*args, **kwargs)
