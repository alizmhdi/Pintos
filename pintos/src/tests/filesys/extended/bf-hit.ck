# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_USER_FAULTS => 1, [<<'EOF']);
(bf-hit) begin
(bf-hit) create "a"
(bf-hit) open "a"
(bf-hit) write 16384 bytes to "a"
(bf-hit) close "a"
(bf-hit) open "a"
(bf-hit) get baseline cache stats
(bf-hit) invalidate cache
(bf-hit) read 16384 bytes from "a"
(bf-hit) get cold cache stats
(bf-hit) close "a"
(bf-hit) open "a"
(bf-hit) read 16384 bytes from "a"
(bf-hit) get new cache stats
(bf-hit) old hit rate percent: 65, new hit rate percent: 81
(bf-hit) close "a"
(bf-hit) end
bf-hit: exit(0)
EOF
pass;
