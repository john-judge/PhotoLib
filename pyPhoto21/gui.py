import numpy as np
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import PySimpleGUI as sg
import matplotlib
from matplotlib.widgets import RectangleSelector
import matplotlib.figure as figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
import matplotlib.pyplot as plt

from mpl_interactions import image_segmenter
import os.path

global_state = {
    'redraw': {'frame_canvas': True,
               'trace_canvas': True}
}

class GUI:

    def __init__(self, data, hardware, file):
        matplotlib.use("TkAgg")
        sg.theme('DarkBlue12')
        self.data = data
        self.hardware = hardware
        self.file = file

        # general state/settings
        self.title = "Photo21"

        # kickoff workflow
        self.introduction()
        self.main_workflow()

    def introduction(self):
        layout = [
            [
                sg.Column([[sg.Image(key="-IMAGE-")]]),
                [sg.Text("Welcome to Photo21! \n\tCheck that your camera and \n\tNI-USB are turned on.")],
                [sg.Button("OK")]
            ]
        ]
        window = sg.Window(self.title, layout, finalize=True)
        self.intro_event_loop(window)
        window.close()

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
            # sg.In(size=(25, 1), enable_events=True, key="-FOLDER-"),
            # sg.FolderBrowse(),
        ]
        analysis_layout = [[
            sg.Button("Take RLI", button_color=('gray', 'brown')),
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
                               size=(300 * 2, 600)
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
                               size=(300 * 2, 600)
                               )]
                ],
                background_color='#DAE0E6',
                pad=(0, 0))]]
        return trace_viewer_layout + file_list_layout

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

        window = sg.Window(self.title,
                           layout,
                           finalize=True,
                           element_justification='center',
                           font='Helvetica 18')
        self.plot_update(window, 'frame_canvas')
        self.plot_update(window, 'trace_canvas')
        self.main_workflow_loop(window)
        window.close()

    def main_workflow_loop(self, window, history=False):
        global global_state
        event_mapping = {
            'Record': {
                'function': self.hardware.record,
                'args': {'images': self.data.get_acqui_images()} # Data...
            },
            'Take RLI': {
                'function': self.hardware.take_rli,
                'args': {'images': self.data.get_rli_images()}
            },
            'Save': {
                'function': self.file.save_to_file,
                'args': {'images': self.data.get_acqui_images()}
            },
        }
        events = ''
        while True:
            event, values = window.read()
            if history and event is not None:
                events += str(event) + '\n'
            if event == "Exit" or event == sg.WIN_CLOSED:
                break
            if global_state['redraw']['frame_canvas']:
                self.plot_update(window, 'frame_canvas')
            if global_state['redraw']['trace_canvas']:
                print("Redraw Trace cv!")
                self.plot_update(window, 'trace_canvas')
            elif event not in event_mapping or event_mapping[event] is None:
                print("Not Implemented:", event)
            else:
                try:
                    ev = event_mapping[event]
                    ev['function'](**ev['args'])
                except Exception as e:
                    print("exception while calling", event_mapping[event])
                    print(str(e))

        if history:
            print(events)

    @staticmethod
    def draw_figure(canvas, figure):
        figure_canvas_agg = FigureCanvasTkAgg(figure, canvas)
        figure_canvas_agg.draw()
        figure_canvas_agg.get_tk_widget().pack(side="top", fill="both", expand=1)
        return figure_canvas_agg

    def plot_update(self, window, name):
        fig = figure.Figure()
        ax = fig.add_subplot(111)

        if name == 'trace_canvas':
            t = np.arange(0, 3, .01)
            fig.add_subplot(111).plot(t, 2 * np.sin(2 * np.pi * t))
        elif name == 'frame_canvas':
            # Image selection / segmentation widget with mpl
            image = self.get_display_frame()
            # https://mpl-interactions.readthedocs.io/en/stable/examples/image-segmentation.html
            segmenter = image_segmenter(image, mask_colors="red", mask_alpha=0.76, figsize=(7, 7))
            plt.imshow(segmenter)
            return

        dpi = fig.get_dpi()
        fig.set_size_inches(505 * 2 / float(dpi), 707 / float(dpi))

        x = np.linspace(0, 2 * np.pi)
        y = np.sin(x)
        line, = ax.plot(x, y)

        def line_select_callback(eclick, erelease):
            x1, y1 = eclick.xdata, eclick.ydata
            x2, y2 = erelease.xdata, erelease.ydata

            global global_state
            print("...")
            # clicking on plot should also update trace viewer
            if name == 'frame_canvas':
                global_state['redraw']['trace_canvas'] = True
                global_state['redraw']['trace_canvas']['click'] = [x1, y1]
                global_state['redraw']['trace_canvas']['release'] = [x2, y2]

            rect = plt.Rectangle((min(x1, x2), min(y1, y2)), np.abs(x1 - x2), np.abs(y1 - y2))
            ax.add_patch(rect)
            fig.canvas.draw()

        rs = RectangleSelector(ax, line_select_callback,
                               drawtype='box', useblit=False, button=[1],
                               minspanx=5, minspany=5, spancoords='pixels',
                               interactive=True)
        self.draw_figure_w_toolbar(window[name].TKCanvas,
                                   fig,
                                   window[name+'_controls'].TKCanvas)
        global_state['redraw'][name] = False

    # update system state so that frame is redrawn in event loop
    def redraw_frame(self):
        global_state['redraw']['frame_canvas'] = True
        # .. choose frame: select, average, retrieve from self.data ...
        raise NotImplementedError

    # Based on system state, create/get the frame that should be displayed.
    def get_display_frame(self):
        # Temporary. Enhancements to come.
        return np.average(self.data.get_rli_images(), axis=0)

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
        figure_canvas_agg.get_tk_widget().pack(side='right', fill='both', expand=1)


class Toolbar(NavigationToolbar2Tk):
    def __init__(self, *args, **kwargs):
        super(Toolbar, self).__init__(*args, **kwargs)

