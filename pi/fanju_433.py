#!/usr/bin/env python3

"""
Analyse RF433 Rx data
January 2021 Chaos Geordend

run as daemon: ./analyse_rf433.py &
terminate: kill -s 15 <pid>  # SIGTERM

"""

import time
import signal
from collections import namedtuple
from datetime import datetime
import tkinter as tk
from threading import Thread
import queue
from queue import Queue

import RPi.GPIO as GPIO


BUFFER_SIZE = 256
EXIT = 'EXIT'

# durations (ns)
MIN_MARK = 450
MAX_SPACE = 16000


class RF433(Thread):
    """
    Register the pulses detected by the receiver.
    """
    def __init__(self, queue, gpio_pin=18, args=(), kwargs=None):
        Thread.__init__(self)
        self.pin = gpio_pin
        self.queue = queue

        self.buffer = [0 for i in range(BUFFER_SIZE)]
        self.count = 0
        self.last = int(time.perf_counter() * 1000000)  # ns

        # use Broadcom SOC channel numbers
        GPIO.setmode(GPIO.BCM)
        # RF433 input with a 50k pull up
        GPIO.setup(self.pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)

    def run(self):
        GPIO.add_event_detect(self.pin, GPIO.BOTH)
        GPIO.add_event_callback(self.pin, callback=self.handle_interrupt)
        self.running = True

    def join(self, timeout=None):
        """Stop this thread."""
        self.cleanup()
        Thread.join(self)

    def cleanup(self):

        def noop_callback(channel):
            pass

        print("Receiver exiting...")
        GPIO.add_event_callback(self.pin, callback=noop_callback)  # remove seems not to work
        time.sleep(1)  # allow any leftover interrupts to be handled
        GPIO.remove_event_detect(self.pin)
        GPIO.cleanup()

    def handle_interrupt(self, channel):
        timer = int(time.perf_counter() * 1000000)
        duration = timer - self.last  # elapsed time (ns) since last edge
        self.last = timer

        if self.count >= BUFFER_SIZE:
            self.count = 0
            try:
                self.queue.put_nowait(self.buffer)
            except queue.Full:
                pass
        if duration >= MIN_MARK and duration <= MAX_SPACE:
            self.buffer[self.count] = duration
            self.count += 1


class Decoder(Thread):
    """
    Try to decode the detected pulses.
    """
    # message length (bits) and durations (ns) for each protocol:
    # Fanju similar to the Auriol v3 (?) protocol
    # Ventus cf Alecto v1

    Protocol = namedtuple('Protocol',
                          ['name',
                           'msg_length',
                           't_preamble',
                           't_max',
                           't_sync',
                           't_mark',
                           't_space_0',
                           't_space_1',
                           'factor'])

    protocols = (Protocol('fanju', 40, 7500, 128000, 540, 850, 1600, 3900, 1.25),
                 Protocol('ventus', 36, 7500, 120000, 450, 595, 1700, 3750, 1.25))

    def __init__(self, qbuffer, qtemperature, args=(), kwargs=None):
        Thread.__init__(self)
        self.running = False
        self.qbuffer = qbuffer
        self.qtemperature = qtemperature
        self.running = False
        self.temperature = [0, 0]

    def run(self):
        self.running = True
        try:
            while self.running:
                try:
                    buffer = self.qbuffer.get()
                    if buffer == EXIT:
                        self.running = False
                    else:
                        for pr in self.protocols:
                            # scan
                            found = False
                            for i, d in enumerate(buffer):
                                if d > pr.t_preamble:
                                    if self.decode(buffer, i, pr):
                                        found = True
                                        break
                            if found:
                                break
                    self.qbuffer.task_done()
                except queue.Empty:
                    pass
        except KeyboardInterrupt:
            self.running = False

    def decode(self, buffer, pos, pr):
        """
        Decode pulse stream.
        @param: buffer array comprising pulse durations
        @param: pos    position in the buffer where the stream is expected to start
        @param: pr     protocol to use for decoding the stream
        """
        retval = False
        data = []
        bit_count = 0
        pulse_count = 0

        # print("Analyse from position: %s" % pos)
        while bit_count < pr.msg_length and pulse_count < BUFFER_SIZE:
            # mark
            if buffer[pos] >= pr.t_mark and \
               buffer[pos] < pr.t_mark * pr.factor:
                pass
            # space
            elif buffer[pos] >= pr.t_preamble and buffer[pos] < pr.t_preamble * pr.factor:
                # begin of a pulse train
                # print("START")
                data = []
                bit_count = 0
            elif buffer[pos] > pr.t_max:
                # time-out
                print("TIME OUT")
                data = []
                bit_count = 0
                break
            elif buffer[pos] >= pr.t_space_0 and buffer[pos] < pr.t_space_0 * pr.factor:
                data.append('0')
                bit_count += 1
            elif buffer[pos] >= pr.t_space_1 and buffer[pos] < pr.t_space_1 * pr.factor:
                data.append('1')
                bit_count += 1

            pulse_count += 1

            pos += 1
            if pos >= BUFFER_SIZE:
                pos = 0

        # print("pos: %s proto: %s, bits: %s" % (pos, pr.name, bit_count))
        # if bit_count == pr.msg_length:
        if bit_count == pr.msg_length and \
           buffer[pos] >= pr.t_sync and buffer[pos] <= pr.t_sync * pr.factor:

            # for debugging only
            next_pos = pos + 1
            if next_pos > BUFFER_SIZE:
                next_pos = 0

            timestamp = datetime.fromtimestamp(time.time())
            retval = False
            # print("proto: %s, bits: %s" % (pr.name, bit_count))

            if pr.name == 'ventus':
                retval = self.checksum_ventus(data)
                if retval:
                    #print("last: %s, next: %s" % (buffer[pos], buffer[next_pos]))
                    temperature = int(''.join(data[23:11:-1]), 2) / 10
                    if self.temperature[0] != temperature:
                        self.temperature[0] = temperature
                        qmsg = ['Ventus', timestamp, temperature, 99, self.markup(''.join(data), pr.msg_length), str(retval)]
                        self.qtemperature.put(qmsg)

            elif pr.name == 'fanju':
                # AliExpress sensor
                retval = self.checksum_fanju(data)
                if True or retval:
                    #print("last: %s, next: %s" % (buffer[pos], buffer[next_pos]))
                    fahrenheit = int(''.join(data[16:28]), 2)
                    temperature = (fahrenheit - 0x4C4) * 5 / 90
                    temperature = round(temperature, 1)
                    #temperature = round(fahrenheit - 0x4E4, 1)
                    humidity = int(''.join(data[28:32]), 2) * 10 + int(''.join(data[32:36]), 2)
                    if True or self.temperature[1] != temperature:
                        self.temperature[1] = temperature
                        qmsg = ['AliExp', timestamp, temperature, humidity, self.markup(''.join(data), pr.msg_length), str(retval)]
                        self.qtemperature.put(qmsg)
        return retval

    def checksum_fanju(self, msg):
        """Calculate Fanju 3378 checksum."""
        csum = 0x0
        mask = 0xC
        data = []
        # last nibble replaces the checksum nibble
        data.extend(msg[0:8])
        data.extend(msg[36:40])
        data.extend(msg[12:36])
        checksum = int(''.join(msg[8:12]), 2)
        for b in data:
            bit = mask & 0x1
            mask >>= 1
            if bit == 0x1:
                mask ^= 0x9
            if b == '1': 
                csum ^= mask
        csum &= 0xF
        return(csum == checksum)

    def checksum_ventus(self, data):
        """Calculate Alecto v1 checksum."""
        n = []
        # reverse all nibbles before processing
        n.append(int(''.join(data[3::-1]), 2))
        for i in range(7, len(data), 4):
            n.append(int(''.join(data[i:i-4:-1]), 2))
        sum = (0xF - n[0] - n[1] - n[2] - n[3] - n[4] - n[5] - n[6] - n[7]) & 0xF
        return(n[8] == sum)

    def markup(self, bits, msg_length, base=2):
        """Print the received data as binary nibbles."""
        text = ''
        s = 0
        e = 4
        nsum = 0
        width = 5
        nnib = 0
        if len(bits) >= msg_length:
            while e <= msg_length:
                nnib += 1
                nibble = ''.join(bits[s:e])
                nnum = int(nibble, 2)

                # exclude the third nibble (the assumed checksum) from checks
                if nnib != 3:
                    nsum += nnum

                if base == 16:
                    text += hex(nnum).ljust(width, ' ')
                elif base == 10:
                    text += str(nnum).ljust(width, ' ')
                else:
                    text += nibble.ljust(width, ' ')
                s += 4
                e += 4
        if base == 16:
            text += ' ' + hex(nsum)
        elif base == 10:
            text += ' ' + str(nsum)
        else:
            text += ' ' + bin(nsum)
        return text


class Gui():
    """
    Makeshift Window to display temperature sensor values.
    """
    def __init__(self, root, queue, args=(), kwargs=None):
        self.root = root
        self.queue = queue
        self.count = 0

        w = tk.Label(root, text="Temperature readings")
        w.pack()

        t1 = tk.Text(root, width=20, height=3, state=tk.DISABLED, bg='dark grey')
        t1.pack(padx='2m', pady='2m')
        self.text1 = t1

        t2 = tk.Text(root, width=20, height=3, state=tk.DISABLED, bg='dark grey')
        t2.pack(padx='2m', pady='2m')
        self.text2 = t2

        t3 = tk.Text(root, width=10, height=3, state=tk.DISABLED, bg='dark grey')
        t3.pack(padx='2m', pady='2m')
        self.text3 = t3

        root.protocol("WM_DELETE_WINDOW", self.quit)

        self.update_temp()

    def quit(self):
        self.root.destroy()
        self.root.quit()

    def update_temp(self):
        try:
            # [protocol, timestamp, value]
            qmsg = self.queue.get(False)
            protocol = qmsg[0]
            timestamp = qmsg[1]
            value = qmsg[2]
            tekst = str(value) + 'Â° \n' + timestamp.strftime('%H:%M %A')

            if qmsg[0] == 'Ventus':
                tekst = str(value) + 'Â° \n(' + timestamp.strftime('%H:%M') + ')'
                self.text1.configure(state=tk.NORMAL)
                self.text1.replace('1.0', tk.END, tekst)
                self.text1.configure(state=tk.DISABLED)

            if qmsg[0] == 'AliExp':
                tekst = str(value) + 'Â° ' + str(qmsg[3]) + '%\n(' + timestamp.strftime('%H:%M') + ')'
                self.text2.configure(state=tk.NORMAL)
                self.text2.replace('1.0', tk.END, tekst)
                self.text2.configure(state=tk.DISABLED)

            self.count += 1
            self.text3.configure(state=tk.NORMAL)
            self.text3.replace('1.0', tk.END, str(self.count))
            self.text3.configure(state=tk.DISABLED)

            self.queue.task_done()
        except queue.Empty:
            pass

        # schedule next update
        self.root.after(500, self.update_temp)


def log_temp(queue):
    flog = open('temperature_data', 'a')
    qmsg = queue.get()
    while qmsg != EXIT:
        log_msg = qmsg[0] + ' ' + qmsg[1].strftime('%d-%m %H:%M') + ' ' + str(qmsg[2]) + 'Â° ' + str(qmsg[3]) + '% ' + qmsg[4] + ' ' + qmsg[5] + '\n'
        print(log_msg)
        flog.write(log_msg)
        queue.task_done()
        qmsg = queue.get()
    flog.close()


def run(tk_gui=False):

    def handle_exit(sig, frame):
        raise(SystemExit)

    signal.signal(signal.SIGTERM, handle_exit)

    qbuffer = Queue(maxsize=50)
    qtemperature = Queue(maxsize=10)
    rf = RF433(qbuffer)
    rf.start()
    decoder = Decoder(qbuffer, qtemperature)
    decoder.start()

    if tk_gui:
        root = tk.Tk()
        root.title("RF433 Data")
        # root.geometry('300x400')
        gui = Gui(root, qtemperature)  # noqa F841
        root.mainloop()
        qbuffer.put_nowait(EXIT)
        qtemperature.put_nowait(EXIT)
    else:
        monitor = Thread(target=log_temp, args=(qtemperature,))
        monitor.setDaemon(True)
        monitor.start()

    try:
        # gui.join()
        if not tk_gui:
            monitor.join()
        decoder.join()
        rf.join()
    except (KeyboardInterrupt, SystemExit):
        print("Main caught exception, exiting...")
        qtemperature.put(EXIT)
        qbuffer.put(EXIT)
        if not tk_gui:
            monitor.join()
        decoder.join()
        rf.join()


if __name__ == '__main__':
    run()

