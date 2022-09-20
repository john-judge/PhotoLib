

# This file maps event/element names to data attribute getters
# So that the action of keeping the GUI up-to-date with f
class FieldBindings:

    def __init__(self, gui):
        self.gui = gui
        self.bindings = {
            'event_name': gui.getter,
        }

    def get_field_bindings(self, event_name):
        if event_name in self.bindings:
            return self.bindings[event_name]