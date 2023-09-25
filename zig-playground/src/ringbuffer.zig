// Learning
const std = @import("std");

pub const RingBuffer = struct {
    capacity: usize,
    buffer: []i32,
    idx: usize,

    pub fn add(self: *RingBuffer, x: i32) !void {
        self.buffer[self.idx] = x;
        self.idx += 1;
        self.wrapAround();
    }

    pub fn wrapAround(self: *RingBuffer) void {
        if (self.idx == self.capacity) {
            self.idx = 0;
        }
    }

    pub fn dump(self: *RingBuffer) void {
        for (self.buffer) |item, index| {
            std.debug.print("rb: {d},{d}\n", .{ index, item });
        }
    }
};

pub fn create(cap: usize) !*RingBuffer {
    const array_list: []i32 = try std.heap.c_allocator.alloc(i32, cap);
    var ring_buffer: *RingBuffer = try std.heap.c_allocator.create(RingBuffer);
    ring_buffer.capacity = cap;
    ring_buffer.buffer = array_list;
    ring_buffer.idx = 0;
    return ring_buffer;
}
