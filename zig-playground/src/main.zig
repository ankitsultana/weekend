const std = @import("std");
const ringbuffer = @import("ringbuffer.zig");

pub fn myFunThatReturnsType() type {
    return struct {
        const Self = @This();

        fn print() void {
            std.debug.print("Called!");
        }
    };
}

pub fn main() !void {
    var age: u8 = 8;
    age = 10;

    const Human = struct {
        name: []const u8,
        age: u8,
    };
    const jeff = Human{
        .name = "jeff",
        .age = 30,
    };
    _ = jeff;

    var allocator = std.heap.page_allocator;
    var list = std.ArrayList(i32).init(allocator);
    defer list.deinit();

    try list.append(42);
    std.debug.print("Hello a list will follow\n", .{});
    for (list.items) |index, item| {
        std.debug.print("Index: {d}", .{index});
        _ = item;
        // std.debug.print("Value: ", item);
    }
    const myType: type = myFunThatReturnsType();
    _ = myType;

    var rb: ringbuffer.RingBuffer = ringbuffer.RingBuffer{};
    for (list.items) |index, item| {
        _ = item;
        rb.add(index);
    }
}
