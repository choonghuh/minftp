# minfpt

The Makefile creates two executable files: 
  mftp, the FTP client
  mftpserve, the server.

The server must be running first in order for a client to connect and establish a socket connection. 

Once the client connects to the server's home directory, user may enter some commands from the client side. Available commands are as follows:

  cd PATH - run a system call to bash command cd on the user end
  rcd PATH - cd on the server end. It changes the server's CWD
  ls - run a system call ls on the user end
  rls - ls command on the server end. It retrieves the content of server's CWD
  get PATH - download the PATH's file to the user's CWD
  show PATH - display the first N lines of the PATH's file's content. N is the message buffer and set to 50
  put PATH - upload the PATH's file to the server
  help - to display available commands
  exit - exit the program
