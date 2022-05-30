#!/usr/bin/env python3

import subprocess
import re
import matplotlib.pyplot as plt
import os

pages = 100
frames_min = 3
frames_max = 100
algs = ["Rand", "FIFO", "Custom"]
programs = ["Alpha", "Beta", "Gamma", "Delta"]

frames_list = list(range(frames_min, frames_max+1))
fault_reg = "Page Faults: ([0-9]*)"
read_reg = "Disk Reads: ([0-9]*)"
write_reg = "Disk Writes: ([0-9]*)"

def main():
    if not os.path.exists("tester"):
        os.makedirs("tester")
    figures = 0
    for prog in programs:
        for alg in algs:
            print("Testing {} with {}".format(prog, alg))
            faults = []
            reads = []
            writes = []
            for frames in frames_list:
                command = "./virtmem {} {} {} {}".format(pages, frames, alg.lower(), prog.lower())
                p = subprocess.Popen([command], stdout=subprocess.PIPE, shell=True)
                ret = p.stdout.read().decode('utf-8')
                faults.append(int(re.search(fault_reg, ret).group(1)))
                reads.append(int(re.search(read_reg, ret).group(1)))
                writes.append(int(re.search(write_reg, ret).group(1)))
            plt.figure(figures)
            plt.plot(frames_list, faults, label = alg)
            plt.figure(figures+1)
            plt.plot(frames_list, reads, label = alg)
            plt.figure(figures+2)
            plt.plot(frames_list, writes, label = alg)
        plt.figure(figures)
        plt.title("{} Page Faults".format(prog))
        plt.xlabel("Frames")
        plt.ylabel("Page Faults")
        plt.legend()
        plt.savefig("tester/{}_page_faults.png".format(prog))
        plt.figure(figures+1)
        plt.title("{} Reads".format(prog))
        plt.xlabel("Frames")
        plt.ylabel("Reads")
        plt.legend()
        plt.savefig("tester/{}_reads.png".format(prog))
        plt.figure(figures+2)
        plt.title("{} Writes".format(prog))
        plt.xlabel("Frames")
        plt.ylabel("Writes")
        plt.legend()
        plt.savefig("tester/{}_writes.png".format(prog))
        figures += 3

if __name__ == "__main__":
    main()

