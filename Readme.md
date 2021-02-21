AdvancePlayer 0.3


portaudio19-dev is needed.

to run: run the executable in CLI with the *.bin (ripped with AdvanceRipper) passed as an argument. If no file is specified, it will look for "SA2.bin" in the directory

if a file is passed as an argument, the second argument will be the song number (first song if not defined). Putting any 3rd argument will trigger infinite looping.

Not all GBA commands and features are implemented yet. This is based off of my SDAT player and is still a WIP.

Changes in Version 0.3:
-Some code cleanup
-GB Noise

Changes in Version 0.2:
-General fixes and improvements
-Switched over to floats for some calculations

Bugs:
-Wave channel sounds more crackly now
