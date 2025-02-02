Assignment 2 Q/A:

1) Yes, externalizing "getstudent()" into its own function is a good design strategy because it allows code reusabiltiy and makes it easier to 
maintain. It abstracts the logic for retrieving a student and can be allowed in multiple other functions which removes code duplication. 

2) Returning a pointer to a local variable in "get_student()" is a bad idea because the variable is stored on the stack and gets deallocated 
when the function returns which can leads to undefined behavior since the pointer will point to an invalid memory location.

3) The alternative implementation using malloc() works as it allocates memory on the heap which ensures the returned pointer remains valid after the function exits. 

4) The "ls" command reports the logical file size while "du" reports the actual disk usage. The difference occurs because Linux supports sparse files, meaning large portions of the file that are unallocated do not consume actual disk space.

Other Questions:

Q: Why Did the Disk Usage Remain Unchanged for Some Additions But Increased at ID=64?

Ans: For IDs 1, 3, and 63, student records were placed in memory without filling a large gap, so the actual disk space remained unchanged. 
However, when student ID=64 was added, the file expanded to a new block boundary which caused Linux to allocate additional disk space.

Q: Why Did Adding ID=99999 Expand the File to 6400000 Bytes, But du Shows Only 12K Used?

Since ID=99999 is a very large number, lseek() moves the file pointer far beyond existing data, creating a massive hole in the file. This makes ls report a size of 6400000 bytes, but the actual disk storage (du) remains small (12K) because most of the file consists of unallocated sparse regions.

