import CHIP_IO.GPIO as GPIO
import time
import threading
import subprocess

duration = 0.1 * 60
gpio_pin = "XIO-P7"


def start_sound():
    subprocess.Popen(["play", "-q", "/home/chip/button/start.wav"])


def cancel_sound():
    subprocess.Popen(["play", "-q", "/home/chip/button/cancel.wav"])


def finish_sound():
    subprocess.Popen(["play", "-q", "/home/chip/button/sw.wav"])


def timer_done():
    print("Done")
    finish_sound()
    tt.cancel()


class RepeatableTimer(object):
    def __init__(self, interval, function, args=[], kwargs={}):
        self._interval = interval
        self._function = function
        self._args = args
        self._kwargs = kwargs
        self.__started = False

    def start(self):
        t = threading.Timer(self._interval, self._function, *self._args, **self._kwargs)
        t.start()
        self.__started = True
        self.__thread = t

    def running(self):
        return self.__started

    def cancel(self):
        self.__thread.cancel()
        self.__started = False

    def stopped(self):
        self.__started = True

# Setup Timer
tt = RepeatableTimer(duration, timer_done)
# Setup Pin
GPIO.setup(gpio_pin, GPIO.IN)

keep_going = True
while keep_going:
    try:
        input_state = GPIO.input(gpio_pin)
        if input_state == False:
            print('Button Pressed')
            if not tt.running():
                print('Starting Timer')
                tt.start()
                start_sound()
            else:
                print('Button Already Pushed. Canceling Timer....')
                tt.cancel()
                cancel_sound()
            time.sleep(1)
    except KeyboardInterrupt:
        keep_going = False
# Be nice...
GPIO.cleanup()
