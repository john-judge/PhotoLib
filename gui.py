import numpy as np
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import PySimpleGUI as sg
import matplotlib
import os.path


class GUI:

    def __init__(self, data, hardware, file):
        matplotlib.use("TkAgg")
        sg.theme('DarkAmber')
        self.data = data
        self.hardware = hardware
        self.file = file
        self.title = "Photo21"
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
        return [
            # [sg.Text('Plot test')],
            [sg.Canvas(key='-CANVAS-')],
            [
                sg.Button("Take RLI"),
                sg.Button("Record"),
                sg.Button("Save"),
                # sg.In(size=(25, 1), enable_events=True, key="-FOLDER-"),
                # sg.FolderBrowse(),
            ],
        ]

    def create_right_column(self):
        return [
            [
                sg.Listbox(
                    values=[],
                    enable_events=True,
                    size=(40, 20),
                    key="-FILE LIST-"
                )
            ],
        ]

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

        # add the plot to the window
        fig = self.plot_frame()
        fig_canvas_agg = self.draw_figure(window['-CANVAS-'].TKCanvas, fig)

        self.main_workflow_loop(window)
        window.close()

    def main_workflow_loop(self, window, history=False):
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

    @staticmethod
    def plot_frame():
        fig = matplotlib.figure.Figure(figsize=(5, 4), dpi=100)
        t = np.arange(0, 3, .01)
        fig.add_subplot(111).plot(t, 2 * np.sin(2 * np.pi * t))
        return fig

