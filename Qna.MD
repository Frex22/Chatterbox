**something I think we will need**
WHat are Null Terminators:

In C, strings are represented as arrays of characters that end with a special character called the **null terminator** (`'\0'`). Here’s why it’s important and why we subtract 1 from `SHM_SIZE`:

- **Null Terminator Role:**  
  The null terminator signals the end of the string. Standard library functions (like `printf`, `strlen`, and `strcpy`) rely on it to determine where the string ends. Without it, these functions might read past the intended end of the string, causing undefined behavior or memory errors.

- **Buffer Size Consideration:**  
  When copying a string into a buffer (or shared memory region), you must ensure that the buffer has enough space for all the characters **plus** the null terminator.  
  - For example, if `SHM_SIZE` is the total size of the buffer, using `SHM_SIZE - 1` as the length in `strncpy` leaves space for adding the `'\0'` at the end.

- **Preventing Overflows:**  
  If you copied `SHM_SIZE` characters without leaving room for the null terminator, you might overwrite memory outside the buffer, or the string might not be properly terminated, leading to bugs or security vulnerabilities.

In summary, subtracting 1 ensures there's space for the null terminator, which is essential for safely handling C strings.