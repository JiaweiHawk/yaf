#!/usr/bin/python3
# -*- coding:utf-8 -*-
import argparse
import coloredlogs
import logging
import os
import select
import subprocess
import sys
import traceback

logger = logging.getLogger(__file__)

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
        self._runtil(":~#")

    def _read(self, timeout=1) -> None:
        rset, _, _ = select.select([self.proc.stdout.fileno()], [], [], timeout)

        self.proc.poll()
        if (self.proc.returncode != None):
            raise QemuTerminate

        self.outbytes += os.read(self.proc.stdout.fileno(), 4096)
        res = self.outbytes.decode("utf-8", "ignore")
        self.output += res
        self.outbytes = self.outbytes[len(res.encode("utf-8")):]

    def _runtil(self, string:str) -> None:
        cursor = 0
        while(True):
            self._read()
            if (string in self.output):
                break
            print(self.output[cursor:], end="", flush=True)
            cursor = len(self.output)

        idx = self.output.find(string) + len(string)
        print(self.output[cursor:idx], end="", flush=True)
        self.output = self.output[idx:]

    def _write(self, buf:str) -> None:
        buf:bytes = buf.encode("utf-8")
        self.proc.stdin.write(buf)
        self.proc.stdin.flush()

    def execute(self, command:str) -> None:
        self._write(command + "\n")
        self._runtil(":~#")

    def kill(self) -> None:
        self.proc.kill()
        self.proc.wait()

if __name__ == "__main__":
    ret = 0
    qemu:Qemu = None

    coloredlogs.install(level=logging.INFO, logger=logger)

    parser = argparse.ArgumentParser(description="yaf test script")
    parser.add_argument("--command", action="store",
                        type=str, required=True,
                        help="command to boot up qemu")
    args = parser.parse_args()

    try:
        logger.info("Boot up the Qemu")
        qemu = Qemu(command=args.command)

        logger.info("insmod the yaf module")
        qemu.execute("insmod /mnt/shares/yaf.ko")

        logger.info("format the disk device")
        qemu.execute("/mnt/shares/mkfs /dev/vda")

        qemu.execute("mkdir -p test")

        logger.info("mount the device")
        qemu.execute("mount -t yaf /dev/vda test")

        logger.info("umount the device")
        qemu.execute("umount test")

        logger.info("remove the yaf module")
        qemu.execute("rmmod yaf")
    except:
        traceback.print_exc()
        ret = -1
    finally:
        if (qemu != None):
            qemu.kill()

    print("")
    if (ret == 0):
        logger.info("test success")
    else:
        logger.error("test failed")
    sys.exit(ret)
