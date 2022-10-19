MicroPL
======
Change MMIO power limit of your Intel CPUs through a command line interface. By design this **will not** overwrite MSR values, so if MSR values are set low enough, MMIO power limits will not be followed by the CPU.

I am not responsible for any damage caused by this program. Use at your own risk.

This **will not** work on AMD CPUs (pretty obvious).

> Usage: mmio [OPTION]...
>
>/h, --help                      displays the help message and exit
> 
>/l, --power-limit-1 <number>    set the power limit 1 register in watts. range: 5 - 255
> 
>/s, --power-limit-2 <number>    (optional) set the power limit 2 register in watts. range: 5 - 255)