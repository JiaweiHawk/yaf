#!/usr/bin/python3
# -*- coding:utf-8 -*-
import argparse
import os
import select
import subprocess
import sys
import traceback

class QemuTerminate(Exception):
    pass

class Qemu:

    def __init__(self, command:str):

        self.proc = subprocess.Popen("exec " + command, shell=True,
                                     stdout=subprocess.PIPE,
                                     stdin=subprocess.PIPE)
        self.output = ""
        self.outbytes = bytearray()

        self._runtil("login:")
        self._write("root\n")

    def _read(self) -> None:
        rset, _, _ = select.select([self.proc.stdout.fileno()], [], [], 1.0)

        if (rset == []):
            self.proc.poll()
            if (self.proc.returncode != None):
                raise QemuTerminate
            return

        self.outbytes += os.read(self.proc.stdout.fileno(), 4096)
        res = self.outbytes.decode("utf-8", "ignore")
        self.output += res
        self.outbytes = self.outbytes[len(res.encode("utf-8")):]

    def _runtil(self, string:str, timeout=0x3f3f3f3f) -> None:
        cursor = 0
        while(True):
            self._read()
            if (string in self.output):
                break
            print(self.output[cursor:], end="", flush=True)

            if (cursor == len(self.output)):
                timeout -= 1
            if (timeout == 0):
                raise TimeoutError
            cursor = len(self.output)

        idx = self.output.find(string) + len(string)
        print(self.output[cursor:idx], end="", flush=True)
        self.output = self.output[idx:]

    def _write(self, buf:str) -> None:
        buf:bytes = buf.encode("utf-8")
        self.proc.stdin.write(buf)
        self.proc.stdin.flush()

    def execute(self, command:str, expect:str=None, timeout=10) -> None:
        self._runtil(":~#", timeout=10)
        self._write(command + "\n")
        if (expect != None):
            self._runtil(expect, timeout=timeout)

    def kill(self) -> None:
        self.proc.kill()
        self.proc.wait()

if __name__ == "__main__":
    ret = 0
    qemu:Qemu = None

    parser = argparse.ArgumentParser(description="yaf test script")
    parser.add_argument("--command", action="store",
                        type=str, required=True,
                        help="command to boot up qemu")
    args = parser.parse_args()

    try:
        # boot up the Qemu
        qemu = Qemu(command=args.command)

        # insmod the yaf module
        qemu.execute("insmod /mnt/shares/yaf.ko")

        # format the disk device
        qemu.execute("/mnt/shares/mkfs /dev/vda")

        qemu.execute("mkdir -p test")

        # mount the device
        qemu.execute("mount -t yaf /dev/vda test")

        qemu.execute("ls -a test", expect=".  ..")

        qemu.execute("mkdir -p test/dir")
        qemu.execute("ls -a test", expect=".  ..  dir")

        qemu.execute("touch test/file")
        qemu.execute("ls -a test", expect=".  ..  dir  file")

        # umount the device
        qemu.execute("umount test")

        # mount the device again
        qemu.execute("mount -t yaf /dev/vda test")

        qemu.execute("ls -a test", expect=".  ..  dir  file")

        # remove the yaf module
        qemu.execute("rmmod yaf")
    except:
        traceback.print_exc()
        ret = -1
    finally:
        if (qemu != None):
            qemu.kill()

    sys.exit(ret)
