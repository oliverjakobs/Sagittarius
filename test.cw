fun outer() {
  var x = "outside";
  print "Test";
  fun inner() {
    print "Test2";
  }
  inner();
}

outer();