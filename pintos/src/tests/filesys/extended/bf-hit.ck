# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_USER_FAULTS => 1, [<<'EOF']);
(bf-hit) begin
(bf-hit) created file named a.
(bf-hit) opened the file.
(bf-hit) wrote random bytes to file.
(bf-hit) closed file.
(bf-hit) opened the file.
(bf-hit) read the file for the first time.
(bf-hit) closed file.
(bf-hit) stored hit and miss rate for the first read.
(bf-hit) opened the file.
(bf-hit) read the file for the second time.
(bf-hit) closed file.
(bf-hit) stored hit and miss rate for the second read.
(bf-hit) hit rate increased in the second read.
(bf-hit) removed the file.
(bf-hit) end
bf-hit: exit(0)
EOF
pass;
