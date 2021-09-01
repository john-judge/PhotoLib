import numpy as np

from pyPhoto21.viewers.trace import Trace
from pyPhoto21.viewers.trace import TraceViewer


class TimeCourse(Trace):

    def __init__(self, points, interval):
        super().__init__(points, interval)


class TimeCourseViewer(TraceViewer):

    def __init__(self, gui):
        super().__init__(gui)

        self.file_map = {}

    def update(self):
        self.clear_figure()
        self.populate_figure()

    def get_file_map(self, filelist):
        # open each file's meta, read slice, location, and
        # record number, then organize into nested dict.
        value_type_to_display = self.get_display_value_options()[self.data.get_display_value_option_index()]
        if value_type_to_display == "None":
            print("Please select a display value type to plot time courses.")
            return

        traces = self.gui.tv.traces
        cached_memmap_file = self.gui.data.db.memmap_file  # cache this off to restore later

        file_map = {}
        for data_file in filelist:
            meta_file = data_file.split('.')[0] + self.data.metadata_extension
            meta = self.gui.data.load_metadata_from_file(meta_file)
            self.gui.data.db.load_mmap_file(meta=meta,
                                            filename=data_file,
                                            mode='r+',
                                            reshape_rli_buffer=False)
            slic, loc, rec = meta.current_slice, meta.current_location, meta.current_record
            if slic not in file_map:
                file_map[slic] = {}
            if loc not in file_map[slic]:
                file_map[slic][loc] = {}
            if rec not in file_map[slic][loc]:
                file_map[slic][loc][rec] = {}
            file_map[slic][loc][rec][data_file] = []
            # file_map[slic][loc][rec][data_file]['meta'] = meta
            for i in range(len(traces)):
                _, points = traces[i].get_data()
                val = self.gui.tv.create_display_value(value_type_to_display, i, points)
                if val == '':
                    val = None
                file_map[slic][loc][rec][data_file].append(val)

        self.gui.data.db.memmap_file = cached_memmap_file  # restore db setting
        return file_map

    def update_file_list(self, **kwargs):
        file_list = kwargs['values']

        # check for new files
        curr_files = self.gui.data.get_data_filenames_in_folder()
        self.gui.window["Time Course File Selector"].update(curr_files)
        self.gui.window["Time Course File Selector"].set_value(file_list)

        # sort file list into an organized map.
        self.file_map = self.get_file_map(file_list)
        if self.file_map is not None and len(list(self.file_map)) > 0:
            print(self.file_map)
            self.update()

    def populate_figure(self):
        gs = self.fig.add_gridspec(1, 1)
        self.ax = self.fig.add_subplot(gs[0, 0])
        int_rec = self.gui.data.get_int_records()
        traces = self.gui.tv.traces
        highest_record = 0
        mn = None
        mx = None

        for slic in self.file_map:
            for loc in self.file_map[slic]:

                time_courses = [[] for _ in range(len(traces))]

                recs = list(self.file_map[slic][loc])
                recs.sort()
                highest_record = max(highest_record, recs[-1])
                t_linspace = np.array([r * int_rec for r in recs])
                for rec in recs:  # in sorted record order

                    files = list(self.file_map[slic][loc][rec])
                    data_file = files[0]
                    if len(files) > 1:
                        print("multiple files found for: \n\tslice:", slic, "\n\tloc:", loc, "\n\trec:", rec,
                              "\n\tOnly this file will be included in the time course plot:", data_file)

                    for i in range(len(traces)):
                        time_courses[i].append(
                            self.file_map[slic][loc][rec][data_file][i]
                        )

                print("time_courses", time_courses)

                for i in range(len(traces)):
                    if None not in time_courses[i]:
                        points = np.array(time_courses[i])
                        if i > 0:
                            points += max(time_courses[i-1])
                        if points.size > 0:
                            if mn is None or mx is None:
                                mn = np.min(points)
                                mx = np.max(points)
                            else:
                                mn = min(mn, np.min(points))
                                mx = max(mx, np.max(points))
                            self.ax.plot(t_linspace, points, color=traces[i].color)

        # y-lim must be set for zoom factor to work
        if mn is not None and mx is not None:
            self.ax.set_ylim([mn - abs(mn)*0.05,
                              mx + abs(mx)*0.05])

        # x-lim is used to create zoom factor effect
        course_duration = highest_record * int_rec
        course_mid_point = course_duration / 2
        zoom_x_radius = course_duration / self.x_zoom_factor
        x_center = course_mid_point + self.get_current_x_pan_offset()  # x window offset from center
        self.ax.set_xlim([x_center - zoom_x_radius,
                          x_center + zoom_x_radius])

        self.fig.canvas.draw_idle()

