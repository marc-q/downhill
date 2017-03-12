# How to create a test case:
* Create a file containing the tests as markdown inside the corresponding folder from *testcases*.
* Create a file containing the expected output inside the folder from above.

After creating the files, add the test case to the others by adding this line to `test.c`.
```C
t += test_file ("FOLDER_FILENAME", "testcases/FOLDER/FILENAME.md", "testcases/FOLDER/FILENAME.html");
```
Then increment the number of test cases in this line.
```C
test_print_summary (t, NUMBEROFTESTCASES);
```
