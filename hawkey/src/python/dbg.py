import time

def clock(subtract=0):
    return time.clock() - subtract

def show(subtract, msg="dbg.py: measured time: "):
    time = clock(subtract)
    print "%s %f" % (msg, time)
