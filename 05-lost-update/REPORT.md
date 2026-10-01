# COMP 310 — Operating Systems
## Week 5 — A5: The lost update

## Prediction
Given 4 threads each running 70000 iterations with a spin of 31 with no mutex to lock them, the final value is hard to predict, because it will vary per run, OS, hardware, etc. A decent amount will almost certainly be lost irreverant of any of the above variables. I would guess that the final count will be around the average value of 105,000, with 175,000 average losses. This is assumptive of a loss of 5/8ths, or 62.5 percent. I chose this with a baseline assumption that between two threads, roughly 50 percent would be lost (4/8ths), and that because there are double the threads, the rate of loss would go up somewhat proportionately due to the increased chance for multiple threads to read the counter variable at the same time before writing. The loss will happen exactly at the aforementioned point. A counter++ operation is not atomic, it consists of a read, an add, and a store at the machine level. Races happen when two or more threads read the same variable in between each others' atomic level operations. If two threads read the same value, write an addition of 1 to the same value and both store that, the machine has spent 2 iterations, but the counter value has increased by only 1. In the counter.c file, the lines that need to be locked where all of this reading, adding, and storing are happening are lines 49-51. Note this is before making any changes to the file.

long seen = counter; // read, store 
        spin(A5_SPIN_OVERRIDE);
        counter = seen + 1; // read, add, store 

## What

*One paragraph: what you measured and what you changed.* //DO AT END??

## Results

### The race, over 30 trials
Median losses: 202738

Minimum losses: 196918

Maximum losses: 207748

This spread indicates that over 30 trials, 70.33-74.20% of values were lost, every single time. No trials came out correct. The average of the min and max is 202,333, which is within 500 losses of the median value, indicating no major outliers in either direction and relatively consistent results throughout this set of trials. In practice, a 75% loss rate would indicate that for every iteration, the same value was read by all four threads. Given a rough average of 72% lost per trial, only around 3% of iterations were not read by all four threads within the read, add, store window. 

### Where the window is

*The read line and the write line, and what another thread does in between.
Then: why the loss comes from two reads returning the same value, not from the
write.* // DO AFTER FIX

### The intervention

I chose a spin value of 100, over triple the original value of 31. I predict that this will increase the rate of loss, likely almost to 75%, because it is increasing the time spent between a read and a write operation. This increases the time window for, and therefore likelihood that two or more threads will read the counter value before they can increment it and store it back in the register. 

Post intervention results:

Median loss: 208692

Minimum loss: 197291

Maximum loss: 210797 

This is in line with what I predicted, an increase in all measures. 

### After the fix, and what it cost

*The worst case across your 30 fixed trials. Then the section 2.5 cost rule,
step by step: both trimmed medians and trimmed ranges, and the branch you
landed in.*

### Prediction vs measurement

*Compare your Prediction section against what happened. Where you were wrong,
say what you now think the right reasoning is.*

### The fix that passes check 9

*Section 2.6, the 3.0 challenge. Write "not attempted" here if you did not do
it. Otherwise: which suspect is broken and how you know, where its window is,
why a 30-run check at normal timing usually passes it, the widths you measured
with `make detect` and what each gave, and the width and number of runs your
check uses and why.*

## Citations

*Anything you drew on — a `man` page counts, and so does a classmate's
suggestion. See `bibliography.pdf`. If you looked nothing up, write "course
material only". If you used an AI assistant at any point, say so here and say
what for — the syllabus does not permit them on submitted work, and saying so
plainly is always better than leaving it out.*

## Answers

threads: 4
iterations: 70000
lost_median: 202738
read_line:
write_line:
intervention_spin:
fixed_lost_max:
racy_suspect:
suspect_read_line:
suspect_write_line:
stress_width:
