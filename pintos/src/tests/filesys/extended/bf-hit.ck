# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_USER_FAULTS => 1, [<<'EOF']);
(bf-hit) begin
(bf-hit) invalidate cache
(bf-hit) create "a"
(bf-hit) open "a"
(bf-hit) write to "a"
(bf-hit) close "a"
(bf-hit) open "a"
(bf-hit) read from "a"
(bf-hit) close "a"
(bf-hit) open "a"
(bf-hit) read from "a"
(bf-hit) hit rate must increase
(bf-hit) close "a"
(bf-hit) end
bf-hit: exit(0)
EOF
pass;
