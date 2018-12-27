# shell

This application partially emulates the work of a command language interpreter (shell). The "Bash" was taken as the main reference (see [Bash manual](https://www.gnu.org/software/bash/manual/bash.html)).

Typical input for shell consists of one or more jobs which are separated by newlines('\\n') or colons(';'). A job consists of one or more commands which are arranged in a pipeline and separated by '|'. A command consists of a command name, arguments and input/output files (separated by <, > or >>).

List of supported features:
*  single (') and double (") quotes;
*  special symbols can be escaped with '\\';
*  comments. All text which comes after "#" symbol will be ignored;
*  history of commands. To see the history type "history" in the shell. To run a command from history type "!%command_number%";
*  several commands were implemented: "cd", "pwd", "exit";
*  current command can be interrupted with "ctrl + c" (SIGINT);
*  application is closed in case EOF is encountered (i.e. user presses "ctrl + d").