/* This test checks that the stack is properly extended even if
   the first access to a stack location occurs inside a system
   call.

   From Godmar Back. */

#include <string.h>
#include <syscall.h>
#include "tests/vm/sample.inc"
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  int handle;
  int slen = strlen (sample);
  char buf2[65536];

  /* Write file via write(). */
  // Create a file "sample.txt" with length slen
  CHECK (create ("sample.txt", slen), "create \"sample.txt\"");
  // Open it and make sure the file handle is over 1
  CHECK ((handle = open ("sample.txt")) > 1, "open \"sample.txt\"");
  // writes the file and makes sure that entire length is written
  CHECK (write (handle, sample, slen) == slen, "write \"sample.txt\"");
  // closes the file handler
  close (handle);

  msg("lols %s", sample);
  /* Read back via read(). */
  // reopens and wants handle over 1
  CHECK ((handle = open ("sample.txt")) > 1, "2nd open \"sample.txt\"");
  msg("lols2");
  // tries reading it
  CHECK (read (handle, buf2 + 32768, slen) == slen, "read \"sample.txt\"");
  msg("lols3");

  msg("sample is %s buffer is %s", sample, buf2+32768);
  CHECK (!memcmp (sample, buf2 + 32768, slen), "compare written data against read data");
  close (handle);
}
