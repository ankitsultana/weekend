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

pub fn myDump(data: anytype) void {
    const T = @TypeOf(data);
    inline for (@typeInfo(T).Struct.fields) |field| {
        std.debug.print("{any}\n", .{@field(data, field.name)});
    }
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

    try list.append(3);
    try list.append(4);
    try list.append(2);
    try list.append(3);
    try list.append(4);
    const myType: type = myFunThatReturnsType();
    _ = myType;

    var rb: *ringbuffer.RingBuffer = try ringbuffer.create(4);
    for (list.items) |item| {
        std.debug.print("Item={d}\n", .{item});
        try rb.add(item);
    }
    rb.dump();
    std.debug.print("Last={any}\n", .{rb.idx});
}
