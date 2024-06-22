#!/usr/sbin/dtrace -s

#pragma D option quiet

pid$target:::entry
{
    self->ts = timestamp;
}

pid$target:::return
/self->ts/
{
    @time[pid, execname] = sum((timestamp - self->ts) / 1000);
    self->ts = 0;
}

dtrace:::END
{
    printa("PID: %d Executable: %s Total CPU time (us): %d\n", @time);
}

