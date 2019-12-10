
print "----------------------------------------------------";
print "Test precedence";
print "----------------------------------------------------";
print "[Test 1]:";
print "* has higher precedence than +. Expect: 14";
print 2 + 3 * 4;

print "[Test 2]:";
print "* has higher precedence than -. Expect: 8";
print 20 - 3 * 4;

print "[Test 3]:";
print "/ has higher precedence than +. Expect: 4";
print 2 + 6 / 3;

print "[Test 4]:";
print "/ has higher precedence than -. Expect: 0";
print 2 - 6 / 3;

print "[Test 5]:";
print "< has higher precedence than ==. Expect: true";
print false == 2 < 1;

print "[Test 6]:";
print "> has higher precedence than ==. Expect: true";
print false == 1 > 2;

print "[Test 7]:";
print "<= has higher precedence than ==. Expect: true";
print false == 2 <= 1;

print "[Test 8]:";
print ">= has higher precedence than ==. Expect: true";
print false == 1 >= 2;

print "[Test 9]:";
print "1 - 1 is not space-sensitive. Expect: 0 for all";
print 1 - 1;
print 1 -1;
print 1- 1;
print 1-1;

print "[Test 10]:";
print "Using () for grouping. Expect: 4";
print (2 * (6 - (2 + 2)));
