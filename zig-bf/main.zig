const warn = @import("std").debug.warn;
const sub = @import("std").math.sub;

fn seekBack(src: []const u8, srcptr: u16) !u16 {
    var depth:u16 = 1;
    var ptr: u16 = srcptr;
    while (depth > 0) {
        ptr = sub(u16, ptr, 1) catch return error.OutOfBounds;
        switch(src[ptr]) {
            '[' => depth -= 1,
            ']' => depth += 1,
            else => {}
        }
    }
    return ptr;
}

fn seekForward(src: []const u8, srcptr: u16) !u16 {
    var depth:u16 = 1;
    var ptr: u16 = srcptr;
    while (depth > 0) {
        ptr += 1;
        if (ptr >= src.len) return error.OutOfBounds;
        switch(src[ptr]) {
            '[' => depth += 1,
            ']' => depth -= 1,
            else => {}
        }
    }
    return ptr;
}

pub fn bf(src: []const u8, storage: []u8) !void {
    var memptr: u16 = 0;
    var srcptr: u16 = 0;
    while (srcptr < src.len) {
        switch(src[srcptr]) {
            '+' => storage[memptr] +%= 1,
            '-' => storage[memptr] -%= 1,
            '>' => memptr += 1,
            '<' => memptr -= 1,
            '[' => if (storage[memptr] == 0) srcptr = try seekForward(src, srcptr),
            ']' => if (storage[memptr] != 0) srcptr = try seekBack(src, srcptr),
            '.' => warn("{c}", storage[memptr]),
            else => {}
        }
        srcptr += 1;
    }
}

pub fn main() void {
    var mem = []u8{0} ** 30000;
    const hello_world = "++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.";
    bf(hello_world, mem[0..]) catch {};
}

const assert = @import("std").debug.assert;

test "+" {
    var mem = []u8{0};
    const src = "+++";
    bf(src, mem[0..]);
    assert(mem[0] == 3);
}

test "-" {
    var mem = []u8{0};
    const src = "---";
    bf(src, mem[0..]);
    assert(mem[0] == 253);
}

test ">" {
    var mem = []u8{0} ** 5;
    const src = ">>>+++";
    bf(src, mem[0..]);
    assert(mem[3] == 3);
}

test "<" {
    const mem = @import("std").mem;

    var storage = []u8{0} ** 5;
    const src = ">>>+++<++<+";
    bf(src, storage[0..]);
    assert(mem.eql(u8, storage, []u8{ 0, 1, 2, 3, 0 }));
}

test "[] skips execution and exits" {
    var storage = []u8{0} ** 3;
    const src = "+++++>[>+++++<-]";
    bf(src, storage[0..]);
    assert(storage[0] == 5);
    assert(storage[1] == 0);
    assert(storage[2] == 0);
}
