const std = @import("std");

pub fn printer() void {
    std.debug.print("I just ran.. sleeping for 1 second\n", .{});
    std.time.sleep(1_000_000_000);
}

pub fn spawnThread() !void {
    var t: std.Thread = std.Thread.spawn(.{}, printer, .{}) catch |err| {
        std.log.err("{any}\n", .{@errorName(err)});
        return err;
    };
    t.join();
    std.debug.print("Joined with thread\n", .{});
}

pub fn playCondition() !void {
    var mutex: std.Thread.Mutex = std.Thread.Mutex{};
    var condition: std.Thread.Condition = std.Thread.Condition{};
    condition.wait(&mutex);
}

pub fn main() !void {
    try spawnThread();
}
