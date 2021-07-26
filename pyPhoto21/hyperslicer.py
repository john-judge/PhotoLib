import napari


class HyperSlicer:
    def __init__(self, data, show_rli=False):
        self.data = data
        print("Launching Napari Viewer...")
        self.viewer = napari.Viewer()
        self.update_data(show_rli=show_rli)

    def update_data(self, show_rli=False):
        try:
            if show_rli:
                self.viewer.add_image(self.data.get_rli_images(), rgb=False)
            else:
                self.viewer.add_image(self.data.get_acqui_images(), rgb=False)
        except Exception as e:
            print("Napari exception -- likely the Napari window was closed and not reopened")
            print(str(e))