MicroPL
======
Change MMIO power limit of your Intel CPUs through a command line interface. By design this **will not** overwrite MSR values.

This **will not** work on AMD CPUs (pretty obvious).

> Usage: mmio [OPTION]...
>
>/h, --help                      display this help and exit
> 
>/l, --power-limit-1 <number>    set the power limit 1 register in watts. range: 5 - 255
> 
>/s, --power-limit-2 <number>    (optional) set the power limit 2 register in watts. range: 5 - 255)