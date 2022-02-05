# MyShell
A POSIX based shell program.

## Features
- Basic shell commands with argument support.
- Background processes supported using & argument at the end of a command.
- Direct output to files by using "> \<filename>" arguments at the end of a command.
- Take input of data from a file by using "< \<filename>" arguments at the end of a command.
- Utilize pipes by using "|" argument between 2 commands.
- Reads set of startup commands from .CIS3110_profile in the user's home directory. Each command should be on its own line.
- Shell is aware of the current working directory; you can change it using "cd" command. 
- Supports environment variables in commands by using the '$' prefix to the command name.
- Ability to edit environment variables by using "export" command.

## Environment Variables
myShell uses its adds its own environment variables: 
- myPath: Default = /bin. 
- myHISTFILE: Default = ~/.CIS3110_history. 
  - Display contents using "history" command.
- myHOME: Home directory variable.

## Download & Run Information
- Clone the repository into your local POSIX machine.
- Run the "make" command in your terminal to generate the executable "myShell".
- Run the progam using "./myShell" command.

### Note:
- You need to have the GCC compiler and make utility installed on your system.

## Limitations

