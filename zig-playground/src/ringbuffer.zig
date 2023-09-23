// Learning
const std = @import("std");

pub const RingBuffer = struct {
    const Self = @This();

    pub fn add(self: Self, x: i32) void {
        _ = self;
        std.debug.print("Adding: {d}", .{x});
    }
};
