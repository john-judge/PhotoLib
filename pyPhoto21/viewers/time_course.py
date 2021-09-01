import numpy as np

from pyPhoto21.viewers.trace import Trace
from pyPhoto21.viewers.trace import TraceViewer


class TimeCourse(Trace):

    def __init__(self, points, interval):
        super().__init__(points, interval)


class TimeCourseViewer(TraceViewer):

    def __init__(self, gui):
        super().__init__(gui)

        self.courses = []

    def update(self):
        self.clear_figure()
        self.populate_figure()

    def get_file_map(self, filelist):
        # open each file's meta, read slice, location, and
        # record number, then organize into nested dict.


    def update_file_list(self, **kwargs):
        file_list = kwargs['values']

        # sort file list into an organized map.
        file_map = self.get_file_map(file_list)
        
        print(file_list)

    def add_time_course(self, tc):
        if isinstance(tc, TimeCourse):
            self.courses.append(tc)

    def populate_figure(self):
        num_courses = len(self.courses)
        gs = self.fig.add_gridspec(1, 1)
        self.ax = self.fig.add_subplot(gs[0, 0])
        self.ax.get_yaxis().set_visible(False)  # intensity units are arbitrary
        times = None
        probe_line = self.get_probe_line_locations()
        region_count = 1
        value_type_to_display = self.get_display_value_options()[self.data.get_display_value_option_index()]
        view_window = self.data.get_artifact_exclusion_window()

        for i in range(num_courses):
            course = self.courses[i]
            points = np.array([])
            if isinstance(course, TimeCourse):
                course.clip_time_window(view_window)
                times, points = course.get_data()
            else:
                print("Not a Time Course object:", type(course))

            if times.shape[0] != points.shape[0]:
                print("time and course shape mismatch:", times.shape, points.shape)
            else:
                points += i  # add constant to plot course in its own space.
                self.ax.plot(times, points, color=course.color)

        # y-lim must be set for zoom factor to work
        self.ax.set_ylim([-1, num_courses])

        # x-lim is used to create zoom factor effect
        n = self.data.get_num_pts()
        course_duration = abs(view_window[1] % n - view_window[0] % n) * self.data.get_int_pts()
        course_mid_point = view_window[0] % n * self.data.get_int_pts() + course_duration / 2
        zoom_x_radius = course_duration / self.x_zoom_factor
        x_center = course_mid_point + self.get_current_x_pan_offset()  # x window offset from center
        self.ax.set_xlim([x_center - zoom_x_radius,
                          x_center + zoom_x_radius])

        # Point line (probe type) -- one for the whole plot, now
        if probe_line is not None:
            self.ax.axvline(x=probe_line,
                            ymin=-2,
                            ymax=2,
                            c="gray",
                            linewidth=3,
                            clip_on=False)
            # Point line annotation
            if num_courses > 0:
                probe_annotation = TextArea(str(probe_line)[:5] + " ms")
                y_annotate = self.ax.get_ylim()[1] * 0.9
                ab = AnnotationBbox(probe_annotation,
                                    (probe_line, y_annotate),
                                    xycoords='data',
                                    xybox=(0.5, 1.05),
                                    boxcoords=("axes fraction", "axes fraction"),
                                    box_alignment=(0., 0.5),
                                    arrowprops=dict(arrowstyle="->"))
                self.ax.add_artist(ab)

        self.fig.canvas.draw_idle()

