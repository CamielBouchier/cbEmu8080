8080_ex1.mac and 8080_ex1.hex come from :
https://web.archive.org/web/20151006085348/http://www.idb.me.uk/sunhillow/8080.html

The *.mac I cannot assemble but that's OK, the hex is useable.

It needs basic CP/M support to write to console.
That's done through my own cpm_support.asm assembled into cpm_support.hex.

8080_ex1.hex and cpm_support.hex are merged into 8080_exerciser.hex which is
then used as test.

Further monkey patched by replacing C3 00 00 ( JMP 0000 ) by 76 00 00 ( HLT )
(and the checksum on that line to 79 ).
Then the test is run once after which the program is halted.
