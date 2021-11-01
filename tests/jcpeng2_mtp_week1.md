# Manual Test Plan for dlg2/jcpeng2 fa-21-CS242-project week 1

------

- Environment Setup
  - Linux OS
  - Terminal with Xterm alternate screen buffer support


##### Begin by cloning the repository and navigating to the project root folder. Make the main executable by running:
`make`

------
# Test 1 - Create a file
- In the same directory, run the command `./main hello.txt`. 
- The terminal should clear the previous content and display an empty file, plus a bar at the bottom of the screen indicating that you are in `NORMAL` mode.
- Press `:w` ENTER `:q ENTER`. The editor should exit, restoring your terminal state.
- Run `ls`. Verify that a new file `hello.txt` has been created. `cat hello.txt` should return empty.

# Test 2 - Create a file with content
- In the same directory, run the command `./main hello2.txt`. 
- Type `i` to enter Insert Mode. Type 'hello' to see that you can type text. Press ESC to exit insert.
![hello](mtp_images/hello.png)
- Press `:w` ENTER `:q` ENTER.
- Run `ls`. Verify that a new file `hello2.txt` has been created. `cat hello2.txt` should return the string 'hello'. (no trailing newline)

# Test 3 - Edit file with newlines
- In the same directory, run the command `./main hello.txt`. 
- Type `i` to enter Insert Mode. Type 'hello', then press enter and type 'world'. Press ESC.
![helloworld](mtp_images/helloworld.png)
- Press `:w` ENTER `:q` ENTER.
- Run `ls`. Verify that a new file `hello.txt` has been created. `cat hello.txt` should return the string 'hello' and the string 'world', separated by a newline. (no trailing newline)

# Test 4 - Quit without saving
- In the same directory, run the command `./main hello2.txt`. 
- Press the right arrow until you have moved to the end of the line. Type `i` to enter Insert Mode. Press the right arrow key again once to move to the end of the line.
- Press ENTER. Type 'world'. Press ESC to exit insert.
![helloworldagain](mtp_images/helloworld.png)
- Press `:q` ENTER.
- `cat hello2.txt` should return the string 'hello'. (no trailing newline)

# Test 4 - `o` command
- In the same directory, run the command `./main hello2.txt`. 
- Press `o`. This should create a newline and put you in insert mode.
- Type 'world'. Press ESC to exit insert.
![helloworldfinally](mtp_images/helloworld.png)
- Press `:q` ENTER.
- `cat hello.txt` should return the string 'hello' and the string 'world', separated by a newline. (no trailing newline)
