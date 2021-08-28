from collections import defaultdict


class Viewer:

    def __init__(self):
        # Key event state
        self.control_key_down = False

        # Mouse event state
        self.press = False
        self.moving = False
        self.draw_path = []
        self.path_x_index = defaultdict(list)

    def onkeypress(self, event):
        if event.key == 'control':
            self.control_key_down = True

    def onkeyrelease(self, event):
        if event.key == 'control':
            self.control_key_down = False

    def onpress(self, event):
        if not self.press:
            if event.button in [1, 2, 3]:
                self.clear_waypoints()
                self.add_waypoint(event)
            self.press = True

    def onmove(self, event):
        if self.press and event.button in [1, 2, 3]:
            self.add_waypoint(event)
            self.moving = True

    def is_control_key_held(self):
        return self.control_key_down

    def get_width_in_pixels(self, ax, fig):
        return ax.get_window_extent().transformed(
            fig.dpi_scale_trans.inverted()).width * fig.dpi

    def get_height_in_pixels(self, ax, fig):
        return ax.get_window_extent().transformed(
            fig.dpi_scale_trans.inverted()).height * fig.dpi

    def clear_waypoints(self):
        self.draw_path = []
        self.path_x_index = defaultdict(list)

    def add_waypoint(self, event):
        if event.xdata is not None and event.ydata is not None:
            x = int(event.xdata)
            y = int(event.ydata)
            # avoid duplicate points
            if x in self.path_x_index and y in self.path_x_index[x]:
                pass
            else:
                self.draw_path.append([x, y])
                self.path_x_index[x].append(y)

    def onrelease(self, event):
        self.press = False
        self.moving = False
