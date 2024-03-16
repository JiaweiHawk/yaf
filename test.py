#!/usr/bin/python3
# -*- coding:utf-8 -*-
import argparse
import os
import select
import subprocess
import sys
import string
import traceback
import random
import time
import hashlib

class QemuTerminate(Exception):
    pass

class Qemu:

    def __init__(self, command:str, history:str):

        self.proc = subprocess.Popen("exec " + command, shell=True,
                                     stdout=subprocess.PIPE,
                                     stdin=subprocess.PIPE)
        self.output = ""
        self.outbytes = bytearray()


        self.history = history

        with open(history, "w") as f:
            f.truncate()
            f.write("#!/bin/bash\n")

    def _read(self) -> None:
        rset, _, _ = select.select([self.proc.stdout.fileno()], [], [], 1)

        if (rset == []):
            self.proc.poll()
            if (self.proc.returncode != None):
                raise QemuTerminate
            return

        self.outbytes += os.read(self.proc.stdout.fileno(), 4096)
        res = self.outbytes.decode("utf-8", "ignore")
        self.output += res
        self.outbytes = self.outbytes[len(res.encode("utf-8")):]

    def runtil(self, string:str, timeout:int) -> None:
        cursor = 0
        cur = time.time()
        while(True):
            self._read()
            if (string in self.output):
                break
            print(self.output[cursor:], end="", flush=True)
            if (time.time() - cur > timeout):
                raise TimeoutError
            cursor = len(self.output)

        idx = self.output.find(string) + len(string)
        #print(self.output[cursor:idx], end="", flush=True)
        self.output = self.output[idx:]

    def write(self, buf:str) -> None:
        buf:bytes = buf.encode("utf-8")
        self.proc.stdin.write(buf)
        self.proc.stdin.flush()

    def execute(self, command:str) -> None:
        with open(self.history, "a") as f:
            f.write(command + "\n")

        self.runtil(":~#", timeout=60)
        self.write(command + "\n")

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
    parser.add_argument("--history", action="store",
                        type=str, required=True,
                        help="path to store executed commands")
    parser.add_argument("--timeout", action="store",
                        type=int, default=10,
                        help="max timeout for receiving from guest")
    args = parser.parse_args()

    try:
        # boot up the Qemu
        qemu = Qemu(command=args.command, history=args.history)
        dirs = []
        files = []

        qemu.runtil("login:", timeout=args.timeout)
        qemu.write("root\n")

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
            qemu.runtil(str(len(dirs) + len(files) + 3), timeout=args.timeout)

            # check '.'
            qemu.execute('''ls -al test | grep " \.$" | wc -l''')
            qemu.runtil("1", timeout=args.timeout)

            # check '..'
            qemu.execute('''ls -al test | grep " \.\.$"''')
            qemu.runtil("1", timeout=args.timeout)

            # check each entrys
            for name in dirs + files:
                qemu.execute('''ls -al test | grep " %s$" | wc -l'''%(name))
                qemu.runtil("1", timeout=args.timeout)

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

        # write random files
        contents = {}

        def check_files():
            qemu.execute('''ls -al test/file*''')
            for name in files:
                if name in contents:
                    qemu.execute("cat test/%s | wc -c"%(name))
                    qemu.runtil(str(len(contents[name])), timeout=args.timeout)
                    qemu.execute("md5sum test/%s"%(name))
                    qemu.runtil(hashlib.md5(contents[name].encode("ascii")).hexdigest(), timeout=args.timeout)
                    qemu.execute("sha256sum test/%s"%(name))
                    qemu.runtil(hashlib.sha256(contents[name].encode("ascii")).hexdigest(), timeout=args.timeout)
                    qemu.execute("sha512sum test/%s"%(name))
                    qemu.runtil(hashlib.sha512(contents[name].encode("ascii")).hexdigest(), timeout=args.timeout)
                else:
                    qemu.execute("cat test/%s | wc -c"%(name))
                    qemu.runtil(str(0), timeout=args.timeout)

        max_filesize = 4096 * 8
        step = 64
        # write *wnumber* times making each file *max_filesize / 2* bytes
        wnumber = int(max_filesize / 2 * len(files) / step)
        for i in range(wnumber):
            name = files[random.randint(0, len(files) - 1)]
            content = ''.join(random.choice(string.digits) for _ in range(step))
            if name in contents:
                if (len(contents[name] + content) >= max_filesize):
                    continue
                contents[name] += content
                qemu.execute('''echo -n "%s" >> test/%s'''%(content, name))
            else:
                contents[name] = content
                qemu.execute('''echo -n "%s" > test/%s'''%(content, name))
        check_files()

        for name, content in contents.items():
            offset = random.randint(2, len(content))
            qemu.execute("tail --bytes=+%d test/%s | md5sum -"%(offset, name))
            qemu.runtil(hashlib.md5(content[offset-1:].encode("ascii")).hexdigest(), timeout=args.timeout)
            qemu.execute("tail --bytes=+%d test/%s | sha256sum -"%(offset, name))
            qemu.runtil(hashlib.sha256(content[offset-1:].encode("ascii")).hexdigest(), timeout=args.timeout)
            qemu.execute("tail --bytes=+%d test/%s | sha512sum -"%(offset, name))
            qemu.runtil(hashlib.sha512(content[offset-1:].encode("ascii")).hexdigest(), timeout=args.timeout)

        # delete test
        qemu.execute("rmdir test")
        qemu.runtil("rmdir: failed to remove 'test': Device or resource busy", timeout=args.timeout)

        # delete random entrys
        for i in range(32 + random.randint(1, 16) + 32 + random.randint(1, 16)):
            if ((random.randint(0, 1) == 0 and len(files)) or len(dirs) == 0):
                # delete the random dir
                qemu.execute("rmdir test/%s"%(dirs.pop(random.randint(0, len(dirs) - 1))))
            else:
                # delete the random file
                name = random.randint(0, len(files) - 1)
                if (name in contents):
                    contents.pop(name)
                qemu.execute("rm test/%s"%(files.pop(name)))

        # umount the device
        qemu.execute("umount test")

        # mount the device again
        qemu.execute("mount -t yaf /dev/vda test")

        check_directory()
        check_files()

        # umount the device
        qemu.execute("umount test")

        # remove the yaf module
        qemu.execute("rmmod yaf")
        qemu.runtil("cleanup filesystem", timeout=args.timeout)

    except:
        traceback.print_exc()
        ret = -1
    finally:
        if (qemu != None):
            qemu.kill()

    sys.exit(ret)
