const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const core = b.createModule(.{
        .root_source_file = b.path("src/core.zig"),
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    core.addAnonymousImport("emoji-test", .{ .root_source_file = b.path("data/unicode/emoji-test.txt") });
    core.addAnonymousImport("annotations", .{ .root_source_file = b.path("data/unicode/annotations-en.xml") });
    core.addAnonymousImport("annotations-derived", .{ .root_source_file = b.path("data/unicode/annotations-derived-en.xml") });

    const lib = b.addLibrary(.{ .name = "glyphpop_core", .root_module = core });
    lib.installHeader(b.path("src/core.h"), "core.h");
    b.installArtifact(lib);

    const tests = b.addTest(.{ .root_module = core });
    b.step("test", "Run core tests").dependOn(&b.addRunArtifact(tests).step);
}
