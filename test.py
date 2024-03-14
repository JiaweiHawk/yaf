#!/usr/bin/python3
# -*- coding:utf-8 -*-
import argparse
import os
import select
import subprocess
import sys
import traceback
import random

class QemuTerminate(Exception):
    pass

class Qemu:

    def __init__(self, command:str):

        self.proc = subprocess.Popen("exec " + command, shell=True,
                                     stdout=subprocess.PIPE,
                                     stdin=subprocess.PIPE)
        self.output = ""
        self.outbytes = bytearray()

        self.runtil("login:")
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

    def runtil(self, string:str, timeout=0x3f3f3f3f) -> None:
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

    def execute(self, command:str) -> None:
        self.runtil(":~#", timeout=10)
        self._write(command + "\n")

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
        dirs = []
        files = []

        # insmod the yaf module
        qemu.execute("insmod /mnt/shares/yaf.ko")

        # format the disk device
        qemu.execute("/mnt/shares/mkfs /dev/vda")

        qemu.execute("mkdir -p test")

        # mount the device
        qemu.execute("mount -t yaf /dev/vda test")

        def check_directory():
            qemu.execute("ls -al test")

            # check entrys number
            qemu.execute("ls -al test | wc -l")
            qemu.runtil(str(len(dirs) + len(files) + 3), timeout=10)

            # check '.'
            qemu.execute('''ls -al test | grep " \.$" | wc -l''')
            qemu.runtil("1", timeout=10)

            # check '..'
            qemu.execute('''ls -al test | grep " \.\.$"''')
            qemu.runtil("1", timeout=10)

            # check each entrys
            for name in dirs + files:
                qemu.execute('''ls -al test | grep " %s$" | wc -l'''%(name))
                qemu.runtil("1", timeout=10)

        # add random subdirectorys
        for i in range(64 + random.randint(1, 32)):
            name = "dir%d"%(i)
            dirs.append(name)
            qemu.execute("mkdir -p test/%s"%(name))
        check_directory()

        # add random files
        for i in range(64 + random.randint(1, 32)):
            name = "file%d"%(i)
            files.append(name)
            qemu.execute("touch test/%s"%(name))
        check_directory()

        # delete test
        qemu.execute("rmdir test")
        qemu.runtil("rmdir: failed to remove 'test': Device or resource busy", timeout=10)

        # delete random entrys
        for i in range(32 + random.randint(1, 16) + 32 + random.randint(1, 16)):
            if (random.randint(0, 1) == 0 or len(dirs) == 0):
                # delete the last dir
                qemu.execute("rmdir test/%s"%(dirs[-1]))
                dirs.pop()
            else:
                # delete the last file
                qemu.execute("rm test/%s"%(files[-1]))
                files.pop()
        check_directory()

        # umount the device
        qemu.execute("umount test")

        # mount the device again
        qemu.execute("mount -t yaf /dev/vda test")

        check_directory()

        # remove the yaf module
        qemu.execute("rmmod yaf")
    except:
        traceback.print_exc()
        ret = -1
    finally:
        if (qemu != None):
            qemu.kill()

    sys.exit(ret)
