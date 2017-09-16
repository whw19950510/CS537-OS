#!/usr/bin/env python

import os
from subprocess import Popen, PIPE, check_call
import signal


class Timeout:
    """Raise TimeoutError when a function call times out"""

    def __init__(self, seconds=0):
        self.seconds = seconds

    def handle_timeout(self, signum, frame):
        raise TimeoutError('Timeout')

    def __enter__(self):
        signal.signal(signal.SIGALRM, self.handle_timeout)
        signal.alarm(self.seconds)

    def __exit__(self, type, value, traceback):
        signal.alarm(0)


class Test(object):
    """Barebone of a unit test."""

    name = "empty test"
    description = "this tests nothing and should be overriden"
    point_value = 20
    timeout = 20

    @classmethod
    def test(cls):
        """any real implementation of the test goes here
        it should raise exceptions when things fails (mostly through assert)
        and the run() below should handle that"""
        pass

    @classmethod
    def run(cls, args=None):
        """run the test
        This method prints useful message/line separators/etc.
        Any exception comes out of test() will be catched and regarded as
        failure.
        If nothing fails, the point value of the test is returned."""

        print('='*21 + ('%s starts' % cls.name).center(30) + '='*21)

        try:
            with Timeout(cls.timeout):
                cls.test()
        except Exception as e:
            print('Test failed: %s' % cls.name)
            print('Reason:      %s' % e)
            print
            return 0
        print('Test succeeded: %s' % cls.name)
        print
        return cls.point_value


class T0(Test):
    name = "compilation test"
    description = "compiles source code"

    cmd = 'gcc -Wall -o shuffle shuffle.c -O'.split()

    @classmethod
    def test(cls):
        check_call(cls.cmd)


def shuffle(lines):
    """python implementation of the shuffle program"""
    output = []
    i = 0
    j = len(lines)-1
    if j == -1:
        return output
    while True:
        output.append(lines[i])
        if i == j:
            break
        if i < j:
            i += 1
        else:
            i -= 1
        i, j = j, i
    return output


class ShuffleTest(Test):
    """abstract class for all test that runs the shuffle program to extend"""
    inputfile = 'inputfile'
    cmd = './shuffle -i %s -o outputfile'
    stdout = ''
    stderr = ''

    @staticmethod
    def generate_lines():
        """generate inputfile lines for testing"""
        return []

    @classmethod
    def test(cls):
        input_lines = cls.generate_lines()
        with open('inputfile', 'w') as f:
            f.write(''.join(input_lines))

        cmd = (cls.cmd % cls.inputfile).split()
        p = Popen(cmd, stdout=PIPE, stderr=PIPE)
        cls.stdout, cls.stderr = p.communicate()

        assert p.returncode == 0, 'Non-zero exit status %d' % p.returncode

        with open('outputfile', 'r') as f:
            test_output_lines = f.read()
        output_lines = ''.join(shuffle(input_lines))

        assert output_lines == test_output_lines, 'Output does not match'

class ExpectErrorTest(ShuffleTest):
    """abstract class for all shuffle tests that expect an error"""
    returncode = 1
    err_msg = ''

    @classmethod
    def test(cls):
        try:
            super(ExpectErrorTest, cls).test()
        except AssertionError as e:
            assert e.message == 'Non-zero exit status %d' % cls.returncode, 'Expect non-zero exit status'
            assert cls.stdout == '', 'Expect nothing to be printed in stdout'
            assert cls.stderr == cls.err_msg, 'Unexpected error message'
            return cls.point_value
        except Exception as e:
            assert False, 'Unexpected error: %s' % e
        assert False, 'Expect an error from the program'

class T1(ExpectErrorTest):
    name = "invalid file test"
    description = "tests on a non-existing file"

    inputfile = 'youshouldnothavethisfileinyourdirectory'
    err_msg = 'Error: Cannot open file %s\n' % inputfile


class T2(ExpectErrorTest):
    name = "invalid argument test"
    description = "tests on invalid arguments"

    cmd = './shuffle # %s'
    returncode = 1
    err_msg = "Usage: shuffle -i inputfile -o outputfile\n"


class T3(ShuffleTest):
    name = "example test"
    description = "comes from the project specification"

    @staticmethod
    def generate_lines():
        return ['first\n', 'second\n', 'third\n', 'fourth\n', 'fifth\n']


class T4(ShuffleTest):
    name = "empty file test"
    description = "tests on an empty file"


class T5(ShuffleTest):
    name = "normal test 1"
    description = "tests on 1000 short lines"

    @staticmethod
    def generate_lines():
        return ['%d\n' % i for i in range(1000)]


class T6(ShuffleTest):
    name = "normal test 2"
    description = "tests on 1000 long lines"

    @staticmethod
    def generate_lines():
        return [('%d\n' % i).rjust(511, '*') for i in range(1000)]


class T7(ShuffleTest):
    name = "stress test 1"
    description = "tests on 100 thousand long lines (~50MB)"

    @staticmethod
    def generate_lines():
        return [('%d\n' % i).rjust(511, '*') for i in range(100*1000)]


class T8(ShuffleTest):
    name = "stress test 2"
    description = "tests on 10 million short lines (~100MB)"

    @staticmethod
    def generate_lines():
        return ['%d\n' % i for i in range(10*1000*1000)]

def run_tests(tests, short_circuit=True):
    points = 0
    tot_points = sum(test.point_value for test in tests)
    succeeded = failed = 0
    for test in tests:
        point = test.run()
        if point == 0:
            failed += 1
            if short_circuit:
                break
        else:
            succeeded += 1
        points += point
    print('='*21 + 'cleaning up'.center(30) + '='*21)
    for f in ['inputfile', 'outputfile', 'shuffle']:
        if os.path.isfile(f):
            print('removing %s' % f)
            os.remove(f)
    print('='*21 + 'cleanup done'.center(30) + '='*21)
    print

    notrun = len(tests)-succeeded-failed
    print('succeeded/failed/not-run: %d %d %d' % (succeeded, failed, notrun))
    print('score: %d (out of %d)' % (points, tot_points))

def main():
    run_tests([T0, T1, T2, T3, T4, T5, T6, T7, T8])

if __name__ == '__main__':
    main()
