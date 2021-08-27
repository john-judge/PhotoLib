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
            if event.button == 1 or event.button == 3:
                self.clear_waypoints()
                self.add_waypoint(event)
            self.press = True

    def onmove(self, event):
        if self.press and event.button in [1, 3]:  # left or right mouse
            self.add_waypoint(event)
            self.moving = True

    def is_control_key_held(self):
        return self.control_key_down

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
