1. fgets() reads input until a newline character (\n) is encountered or an end-of-file (EOF) is detected, which matches how shell commands are typically entered—one command per line. It is a good choice because it is safe (prevents buffer overflows) and efficient for line-based input, ensuring that we don’t read beyond the buffer size.


2. Using malloc() allows for dynamic memory allocation, giving more flexibility compared to a fixed-size array. While fixed-size arrays allocate memory on the stack, malloc() allocates memory on the heap, allowing the program to handle variable-length input and larger buffers if needed. This helps manage memory efficiently and avoids stack overflows for large input sizes.

3. Trimming leading and trailing spaces ensures that commands are parsed and executed correctly. Without trimming, the shell might misinterpret commands. For example, " ls"  might not be recognized as "ls". This can lead to unintended arguments being passed and issues with command comparison logic, especially when checking for built-in commands like exit or cd.
 
 4. 

 ANS: Redirection Examples:
 
 a)  ls > output.txt
Redirects STDOUT to output.txt, meaning the output won’t appear in the terminal. The shell must close STDOUT, open the file for writing, and handle file descriptor duplication using dup2().

b) echo "New Entry" >> log.txt
Appends the output to log.txt instead of overwriting it. The shell must open the file in append mode (O_APPEND) and manage file permissions to prevent data loss.

c) sort < unsorted.txt
Redirects STDIN to read from unsorted.txt. The shell must ensure proper file handling, check if the file exists, and handle permission errors.

Difference Between Redirection and Piping:

ANS: Redirection involves sending input/output to or from a file (e.g., ls > output.txt). In contrast, piping (|) connects the output of one command directly to the input of another, allowing for real-time data transfer between commands without using intermediate files (e.g., ps aux | grep python). The key difference is that redirection interacts with files, while piping connects processes in memory.

Why Keep STDOUT and STDERR Separate?

ANS: Separating STDOUT (standard output) and STDERR (standard error) allows for a clear distinction between normal program output and error messages. This separation is important because it makes error handling easier and provides redirection flexibility. For example, in: ls > output.txt 2> error.log

Regular output is saved to output.txt, while errors are redirected to error.log. This helps in debugging without mixing error messages with regular output.

How Should Our Custom Shell Handle Command Errors?

ANS: Our custom shell should handle command errors by:

a.  Detecting failures using exit codes 
b.  Displaying appropriate error messages by reading from STDERR.
c.  Allowing redirection of both STDOUT and STDERR when needed, using:
        command > output.txt 2>&1

This merges both outputs into output.txt. To implement this, the shell must handle file descriptor duplication (using dup2()) to redirect both STDOUT (file descriptor 1) and STDERR (file descriptor 2) to the same file or stream. This helps in logging both outputs for debugging or reporting.

