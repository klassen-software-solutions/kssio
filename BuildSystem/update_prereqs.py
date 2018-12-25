#!/usr/bin/env python

import json
import os
import platform
import subprocess

#class Dependancy:


if __name__ == '__main__':
    try:
        with open('prereqs.json', 'r') as file:
            deps = json.load(file)
    except IOError:
        print("Could not open prereqs.json. Assuming no dependancies.")
        exit(0)

    numChanged = 0
    cwd = os.getcwd()
    operatingSystem = platform.system()
    dependancyDir = ".prereqs"
    dependancySrcDir = ".prereqs/Sources"
    dependancyOsDir = ".prereqs/" + operatingSystem
    if not os.path.exists(dependancySrcDir):
        print("Creating directory " + dependancySrcDir)
        os.makedirs(dependancySrcDir)
    if not os.path.exists(dependancyOsDir):
        print("Creating directory " + dependancyOsDir)
        os.makedirs(dependancyOsDir)

    print("Updating prerequisites...")
    for dep in deps:
        changed = False
        name = dep["name"]
        print("  " + name + "...")

        # Download/update the sources
        if os.path.exists(dependancySrcDir + "/" + name):
            os.chdir(dependancySrcDir + "/" + name)
            cmd = dep["update-needed"]
            if subprocess.call(cmd, shell=True) == 0:
                print("    no update needed")
            else:
                print("    updating...")
                cmd = dep["update"]
                subprocess.check_call(cmd, shell=True)
                numChanged += 1
                changed = True
            os.chdir(cwd)
        else:
            print("    downloading...")
            os.chdir(dependancySrcDir)
            cmd = dep["download"]
            subprocess.check_call(cmd, shell=True)
            os.chdir(cwd)
            numChanged += 1
            changed = True

        # Build and install
        if changed == True:
            print("    building...")
            os.chdir(dependancySrcDir + "/" + name)
            cmds = dep["build"]
            tmpEnv = os.environ.copy()
            tmpEnv["TARGETDIR"] = cwd + "/" + dependancyOsDir
            for cmd in cmds:
                print("      " + cmd)
                subprocess.check_call(cmd, env=tmpEnv, shell=True)
            os.chdir(cwd)

    print("Prerequisites updated or changed: " + str(numChanged))
    exit(numChanged)
