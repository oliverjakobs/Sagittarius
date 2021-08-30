# Test program
let breakfast = "beignets"
let beverage = "cafe au lait"
breakfast = "beignets with " + beverage

print breakfast;

# Test scope
let a = 7
{
    let a = 42
    print a
}
print a