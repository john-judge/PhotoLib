import numpy as np
import os.path
import threading
import time
import string
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
from pyPhoto21.event_mapping import EventMapping

# from mpl_interactions import image_segmenter
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
        self.tv = TraceViewer(self.data)
        self.fv = FrameViewer(self.data, self.tv)
        self.roi = ROI(self.data)
        self.layouts = Layouts(data)
        self.window = None

        # general state/settings
        self.title = "Photo21"
        self.freeze_input = False  # whether to allow fields to be updated. Frozen during acquire (how about during file loaded?)
        self.event_mapping = None
        self.background_option_index = 0
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
        right_col = self.layouts.create_right_column(self)
        left_col = self.layouts.create_left_column(self)
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
        self.window.Maximize()
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
                if self.is_recording():
                    print("Cleaning up hardware before exiting. Waiting until safe to exit (or at most 8 seconds)...")

                    self.hardware.set_stop_flag(True)
                    timeout = 8
                    while self.hardware.get_stop_flag() and timeout > 0:
                        time.sleep(1)
                        timeout -= 1
                        print(timeout, "seconds")
                break
            elif event not in self.event_mapping or self.event_mapping[event] is None:
                print("Not Implemented:", event)
            else:
                ev = self.event_mapping[event]
                if event in values:
                    ev['args']['window'] = window
                    ev['args']['values'] = values[event]
                    ev['args']['event'] = event
                ev['function'](**ev['args'])
        if history_debug:
            print("**** History of Events ****\n", events)

    def is_recording(self):
        return self.freeze_input and not self.data.get_is_loaded_from_file()

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

        figure_canvas_agg.get_tk_widget().pack(fill="none", expand=True)
        # figure_canvas_agg.mpl_connect('scroll_event', self.fv.onscroll) # currently scroll not used.
        figure_canvas_agg.mpl_connect('button_release_event', self.fv.onrelease)
        figure_canvas_agg.mpl_connect('button_press_event', self.fv.onpress)
        figure_canvas_agg.mpl_connect('motion_notify_event', self.fv.onmove)
        figure_canvas_agg.draw_idle()
        s_max.on_changed(self.fv.change_frame)

    # include a matplotlib figure in a Tkinter canvas
    def draw_figure_w_toolbar(self, canvas, fig, canvas_toolbar):
        if canvas.children:
            for child in canvas.winfo_children():
                child.destroy()
        if canvas_toolbar.children:
            for child in canvas_toolbar.winfo_children():
                child.destroy()
        figure_canvas_agg = FigureCanvasTkAgg(fig, master=canvas)
        figure_canvas_agg.mpl_connect('button_press_event', self.tv.onpress)
        figure_canvas_agg.draw_idle()
        toolbar = Toolbar(figure_canvas_agg, canvas_toolbar)
        toolbar.update()
        figure_canvas_agg.get_tk_widget().pack(fill='none', expand=True)

    def freeze_hardware_settings(self, v=True, include_buttons=True):
        if type(v) == bool:
            self.freeze_input = v
            events_to_control = self.layouts.list_hardware_settings()
            if include_buttons:
                events_to_control += self.layouts.list_hardware_events()
                events_to_control += self.layouts.list_file_events()
            for ev in events_to_control:
                self.window[ev].update(disabled=v)

    def unfreeze_hardware_settings(self):
        self.freeze_hardware_settings(v=False, include_buttons=True)

    def get_trial_sleep_time(self):
        sleep_sec = self.data.get_int_trials()
        if self.get_is_schedule_rli_enabled():
            sleep_sec = max(0, sleep_sec - .12)  # attempt to shorten by 120 ms, rough lower bound on time to take RLI
        return sleep_sec

    def get_record_sleep_time(self):
        sleep_sec = self.data.get_int_records()
        return max(0, sleep_sec - self.get_trial_sleep_time())

    def save_file_in_background(self):
        self.file.save_to_compressed_file()
        self.update_tracking_num_fields()

    # returns True if stop flag is set
    def sleep_and_check_stop_flag(self, sleep_time, interval=1):
        elapsed = 0
        while elapsed < sleep_time:
            time.sleep(interval)
            elapsed += interval
            if self.hardware.get_stop_flag():
                self.hardware.set_stop_flag(False)
                return True
        return False

    def record_in_background(self):
        self.freeze_hardware_settings()
        self.hardware.set_stop_flag(False)

        sleep_trial = self.get_trial_sleep_time()
        sleep_record = self.get_record_sleep_time()

        # resolve any hardware / file conflicting control of settings
        if self.data.get_is_loaded_from_file():
            self.data.sync_settings_from_hardware()
            self.data.resize_image_memory()
        self.data.set_is_loaded_from_file(False)
        exit_recording = False

        if self.data.get_num_records() * self.data.get_num_trials() * self.data.get_num_pts() == 0:
            print("Settings are such that no trials or points are recorded. Ending recording session.")
            return

        # Note that record index may not necessarily match the record num for file saving
        for record_index in range(self.data.get_num_records()):
            is_last_record = (record_index == self.data.get_num_records() - 1)
            if exit_recording:
                break

            for trial in range(self.data.get_num_trials()):
                self.data.set_current_trial_index(trial)
                self.update_tracking_num_fields()
                is_last_trial = (trial == self.data.get_num_trials() - 1)
                if self.get_is_schedule_rli_enabled():
                    self.take_rli_core()
                self.hardware.record(images=self.data.get_acqui_memory(),
                                     fp_data=self.data.get_fp_data())
                self.data.set_current_trial_index(trial)
                self.fv.update_new_image()
                print("\tTook trial", trial + 1, "of", self.data.get_num_trials())
                if not is_last_trial:
                    print("\t\t", sleep_trial, "seconds until next trial...")
                    exit_recording = self.sleep_and_check_stop_flag(sleep_time=sleep_trial)
                if exit_recording:
                    break

            if self.get_is_auto_save_enabled():
                threading.Thread(target=self.save_file_in_background, args=(), daemon=True).start()
            if exit_recording:
                break
            print("Took recording set", record_index + 1, "of", self.data.get_num_records())
            if not is_last_record:
                print("\t", sleep_record, "seconds until next recording set...")
                exit_recording = self.sleep_and_check_stop_flag(sleep_time=sleep_record)

        print("Recording ended.")
        # done recording
        self.unfreeze_hardware_settings()

    def record(self, **kwargs):
        # we spawn a new thread to acquire in the background.
        # meanwhile the original thread returns and keeps handling GUI clicks
        # but updates to Data/Hardware fields will be frozen
        #self.record_in_background()
        threading.Thread(target=self.record_in_background, args=(), daemon=True).start()

    ''' RLI Controller functions '''
    def take_rli_core(self):
        self.hardware.take_rli(images=self.data.get_rli_memory())
        self.data.set_is_loaded_from_file(False)
        if self.fv.get_show_rli_flag():
            self.fv.update_new_image()

    def take_rli_in_background(self):
        self.freeze_hardware_settings()
        self.hardware.set_stop_flag(False)
        if self.data.get_is_loaded_from_file():
            self.data.sync_settings_from_hardware()
            self.data.resize_image_memory()
        self.take_rli_core()
        self.unfreeze_hardware_settings()

    def take_rli(self, **kwargs):
        self.data.get_current_trial_index()
        threading.Thread(target=self.take_rli_in_background, args=(), daemon=True).start()

    ''' Live Feed Controller functions '''
    def start_livefeed(self, **kwargs):
        if self.data.get_is_livefeed_enabled():
            return
        self.window["Live Feed"].update(button_color=('black', 'yellow'))
        self.freeze_hardware_settings()
        self.data.set_is_livefeed_enabled(True)
        lf_frame = self.data.get_livefeed_frame()
        if not self.hardware.start_livefeed(lf_frame):  # C++ DLL will keep pointer to lf_frame
            # Hardware not enabled
            self.stop_livefeed()
            return
        self.fv.update_new_image()

        # launch live feed daemon: plotter
        threading.Thread(target=self.continue_livefeed_in_background, args=(lf_frame,), daemon=True).start()

        # launch acqui daemon
        threading.Thread(target=self.hardware.continue_livefeed, args=(), daemon=True).start()

    # a continuous loop in background
    def continue_livefeed_in_background(self, lf_frame, fps=30):
        if not self.data.get_is_livefeed_enabled():
            return
        interval = 1.0 / float(fps)
        lf_img = self.fv.livefeed_im
        fig = self.fv.get_fig()
        # C++ DLL has stored pointer to lf_frame, no need to keep passing
        while not self.hardware.get_stop_flag():  # the GUI flag
            timeout = 80.0
            if self.hardware.get_livefeed_produced_image_flag():
                print("plotter got image!", np.std(lf_frame))
                lf_img.set_data(lf_frame[0, :, :])
                fig.canvas.draw_idle()

                # Allows DLL to continue to next image
                self.hardware.clear_livefeed_produced_image_flag()
                time.sleep(interval)

        # stop flag read -- notify hardware
        self.stop_livefeed()
        print("Live Feed daemon exiting.")

    def stop_livefeed(self):
        self.window["Live Feed"].update(button_color=('black', 'gray'))
        self.hardware.stop_livefeed()  # clean up
        self.data.set_is_livefeed_enabled(False)
        self.unfreeze_hardware_settings()
        self.hardware.set_stop_flag(False)  # take down flag to signal to GUI daemon that we've cleaned up
        self.fv.update_new_image()

    def set_camera_program(self, **kwargs):
        program_name = kwargs['values']
        program_index = self.data.display_camera_programs.index(program_name)
        self.data.set_camera_program(program_index)
        self.window["Acquisition Duration"].update(self.data.get_acqui_duration())

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

    def change_save_filename(self, **kwargs):
        # Spawn a folder browser for auto-save
        v = kwargs['values']
        if len(v) < 1:
            return
        valid_chars = "-_.() %s%s" % (string.ascii_letters, string.digits)
        v = ''.join(c for c in v if c in valid_chars)
        kwargs['window'][kwargs['event']].update(v)
        self.file.set_override_filename(v)

    def choose_save_dir(self, **kwargs):
        folder = self.browse_for_folder()
        self.file.set_save_dir(folder)
        print("New save location:", folder)

    def browse_for_file(self, file_extensions):
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
                if file_ext not in file_extensions:
                    supported_file_str = " ".join(file_extensions)
                    self.notify_window("File Type",
                                       "Unsupported file type.\nSupported: " + supported_file_str)
                else:
                    break
        if self.freeze_input:
            file = None
            self.notify_window("File Input Error",
                               "Cannot load file during acquisition")
        file_window.close()
        return file

    def browse_for_folder(self):
        folder_window = sg.Window('File Browser',
                                  self.layouts.create_folder_browser(),
                                  finalize=True,
                                  element_justification='center',
                                  resizable=True,
                                  font='Helvetica 18')
        folder = None
        # file browser event loop
        while True:
            event, values = folder_window.read()
            if event == sg.WIN_CLOSED or event == "Exit":
                folder_window.close()
                return
            elif event == "folder_window.open":
                folder = values["folder_window.browse"]
                break
        if self.is_recording():
            folder = self.file.get_save_dir()
            self.notify_window("Warning",
                               "You are changing the save location during acquisition." +
                               "I don't recommend scattering your files. " +
                               "Keeping this save directory:\n" +
                               folder)
        folder_window.close()
        return folder

    def load_roi_file(self, **kwargs):
        filename = self.browse_for_file(['roi'])
        data_obj = self.file.retrieve_python_object_from_pickle(filename)
        self.roi.load_roi_data(data_obj)

    def save_roi_file(self, **kwargs):
        data_obj, filename = self.roi.dump_roi_data()
        self.file.dump_python_object_to_pickle(filename,
                                               data_obj,
                                               extension='roi')

    def load_data_file_in_background(self, file):
        self.file.load_from_file(file)
        # Sync GUI
        self.file_gui_fields_sync()
        # Freeze input fields to hardware

        print("File Loaded.")
        self.fv.update_new_image()

    def load_data_file(self):
        file = self.browse_for_file(['zda', 'pbz2'])
        if file is not None:
            self.freeze_hardware_settings(include_buttons=False)
            print("Loading from file:", file, "\nThis will take a few seconds...")

            threading.Thread(target=self.load_data_file_in_background, args=(file,), daemon=True).start()

    # Pull all file-based data from Data and sync GUI fields
    def file_gui_fields_sync(self):
        w = self.window
        w['Number of Points'].update(self.data.get_num_pts())
        w['int_trials'].update(self.data.get_int_trials())
        w['num_trials'].update(self.data.get_num_trials())
        w['Acquisition Onset'].update(self.data.get_acqui_onset())
        w['Acquisition Duration'].update(self.data.get_acqui_duration())
        w['Stimulator #1 Onset'].update(self.data.get_stim_onset(1))
        w['Stimulator #2 Onset'].update(self.data.get_stim_onset(2))
        w['Stimulator #1 Duration'].update(self.data.get_stim_duration(1))
        w['Stimulator #2 Duration'].update(self.data.get_stim_duration(2))
        self.update_tracking_num_fields()

    # disable file-viewing mode, allowing acquisition to resume
    def unload_file(self):
        if self.data.get_is_loaded_from_file():
            self.unfreeze_hardware_settings()
            self.data.set_is_loaded_from_file(value=False)
            self.data.clear_data_memory()
            self.fv.update_new_image()

    @staticmethod
    def launch_github_page():
        open_browser('https://github.com/john-judge/PhotoLib', new=2)

    # Returns True if string s is a valid numeric input
    @staticmethod
    def validate_numeric_input(s, non_zero=False, max_digits=None, min_val=None, max_val=None, decimal=False):
        if decimal:  # decimals: allow removing at most one decimal anywhere
            if len(s) > 0 and s[0] == '.':
                s = s[1:]
            elif len(s) > 0 and s[-1] == '.':
                s = s[:-1]
            elif '.' in s:
                s = s.replace('.', '')
        return type(s) == str \
               and s.isnumeric() \
               and (max_digits is None or len(s) <= max_digits) \
               and (not non_zero or int(s) != 0) \
               and (min_val is None or int(s) >= min_val) \
               and (max_val is None or int(s) <= max_val)

    def set_digital_binning(self, **kwargs):
        binning = kwargs['values']
        while len(binning) > 0 and \
                (not self.validate_numeric_input(binning, max_val=min(self.data.get_display_width(),
                                                                      self.data.get_display_height()) // 4) \
                 or len(binning) > 3):
            binning = binning[:-1]
        if not self.validate_numeric_input(binning, non_zero=True):
            self.window['Digital Binning'].update('')
            return
        else:
            self.window['Digital Binning'].update(binning)
        binning = int(binning)
        self.fv.set_digital_binning(binning)

    def get_is_auto_save_enabled(self):
        return self.data.get_is_auto_save_enabled()

    def set_is_auto_save_enabled(self, value):
        self.data.set_is_auto_save_enabled(value)

    def get_is_schedule_rli_enabled(self):
        return self.data.get_is_schedule_rli_enabled()

    def set_is_schedule_rli_enabled(self, value):
        self.data.set_is_schedule_rli_enabled(value)

    def set_acqui_onset(self, **kwargs):
        v = kwargs['values']
        while len(v) > 0 and not self.validate_numeric_input(v, decimal=True, max_val=15000):
            v = v[:-1]
        if self.validate_numeric_input(v, decimal=True, max_val=15000):
            num_frames = float(v) // self.data.get_int_pts()
            self.hardware.set_acqui_onset(acqui_onset=num_frames)

    def set_num_pts(self, **kwargs):
        v = kwargs['values']

        while len(v) > 0 and not self.validate_numeric_input(v, decimal=True, max_val=15000):
            v = v[:-1]
        if len(v) > 0 and self.validate_numeric_input(v, decimal=True, max_val=15000):
            acqui_duration = float(v) * self.data.get_int_pts()
            self.data.set_num_pts(value=int(v))  # Data method resizes data
            kwargs['window'][kwargs['event']].update(v)
            kwargs['window']["Acquisition Duration"].update(str(acqui_duration))
        else:
            self.data.set_num_pts(value=0)  # Data method resizes data
            kwargs['window'][kwargs['event']].update('')
            kwargs['window']["Acquisition Duration"].update('')

    def set_acqui_duration(self, **kwargs):
        v = kwargs['values']

        # looks at num_pts as well to validate.
        def is_valid_acqui_duration(u, max_num_pts=15000):
            return self.validate_numeric_input(u, decimal=True) \
                   and int(float(u) * self.data.get_int_pts()) <= max_num_pts

        while len(v) > 0 and not is_valid_acqui_duration(v):
            v = v[:-1]
        if len(v) > 0 and is_valid_acqui_duration(v):
            num_pts = int(float(v) // self.data.get_int_pts())
            self.data.set_num_pts(value=num_pts)
            kwargs['window'][kwargs['event']].update(v)
            kwargs['window']["Number of Points"].update(str(num_pts))
        else:
            self.data.set_num_pts(value=0)
            kwargs['window'][kwargs['event']].update('')
            kwargs['window']["Number of Points"].update('')

    @staticmethod
    def pass_no_arg_calls(**kwargs):
        for key in kwargs:
            if key.startswith('call'):
                kwargs[key]()

    def validate_and_pass_int(self, **kwargs):
        max_val = None
        if 'max_val' in kwargs:
            max_val = kwargs['max_val']
        fn_to_call = kwargs['call']
        v = kwargs['values']
        window = kwargs['window']
        while len(v) > 0 and not self.validate_numeric_input(v, max_digits=5, max_val=max_val):
            v = v[:-1]
        if len(v) > 0 and self.validate_numeric_input(v, max_digits=5, max_val=max_val):
            fn_to_call(value=int(v))
            window[kwargs['event']].update(v)
            print("called:", fn_to_call)
            if 'call2' in kwargs:
                kwargs['call2'](value=int(v))
                print("called:", kwargs['call2'])
        else:
            fn_to_call(value=None)
            window[kwargs['event']].update('')

    # for passing to channel-based setters
    def validate_and_pass_channel(self, **kwargs):
        fn_to_call = kwargs['call']
        v = kwargs['values']
        ch = kwargs['channel']
        window = kwargs['window']
        while len(v) > 0 and not self.validate_numeric_input(v, max_digits=6):
            v = v[:-1]
        if len(v) > 0 and self.validate_numeric_input(v, max_digits=6):
            fn_to_call(value=int(v), channel=ch)
            window[kwargs['event']].update(v)
            print("called:", fn_to_call)
        else:
            fn_to_call(value=0, channel=ch)
            window[kwargs['event']].update('')

    def launch_roi_settings(self, **kwargs):
        w = sg.Window('ROI Identification Settings',
                      self.layouts.create_roi_settings_form(self),
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
        form = kwargs['form']
        v = kwargs['values']
        window = kwargs['window']
        max_val = None
        min_val = None
        if form == 'percentile':
            max_val = 100.0
            min_val = 0.0
        while len(v) > 0 and not self.validate_numeric_input(v, min_val=min_val, max_val=max_val, decimal=True):
            v = v[:-1]

        if len(v) > 0 and form == 'percentile' and float(v) > max_val:
            v = str(max_val)

        partner_field = None
        partner_form = None
        if form == 'Raw':
            partner_form = 'Percentile'
            partner_field = kwargs['event'].replace('Raw', 'Percentile')
        else:
            partner_form = 'Raw'
            partner_field = kwargs['event'].replace('Percentile', 'Raw')

        self.roi.set_cutoff(kwargs['kind'],
                            form,
                            v)
        partner_v = self.roi.get_cutoff(kwargs['kind'], partner_form)
        window[partner_field].update(str(partner_v))
        if len(v) == 0 or float(v) != kwargs['values']:
            window[kwargs['event']].update(v)

    def toggle_auto_save(self, **kwargs):
        self.set_is_auto_save_enabled(kwargs['values'])

    def toggle_auto_rli(self, **kwargs):
        self.set_is_schedule_rli_enabled(kwargs['values'])

    # generic setter that links together 2 ms / frame linked fields
    def set_time_window_generic(self, setter_function, arg_dict):
        v = arg_dict['values']
        form = arg_dict['form']
        kind = arg_dict['kind']
        index = arg_dict['index']
        partner_field = None
        partner_v = None
        if form == 'ms':
            partner_field = arg_dict['event'].replace('(ms)', 'frames')
        else:
            partner_field = arg_dict['event'].replace('frames', '(ms)')

        # if possible, trim input of invalid characters
        while len(v) > 0 and not self.validate_numeric_input(v):
            v = v[:-1]

        if self.validate_numeric_input(v):
            if form == 'ms':
                v = float(v)
                partner_v = int(v / self.data.get_int_pts())
            else:
                v = int(v)
                partner_v = float(v * self.data.get_int_pts())

            self.window[partner_field].update(str(partner_v)[:6])
            setter_function(kind, index, v)
        else:
            setter_function(kind, index, None)
            self.window[partner_field].update('')
            self.window[arg_dict['event']].update('')

    def set_baseline_skip_window(self, **kwargs):
        self.set_time_window_generic(self.data.core.set_baseline_skip_window, kwargs)

    def set_roi_time_window(self, **kwargs):
        self.set_time_window_generic(self.roi.set_time_window, kwargs)

    def select_baseline_skip_window(self):
        print("select_baseline_skip_window not implemented")

    def select_time_window_workflow(self):
        pass

    def set_roi_k_clusters(self, **kwargs):
        k = kwargs['values']
        while len(k) > 0 and not self.validate_numeric_input(k,
                                                             non_zero=True,
                                                             max_digits=3,
                                                             min_val=0,
                                                             max_val=None,
                                                             decimal=False):
            k = k[:-1]
        if kwargs['values'] != k:
            kwargs['window'][kwargs['event']].update(k)
        if len(k) == 0:
            k = None
        else:
            k = int(k)
        self.roi.set_k_clusters(k)

    def view_roi_plot(self, **kwargs):
        plot_type = kwargs['type']
        self.roi.launch_cluster_score_plot(plot_type)

    def set_num_trials(self, **kwargs):
        v = kwargs['values']
        self.data.set_num_trials(int(v))

    def define_event_mapping(self):
        if self.event_mapping is None:
            self.event_mapping = EventMapping(self).get_event_mapping()

    def update_tracking_num_fields(self, **kwargs):
        self.window["Slice Number"].update(self.file.get_slice_num())
        self.window["Location Number"].update(self.file.get_location_num())
        self.window["Record Number"].update(self.file.get_record_num())
        self.window["Trial Number"].update(self.data.get_current_trial_index())
        self.window["File Name"].update(self.file.get_filename(no_path=True))

    def set_current_trial_index(self, **kwargs):
        if 'value' in kwargs:
            value = int(kwargs['value'])
            self.data.set_current_trial_index(value)
            self.fv.update_new_image()

    def set_slice(self, **kwargs):
        value = int(kwargs['value'])
        self.file.set_slice(value)
        self.fv.update_new_image()

    def set_record(self, **kwargs):
        value = int(kwargs['value'])
        self.file.set_record(value)
        self.fv.update_new_image()

    def set_location(self, **kwargs):
        value = int(kwargs['value'])
        self.file.set_location(value)
        self.fv.update_new_image()

    def get_background_option_index(self):
        return self.background_option_index

    def set_background_option_index(self, **kwargs):
        bg_name = kwargs['values']
        bg_index = self.layouts.get_background_options().index(bg_name)
        self.background_option_index = bg_index

    # value to display in trace viewer
    def get_display_value_option_index(self):
        return self.data.get_display_value_option_index()

    # value to display in trace viewer
    def set_display_value_option_index(self, **kwargs):
        name = kwargs['values']
        ind = self.tv.get_display_value_options().index(name)
        self.data.set_display_value_option_index(ind)

    def set_temporal_filter_index(self, **kwargs):
        tf_name = kwargs['values']
        tf_index = self.data.core.get_temporal_filter_options().index(tf_name)
        self.data.core.set_temporal_filter_index(tf_index)
        self.tv.update_new_traces()

    def set_t_filter_radius(self, **kwargs):
        v = int(kwargs['values'])
        self.data.core.set_temporal_filter_radius(v)
        self.tv.update_new_traces()

    def set_s_filter_sigma(self, **kwargs):
        v = float(kwargs['values'])
        self.data.core.set_spatial_filter_sigma(v)
        self.fv.update_new_image()

    def set_is_t_filter_enabled(self, **kwargs):
        v = bool(kwargs['values'])
        self.data.core.set_is_temporal_filter_enabled(v)
        self.tv.update_new_traces()

    def set_is_s_filter_enabled(self, **kwargs):
        v = bool(kwargs['values'])
        self.data.core.set_is_spatial_filter_enabled(v)
        self.fv.update_new_image()

    def set_baseline_correction(self, **kwargs):
        v = kwargs['values']
        v = self.data.core.get_baseline_correction_options().index(v)
        self.data.core.set_baseline_correction_type_index(v)
        self.tv.update_new_traces()
        self.fv.update_new_image()

    def set_rli_division(self, **kwargs):
        v = bool(kwargs['values'])
        self.data.set_is_rli_division_enabled(v)
        self.tv.update_new_traces()
        self.fv.update_new_image()

    def set_data_inverse(self, **kwargs):
        v = bool(kwargs['values'])
        self.data.set_is_data_inverse_enabled(v)
        self.tv.update_new_traces()
        self.fv.update_new_image()


class Toolbar(NavigationToolbar2Tk):
    def __init__(self, *args, **kwargs):
        super(Toolbar, self).__init__(*args, **kwargs)
