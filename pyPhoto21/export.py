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
        max_pixel_count_len = 1
        mask_pixels = []
        for i in range(len(traces)):
            if not traces[i].is_fp_trace:
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
                    if not traces[i].is_fp_trace:
                        tr_pixels = mask_pixels[i]
                        if tr_annotations[i].startswith("Region") and row_ct < tr_pixels[0].size:
                            row.append(tr_pixels[0][row_ct])  # x location of pixel
                            row.append(tr_pixels[1][row_ct])  # y location of pixel
                        elif 'px' in tr_annotations[i] and row_ct == 0:
                            indices = traces[i].pixel_indices
                            if len(indices) > 0 and len(indices[0]) == 2:
                                row.append(indices[0][0])  # x location of pixel
                                row.append(indices[0][1])  # y location of pixel
                            else:
                                row.append('')
                                row.append('')
                        else:
                            row.append('')
                            row.append('')
                    tsv_writer.writerow(row)

    def import_regions_from_tsv(self, filename):
        # missing values are filled with -1
        regions = np.genfromtxt(fname=filename,
                                delimiter="\t",
                                skip_header=1,
                                dtype=int)
        annotations = np.genfromtxt(fname=filename,
                                    delimiter="\t",
                                    max_rows=1,
                                    dtype=str)
        regions = np.array(regions)

        if len(regions.shape) == 1:  # all of the regions are 1-px selections
            for i in range(len(annotations)):
                print(regions)
                region_pt = np.array(regions[i*2:i*2+2]).reshape(1, -1)
                print(region_pt.shape, region_pt)
                tr = self.tv.data.get_display_trace(index=region_pt)
                tr.annotation = annotations[i]
                tr.color = self.fv.get_next_color()
                self.tv.traces.append(tr)
        else:
            for i in range(len(annotations)):
                region_points = regions[:, i*2:i*2+2]
                actual_len = 0
                while actual_len < region_points.shape[0] and region_points[actual_len, 0] >= 0:
                    actual_len += 1
                region_points = region_points[:actual_len, :]
                tr = None
                if actual_len < 4:  # less than 4 points is a polygon, not a mask, and order doesn't matter
                    pixel_indices = region_points.reshape(1, -1)
                    tr = self.tv.data.get_display_trace(index=pixel_indices)

                else:
                    h = self.tv.data.get_display_height()
                    w = self.tv.data.get_display_width()
                    mask = np.zeros((h, w), dtype=np.bool)
                    for j in range(actual_len):
                        mask[region_points[j, 0], region_points[j, 1]] = True
                    tr = self.tv.data.get_display_trace(masks=[mask])

                tr.annotation = annotations[i]
                tr.color = self.fv.get_next_color()
                self.tv.traces.append(tr)

        self.tv.update_new_traces()
        self.fv.update_new_image()


