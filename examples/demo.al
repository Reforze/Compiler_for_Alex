
// ── 1. Факториал в  while
int n = 5;
int result = 1;
int i = 1;
while (i <= n) {
    result = result * i;
    i = i + 1;
}

print(result);

char letter = 'A';
print(letter);

// ── 3. Bool variable and if/else ─────────────────────────────────────────────
bool flag = true;
int val = 0;
if (flag) {
    val = 42;
} else {
    val = 0;
}
print(val);

int a = 12;   // binary: 1100
int b = 10;   // binary: 1010
print(a & b); // AND  = 1000 = 8
print(a | b); // OR   = 1110 = 14
print(a ^ b); // XOR  = 0110 = 6

int sum = 0;
int k = 1;
while (k  10) {
    sum = sum + k;
    k = k + 1;
}
// Sum 1..10 = 55
print(sum);

// ── 6. Logical operators ──────────────────────────────────────────────────────
bool p = true;
bool q = false;
int x = 0;
if (p || q) {
    x = 1;
}
print(x);

if (p && q) {
    x = 99;
} else {
    x = 0;
}
print(x);

// ── 7. Comparison and equality ────────────────────────────────────────────────
int m = 7;
int check_val = 0;
if (m > 5) {
    check_val = 1;
}
print(check_val);
