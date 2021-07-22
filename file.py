

class File:

    def __init__(self):
        self.current_slice = 0
        self.current_location = 0
        self.current_run = 0

    def increment_slice(self):
        self.current_slice += 1
        self.current_location = 0
        self.current_run = 0

    def increment_location(self):
        self.current_location += 1
        self.current_run = 0

    def increment_run(self):
        self.current_run += 1

    @staticmethod
    def pad_zero(i):
        s = str(i)
        if len(s) < 2:
            return '0' + s
        return s

    def get_filename(self, extension='.zda'):
        return self.pad_zero(self.current_slice) + '-' + \
               self.pad_zero(self.current_location) + '-' + \
               self.pad_zero(self.current_run) + extension

    def save_to_file(self, images):
        fn = self.get_filename()
        print("Saving to file " + fn + "...")
        # TO DO: use ZDA format.
