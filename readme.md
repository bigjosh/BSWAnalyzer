# MSP430 Bi-Spy-wire (BSW) Low-Level Analyzer

This is a low-level anaylizer for the Salae Logic Analizer.

BSW is a two pin interface that is used to program and debug TI's MSP430 MCU's. It is an encapulation layer that it takes the normal four JTAG pins and serializes them into two pins to save space at the cost of being slower and more complicated. 

This analyizer only decodes the BSW signals into the standard JTAG signals. You can then use a high level analizyer to decode those. 

## Pin names

BSW calls the signals `tck` and `dio`. In the datasheets the actual pins can have different names. 

`TEST`=`TST`=`BSWTCK`=`tck`

`RESET`=`RST`=`BSWTDIO`=`dio`

## Limitations

I do not even try to decode the TCLK signal becuase I don't think you can do it reliably without knowing the MCU internal state when the trace started.

I ignore error cases when the DIO line is low for longer than 7us. According to the docs, this disables the interface but they do not seem to tell you what cases can re-enable the interface and what state it comes back to.

## Install 
Download the latest release from this repo and copy the files to the directory that you currently have set for Customer Low Level Analizers in Preferences...

![image](https://github.com/bigjosh/BSWAnalyzer/assets/5520281/5f073f72-e8c3-47a0-8b22-8adfa809d303)

## Why

I had some MSP430s that were failing durring production programming so I wanted to see what the MSP430Flasher software was doing. Note that this showed that it is doing a lot of stuff and wasting lots of time, so you could write a much faster programming system.

## Problems

Please open an issue in this repo. You can also reach me [here](http://josh.com/contact.html).
