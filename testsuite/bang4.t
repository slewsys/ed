.! y=0 \
   for i in 1 2 3 4 5; do \
       x=$(expr $i \* $i) \
       y=$(expr $y + $x) \
       echo "${i}^2 == $x" \
   done \
   echo "1^2 + ... + 5^2 == $y == 5(5 + 1)(2*5 + 1)/6"
0a
==========
.
$i
----------
.
