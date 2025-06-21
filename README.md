# snd

Basic windows cli sound device control. Lets you:

* set active sound device
* (un)mute
* set/add/subtract volume

When no device is specified, the tool operates on the current active device.  
`snd.exe [D:nameofDevice] verb [argument]`  
Examples:  
`snd +.1` increases volume by 10%  
`snd -.5` reduces by 50%  
`snd 1` sets volume to max  
`snd D:Rea 1` does a partial match on the text following D: for device, matching "Realtek(R) Audio" on my system, and sets volume to max.  
  
Verbs:  
volume \[+/-\]{0.0-1.0}; default action if no verb is specified  
setactive; sets the active output device.  
list; lists all current connected speakers  
help; needs actually implemented.  



Could have more done with it - mic support, a proper help page, etc, but this was made for my own use- i couldn't find a windows equivalent to the linux utils that made me want this. PRs welcome.
