#
# Trim spaces; copy line, then replace first (-) copy with resulting length.
s;^[[:space:]]*;;
s;[[:space:]]*$;;
t
-! tr -d '\n' | wc -c
#
# Wrap length in printf command that generates spaces for centering.
s;^;-r ! printf "\\%$(( (80 - ;
s;$;) / 2 ))s\\n" ''
#
# Move printf command to register No. 1 and run it.
m>1
@1
#
# Join the line of spaces and trimmed text.
.,+j
