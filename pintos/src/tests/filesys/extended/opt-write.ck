# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(opt-write) begin
(opt-write) create "a"
(opt-write) open "a"
(opt-write) write 102400 bytes to "a"
(opt-write) invalidate cache
(opt-write) write 102400 bytes to "a"
(opt-write) old reads: 43, old writes: 825, new reads: 48, new writes: 1372
(opt-write) close "a"
(opt-write) end
EOF
pass;