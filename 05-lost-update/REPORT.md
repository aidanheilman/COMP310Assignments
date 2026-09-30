# COMP 310 — Operating Systems
## Week 5 — A5: The lost update

## Prediction

*Write this before you run anything.*

- Final value I expect, and why:
- Fraction of updates I expect to be lost:
- Where I think the loss happens:

## What

*One paragraph: what you measured and what you changed.*

## Results

### The race, over 30 trials

*From `make summary` for `bench/trials.csv`: the median lost updates, the lowest
and highest, and what that spread means. If a trial came out correct, say so and
say what that does not show.*

### Where the window is

*The read line and the write line, and what another thread does in between.
Then: why the loss comes from two reads returning the same value, not from the
write.*

### The intervention

*The width you chose, the direction you predicted and why — written before you
ran it — then the measured result from `bench/trials-intervention.csv`.*

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

threads:
iterations:
lost_median:
read_line:
write_line:
intervention_spin:
fixed_lost_max:
racy_suspect:
suspect_read_line:
suspect_write_line:
stress_width:
