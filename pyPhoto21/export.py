import csv
import numpy as np

from pyPhoto21.database.file import File


class Exporter(File):

    def __init__(self, tv, fv):
        super().__init__(tv.data.meta)
        self.tv = tv  # Pulls annotations and traces from Trace Viewer
        self.fv = fv  # Frame Viewer

    def export_frame_to_tsv(self, filename):
        self.fv.refresh_current_frame()
        curr_frame = self.fv.get_current_frame()
        np.savetxt(filename, curr_frame, delimiter="\t")

        # Get the list of selected traces and export them to TSV
    # Export them clipped, i.e. with only valid times given
    def export_traces_to_tsv(self, filename, precision=8):
        traces = self.tv.get_traces()
        if len(traces) < 1:
            return
        tr_annotations = []
        region_ct = 1
        for i in range(len(traces)):
            text, region_ct = self.tv.create_annotation_text(region_ct, i)
            tr_annotations.append(text)

        starts = [tr_obj.get_start_point() for tr_obj in traces]
        ends = [tr_obj.get_end_point() for tr_obj in traces]
        data = [tr_obj.get_data_clipped() for tr_obj in traces]

        times = self.tv.data.get_cropped_linspace(start_frames=min(starts), end_frames=max(ends))
        with open(filename, 'wt') as output_file:
            tsv_writer = csv.writer(output_file, delimiter='\t')
            tsv_writer.writerow(['Time (ms)'] + tr_annotations)
            for i in range(min(starts), max(ends)):
                time = times[i]
                row = [str(time)[:precision]]
                for j in range(len(traces)):
                    if starts[j] <= i <= ends[j]:
                        row.append(str(data[j][i])[:precision])
                tsv_writer.writerow(row)

    def export_frame_to_png(self, filename):
        fig = self.fv.get_fig()
        fig.savefig(filename)

    def export_traces_to_png(self, filename):
        fig = self.tv.get_fig()
        fig.savefig(filename)
