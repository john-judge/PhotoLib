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
                    else:
                        row.append('')
                tsv_writer.writerow(row)

    def export_frame_to_png(self, filename):
        fig = self.fv.get_fig()
        fig.savefig(filename)

    def export_traces_to_png(self, filename):
        fig = self.tv.get_fig()
        fig.savefig(filename)

    def export_regions_to_tsv(self, filename):
        traces = self.tv.get_traces()
        if len(traces) < 1:
            return
        tr_annotations = []
        region_ct = 1
        max_pixel_count_len = 0
        mask_pixels = []
        for i in range(len(traces)):
            text, region_ct = self.tv.create_annotation_text(region_ct, i)
            tr_annotations.append(text)
            max_pixel_count_len = max(max_pixel_count_len, traces[i].get_pixel_count())
            mask_pixels.append(np.where(traces[i].master_mask))

        with open(filename, 'wt') as output_file:
            tsv_writer = csv.writer(output_file, delimiter='\t')
            tsv_writer.writerow(tr_annotations)
            for row_ct in range(max_pixel_count_len):
                row = []
                for i in range(len(traces)):
                    tr_pixels = mask_pixels[i]
                    if tr_annotations[i].startswith("Region") and row_ct < tr_pixels[0].size:
                        row.append(tr_pixels[0][row_ct])  # x location of pixel
                        row.append(tr_pixels[1][row_ct])  # y location of pixel
                    else:
                        row.append('')
                        row.append('')
                tsv_writer.writerow(row)

    def import_regions_from_tsv(self, filename):
        pass