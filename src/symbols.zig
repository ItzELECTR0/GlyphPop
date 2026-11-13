pub const Group = struct {
    name: [*:0]const u8,
    codepoints: []const u32,
};

const Range = [2]u32;

/// Inclusive ranges. Unassigned codepoints and ones the font lacks are filtered by the host.
fn expand(comptime ranges: []const Range) []const u32 {
    @setEvalBranchQuota(100_000);
    comptime var n: usize = 0;
    inline for (ranges) |r| n += r[1] - r[0] + 1;
    comptime var out: [n]u32 = undefined;
    comptime var i: usize = 0;
    inline for (ranges) |r| {
        comptime var cp = r[0];
        inline while (cp <= r[1]) : (cp += 1) {
            out[i] = cp;
            i += 1;
        }
    }
    const final = out;
    return &final;
}

pub const groups = [_]Group{
    .{ .name = "Punctuation", .codepoints = expand(&.{
        .{ 0x00A1, 0x00A1 }, .{ 0x00A7, 0x00A7 }, .{ 0x00AB, 0x00AB }, .{ 0x00B6, 0x00B7 },
        .{ 0x00BB, 0x00BB }, .{ 0x00BF, 0x00BF }, .{ 0x2010, 0x2027 }, .{ 0x2030, 0x205E },
        .{ 0x2E18, 0x2E18 }, .{ 0x2E2E, 0x2E2E },
    }) },
    .{ .name = "Currency", .codepoints = expand(&.{
        .{ 0x0024, 0x0024 }, .{ 0x00A2, 0x00A5 }, .{ 0x058F, 0x058F }, .{ 0x060B, 0x060B },
        .{ 0x09F3, 0x09F3 }, .{ 0x0E3F, 0x0E3F }, .{ 0x17DB, 0x17DB }, .{ 0x20A0, 0x20C1 },
    }) },
    .{ .name = "Maths", .codepoints = expand(&.{
        .{ 0x00AC, 0x00AC }, .{ 0x00B0, 0x00B1 }, .{ 0x00B2, 0x00B3 }, .{ 0x00B5, 0x00B5 },
        .{ 0x00B9, 0x00B9 }, .{ 0x00BC, 0x00BE }, .{ 0x00D7, 0x00D7 }, .{ 0x00F7, 0x00F7 },
        .{ 0x2070, 0x209C }, .{ 0x2150, 0x218B }, .{ 0x2200, 0x22FF }, .{ 0x27C0, 0x27EF },
    }) },
    .{ .name = "Arrows", .codepoints = expand(&.{
        .{ 0x2190, 0x21FF }, .{ 0x27F0, 0x27FF }, .{ 0x2900, 0x297F }, .{ 0x2B00, 0x2B11 },
    }) },
    .{ .name = "Shapes", .codepoints = expand(&.{
        .{ 0x25A0, 0x25FF }, .{ 0x2B12, 0x2B2F }, .{ 0x2B50, 0x2B59 },
    }) },
    .{ .name = "Letterlike", .codepoints = expand(&.{
        .{ 0x00A9, 0x00A9 }, .{ 0x00AE, 0x00AE }, .{ 0x2100, 0x214F },
    }) },
    .{ .name = "Latin", .codepoints = expand(&.{
        .{ 0x00C0, 0x00D6 }, .{ 0x00D8, 0x00F6 }, .{ 0x00F8, 0x017F },
    }) },
    .{ .name = "Greek", .codepoints = expand(&.{
        .{ 0x0391, 0x03A1 }, .{ 0x03A3, 0x03A9 }, .{ 0x03B1, 0x03C9 },
    }) },
    .{ .name = "Miscellaneous", .codepoints = expand(&.{
        .{ 0x2300, 0x23FF }, .{ 0x2600, 0x26FF }, .{ 0x2700, 0x27BF },
    }) },
    .{ .name = "Box drawing", .codepoints = expand(&.{
        .{ 0x2500, 0x259F },
    }) },
};
