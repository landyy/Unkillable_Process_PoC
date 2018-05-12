# Unkillable_Process_PoC

## Usage
1.) Download Debug-PoC.exe

2.) Run as Administrator

3.) Look in task manager, procexp, or tasklist to view that a bunch of notepads have been spawned under random processes

## Some Basic Insight
This is a simple PoC to demonstrate using Extended Startup attributes with CreateProcess() and DebugActiveProcess() you can create an "unkillable" process. I use quotes because if you find the parent, you can kill the process, but with the right Extended Startup Attributes (specifically PROC_THREAD_ATTRIBUTE_PARENT_PROCESS) you can specify a different parent process for CreateProcess to start the process under. For those unaware, PPID (Parent Process ID) in windows means absolutely nothing.

DebugActiveProcess() can be used to actually make the process unkillable because of the permissions on certain threads when a process in actively being debugged by another process. Even as administrator, you can not close these threads, only SYSTEM can. 

## About the program
This program simply spawns a bunch of notepads under different processes on the system and debugs them so you can't kill them (unless you have a SYSTEM shell). Also worth noting: the program that is being debugged will not actually be running, since it is waiting for debug commands. Lastly, Im not a CS major so don't laugh at my code. Thanks.

## Future Work
- Integrate into a service
- Get Working with Dll for process injection
- Find a way to allow for execution of payloads even if it is being debugged

