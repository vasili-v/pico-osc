#!env python3

import sys
import datetime
import re

import serial

BAUDRATE = 115200

ENQ = b'\x05'
ACK = b'\x06'
NAK = b'\x15'
EOL = b'\n'

MARK_ERROR = 'ERR: '

REGEX_START = re.compile('START: (\\d+)$')
REGEX_DONE = re.compile('DONE: (\\d+)$')

class MeasureError(Exception):
    pass

def wait_for_prompt(s):
    data = s.read_until(ENQ)
    if not data:
        raise MeasureError(f'Got timeout while expecting prompt (ASCII ENQ)')

    prompt = data.find(ENQ)
    if prompt < 0:
        raise MeasureError(
                f'Didn\'t get prompt (ASCII ENQ) in the device response ({repr(data)})'
            )

    response = data[:prompt]
    if response:
        print(f'Got data before prompt (ASCII ENQ): {repr(response)}. Ignoring...', file=sys.stderr)

    tail = data[prompt+1:]
    if tail:
        print(f'Got data after prompt (ASCII ENQ): {repr(tail)}. Ignoring...', file=sys.stderr)

def empty_command(s):
    s.write(EOL)
    wait_for_prompt(s)

def wait_ack(s, name):
    data = s.read_until(ACK)
    if not data:
        raise MeasureError(
                f'Got timeout while expecting acknowledgement (ASCII ACK) for command "{name}"'
            )

    ack = data.find(ACK)
    if ack:
        raise MeasureError(
                f'Didn\'t get acknowledgement (ASCII ACK) for command "{name}" in the device '
                f'response ({repr(data)})'
            )

    response = data[:ack]
    if response:
        print(
                f'Got data before acknowledgement (ASCII ACK) for command "{name}": '
                f'{repr(response)}. Ignoring...',
                file=sys.stderr
            )

    tail = data[ack+1:]
    if tail:
        print(
                f'Got data after acknowledgement (ASCII ACK) for command "{name}": {repr(tail)}. '
                 'Ignoring...',
                file=sys.stderr
            )

def command(s, *args):
    s.write(' '.join(args).encode('ascii')+EOL)
    return wait_ack(s, args[0])

def read_response(s, timeout=None):
    prev_timeout = None
    if timeout is not None:
        prev_timeout = s.timeout
        s.timeout = timeout

    try:
        data = s.read_until(EOL)
        if not data:
            raise MeasureError('Got timeout while expected a line')

        eol = data.find(EOL)
        if eol < 0:
            raise MeasureError(
                    f'Didn\'t get a complete line: ({repr(data)})'
                )

        tail = data[eol+1:]
        if tail:
            print(f'Got data after end of line: {repr(tail)}. Ignoring...', file=sys.stderr)

        return data[:eol].decode('ascii').strip()
    finally:
        if prev_timeout is not None:
            s.timeout = prev_timeout

def measure(device, points, baudrate):
    with serial.Serial(sys.argv[1], BAUDRATE, timeout=1, write_timeout=5) as s:
        print('Sending empty command...', file=sys.stderr)
        empty_command(s)
        print('Got command prompt.', file=sys.stderr)

        print('Sending "id" command...', file=sys.stderr)
        command(s, 'id')
        print(f'Device: {read_response(s)}.', file=sys.stderr)

        wait_for_prompt(s)
        print('Sending "measure" command...', file=sys.stderr)
        command(s, 'measure', str(points))
        response = read_response(s)
        if response.startswith(MARK_ERROR):
            try:
                wait_for_prompt(s)
            except MeasureError:
                pass

            raise MeasureError(f'Measurement error: {response[len(MARK_ERROR):]}')

        m = REGEX_START.match(response)
        if not m:
            raise MeasureError(f'Unexpected response to "measure" command: "{response}"')
        t0 = datetime.datetime.now()
        print(f'Started measurement for {m.group(1)} point(s).', file=sys.stderr)

        response = read_response(s, timeout=60)
        m = REGEX_DONE.match(response)
        if not m:
            raise MeasureError(f'Unexpected response to "measure" command: "{response}"')
        t1 = datetime.datetime.now()
        try:
            points = int(m.group(1))
        except ValueError:
            raise MeasureError(f'Can\'t treat "{m.group(1)}" as number of measured points')
        print(f'Finished measurement. Got {points} point(s) in {t1-t0}.', file=sys.stderr)

        wait_for_prompt(s)
        if points <= 0:
            return

        print('Sending "measure" command...', file=sys.stderr)
        command(s, 'data')

        print('#,Time,Value')
        cols_num = None
        t0 = None
        n = 0
        lines = 0
        for i in range(points):
            line = read_response(s)
            cols = line.split(',')
            if cols_num is None:
                if len(cols) < 2 or len(cols) > 3:
                    print(
                            f'Got {len(cols)} column(s) for line {i+1} ("{line}"), while expected '
                             '2 or 3. Ignoring...',
                            file=sys.stderr
                        )
                    continue

                cols_num = len(cols)

            if len(cols) != cols_num:
                print(
                        f'Got {len(cols)} column(s) for line {i+1} ("{line}"), while expected '
                        f'{cols_num}. Ignoring...',
                        file=sys.stderr
                    )
                continue

            if cols_num < 3:
                try:
                    v = int(cols[1])
                except ValueError:
                    print(
                            f'Can\'t treat "{cols[1]}" in line {i+1} as a valid measurement '
                             'value. Ignoring...',
                            file=sys.stderr
                        )
                    continue

                n += 1
                lines += 1
                if v < 2:
                    print(f'{n},0')
                    print(f'{n},1')
                else:
                    print(f'{n},1')
                    print(f'{n},0')
            else:
                try:
                    t1 = int(cols[1])
                except ValueError:
                    print(
                            f'Can\'t treat "{cols[1]}" in line {i+1} as a number of ticks. '
                             'Ignoring...',
                            file=sys.stderr
                        )
                    continue

                if t0 is None:
                    t0 = t1

                t = t0-t1
                try:
                    v = int(cols[2])
                except ValueError:
                    print(
                            f'Can\'t treat "{cols[2]}" in line {i+1} as a valid measurement '
                             'value. Ignoring...',
                            file=sys.stderr
                        )
                    continue

                n += 1
                lines += 1
                if v < 2:
                    print(f'{n},{t},0')
                    n += 1
                    print(f'{n},{t},1')
                else:
                    print(f'{n},{t},1')
                    n += 1
                    print(f'{n},{t},0')

        print(f'Got {lines} valid data line(s).', file=sys.stderr)

def main():
    if len(sys.argv) <= 1:
        print('Missing serial interface name. Exiting...', file=sys.stderr)
        return 2

    points = 1000
    if len(sys.argv) > 2:
        points = sys.argv[2]

    try:
        measure(sys.argv[1], points, BAUDRATE)
    except MeasureError as e:
        print(f'{e}. Exiting...', file=sys.stderr)
        return 1

    return 0

if __name__ == '__main__':
    sys.exit(main())
