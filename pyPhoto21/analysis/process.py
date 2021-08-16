import time

import numpy as np
from queue import Queue


class Processor:
    """ Processes data in background """
    def __init__(self):
        self.queued_jobs = Queue()
        self.is_active = True
        self.sleep_interval = 2.0
        self.stop_worker_flag = False

    def stop_processor(self):
        self.stop_worker_flag = True
        while self.stop_worker_flag:
            time.sleep(1)

    # Launch this looped worker as separate thread
    def process_continually(self):
        while not self.stop_worker_flag:
            if self.queued_jobs.empty():
                self.is_active = False
                time.sleep(self.sleep_interval * 3)
            else:
                self.is_active = True
                job = self.queued_jobs.get()
                job.process()

            time.sleep(self.sleep_interval)

    def get_is_active(self):
        return self.is_active

    def submit_processing_job(self, job):
        self.queued_jobs.put(job)


class Job:
    def __init__(self, fns_to_call, args_to_call):
        self.functions = fns_to_call
        self.args = args_to_call

    def process(self):
        for i in range(len(self.functions)):
            self.functions[i](self.args[i])