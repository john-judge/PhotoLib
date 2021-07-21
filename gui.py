import numpy as np
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import PySimpleGUI as sg
import matplotlib
from wrapper import PhotoLibDriver
import os.path

class GUI:

    def __init__(self):
        matplotlib.use("TkAgg")
        self.title = "Photo21"
        self.introduction()
        self.main_workflow()

    def introduction(self):

        #file_list_column = [
            #[
            #    sg.Text("Image Folder"),
            #    sg.In(size=(25, 1), enable_events=True, key="-FOLDER-"),
            #    sg.FolderBrowse(),
            #],
            # [
            #     sg.Listbox(
            #         values=[], enable_events=True, size=(40, 20),
            #         key="-FILE LIST-"
            #     )
            # ],
        #]
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

    def intro_event_loop(self, window, filename='art/meyer.png'):
        window["-IMAGE-"].update(filename=filename)
        while True:
            event, values = window.read()
            # End intro when user closes window or
            # presses the OK button

            if event == "OK" or event == sg.WIN_CLOSED:
                break

    def main_workflow(self):
        trace_viewer_column = [
            [
                sg.Text("Image Folder"),
                sg.In(size=(25, 1), enable_events=True, key="-FOLDER-"),
                sg.FolderBrowse(),
            ],
            [
                sg.Listbox(
                    values=[], enable_events=True, size=(40, 20), key="-FILE LIST-"
                )
            ],
        ]

        # For now will only show the name of the file that was chosen
        frame_viewer_column = [
             [sg.Text('Plot test')],
             [sg.Canvas(key='-CANVAS-')],
        ]

        layout = [
            [
                sg.Column(trace_viewer_column),
                sg.VSeperator(),
                sg.Column(frame_viewer_column),
            ]
        ]

        fig = self.plot_frame()


        # create the form and show it without the plot
        window = sg.Window(self.title,
                           layout,
                           finalize=True,
                           element_justification='center',
                           font='Helvetica 18')

        # add the plot to the window
        fig_canvas_agg = self.draw_figure(window['-CANVAS-'].TKCanvas, fig)

        self.main_workflow_loop(window)
        window.close()

    def main_workflow_loop(self, window):
        while True:
            event, values = window.read()
            if event == "Exit" or event == sg.WIN_CLOSED:
                break
            # Folder name was filled in, make a list of files in the folder
            if event == "-FOLDER-":
                folder = values["-FOLDER-"]
                try:
                    # Get list of files in folder
                    file_list = os.listdir(folder)
                except:
                    file_list = []

                fnames = [
                    f
                    for f in file_list
                    if os.path.isfile(os.path.join(folder, f))
                       and f.lower().endswith((".png", ".gif"))
                ]
                window["-FILE LIST-"].update(fnames)
            elif event == "-FILE LIST-":  # A file was chosen from the listbox
                try:
                    filename = os.path.join(
                        values["-FOLDER-"], values["-FILE LIST-"][0]
                    )
                    window["-TOUT-"].update(filename)
                    window["-IMAGE-"].update(filename=filename)

                except:
                    pass

    def draw_figure(self, canvas, figure):
        figure_canvas_agg = FigureCanvasTkAgg(figure, canvas)
        figure_canvas_agg.draw()
        figure_canvas_agg.get_tk_widget().pack(side="top", fill="both", expand=1)
        return figure_canvas_agg

    def plot_frame(self):
        fig = matplotlib.figure.Figure(figsize=(5, 4), dpi=100)
        t = np.arange(0, 3, .01)
        fig.add_subplot(111).plot(t, 2 * np.sin(2 * np.pi * t))
        return fig