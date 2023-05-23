# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_USER_FAULTS => 1, [<<'EOF']);
(buffer-cache-hit) begin
(buffer-cache-hit) created file named 'file'.
(buffer-cache-hit) opened the file.
(buffer-cache-hit) wrote random bytes to file.
(buffer-cache-hit) closed file.
(buffer-cache-hit) opened the file.
(buffer-cache-hit) read the file for the first time.
(buffer-cache-hit) closed file.
(buffer-cache-hit) stored hit and miss rate for the first read.
(buffer-cache-hit) opened the file.
(buffer-cache-hit) read the file for the second time.
(buffer-cache-hit) closed file.
(buffer-cache-hit) stored hit and miss rate for the second read.
(buffer-cache-hit) hit rate increased in the second read.
(buffer-cache-hit) removed the file.
(buffer-cache-hit) end
buffer-cache-hit: exit(0)
EOF
pass;