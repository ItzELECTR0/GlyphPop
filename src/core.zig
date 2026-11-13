const std = @import("std");
const symbols = @import("symbols.zig");
const kaomoji = @import("kaomoji.zig");

const emoji_test = @embedFile("emoji-test");
const annotations = @embedFile("annotations");
const annotations_derived = @embedFile("annotations-derived");

const no_tones = std.math.maxInt(u32);

pub const Emoji = struct {
    text: [:0]const u8,
    name: [:0]const u8,
    keywords: []const u8,
    group: u8,
    /// Index of the first of five uniformly toned variants in `Db.tone_variants`, or `no_tones`.
    tones: u32 = no_tones,
};

const Annotation = struct {
    name: []const u8 = "",
    keywords: []const u8 = "",
};

pub const Db = struct {
    arena: std.heap.ArenaAllocator,
    groups: std.ArrayList([:0]const u8) = .empty,
    emoji: std.ArrayList(Emoji) = .empty,
    tone_variants: std.ArrayList([:0]const u8) = .empty,
    /// Emoji text with FE0F removed, mapped to its index; toned variants map to their base.
    by_key: std.StringHashMapUnmanaged(u32) = .empty,
    names: std.StringHashMapUnmanaged(Annotation) = .empty,

    pub fn init(gpa: std.mem.Allocator) !Db {
        var db: Db = .{ .arena = .init(gpa) };
        errdefer db.arena.deinit();
        const a = db.arena.allocator();
        try parseAnnotations(&db.names, a, annotations);
        try parseAnnotations(&db.names, a, annotations_derived);
        try db.parseEmoji(a);
        return db;
    }

    pub fn deinit(db: *Db) void {
        db.arena.deinit();
    }

    fn parseEmoji(db: *Db, a: std.mem.Allocator) !void {
        var pending: std.ArrayList(Variant) = .empty;
        var group: ?u8 = null;
        var lines = std.mem.splitScalar(u8, emoji_test, '\n');
        while (lines.next()) |line| {
            if (std.mem.startsWith(u8, line, "# group: ")) {
                const title = line["# group: ".len..];
                if (std.mem.eql(u8, title, "Component")) {
                    group = null;
                } else {
                    group = @intCast(db.groups.items.len);
                    try db.groups.append(a, try a.dupeSentinel(u8, title, 0));
                }
                continue;
            }
            const g = group orelse continue;
            if (line.len == 0 or line[0] == '#') continue;
            const semi = std.mem.findScalar(u8, line, ';') orelse continue;
            const hash = std.mem.findScalar(u8, line, '#') orelse continue;
            const status = std.mem.trim(u8, line[semi + 1 .. hash], " ");
            if (!std.mem.eql(u8, status, "fully-qualified")) continue;

            var text: std.ArrayList(u8) = .empty;
            var tone: u8 = 0;
            var mixed = false;
            var cps = std.mem.tokenizeScalar(u8, line[0..semi], ' ');
            while (cps.next()) |hex| {
                const cp = try std.fmt.parseInt(u21, hex, 16);
                if (toneOf(cp)) |t| {
                    if (tone != 0 and tone != t) mixed = true;
                    tone = t;
                }
                var buf: [4]u8 = undefined;
                const n = try std.unicode.utf8Encode(cp, &buf);
                try text.appendSlice(a, buf[0..n]);
            }
            const utf8 = try text.toOwnedSlice(a);
            const z = try a.dupeSentinel(u8, utf8, 0);

            if (tone != 0) {
                if (!mixed) try pending.append(a, .{ .text = z, .tone = tone });
                continue;
            }
            const key = try stripped(a, z, false);
            const ann = db.names.get(key) orelse Annotation{};
            const name = if (ann.name.len > 0) ann.name else commentName(line[hash..]);
            try db.by_key.put(a, key, @intCast(db.emoji.items.len));
            try db.emoji.append(a, .{
                .text = z,
                .name = try a.dupeSentinel(u8, name, 0),
                .keywords = ann.keywords,
                .group = g,
            });
        }
        try db.attachTones(a, pending.items);
    }

    const Variant = struct { text: [:0]const u8, tone: u8 };

    fn attachTones(db: *Db, a: std.mem.Allocator, variants: []const Variant) !void {
        for (variants) |v| {
            const key = try stripped(a, v.text, true);
            const base = db.by_key.get(key) orelse continue;
            const e = &db.emoji.items[base];
            if (e.tones == no_tones) {
                e.tones = @intCast(db.tone_variants.items.len);
                try db.tone_variants.appendNTimes(a, e.text, 5);
            }
            db.tone_variants.items[e.tones + v.tone - 1] = v.text;
            try db.by_key.put(a, try stripped(a, v.text, false), base);
        }
    }

    pub fn toned(db: *const Db, index: u32, tone: u32) [:0]const u8 {
        const e = db.emoji.items[index];
        if (tone == 0 or tone > 5 or e.tones == no_tones) return e.text;
        return db.tone_variants.items[e.tones + tone - 1];
    }

    pub fn find(db: *const Db, text: []const u8) ?u32 {
        var buf: [1024]u8 = undefined;
        var fba = std.heap.FixedBufferAllocator.init(&buf);
        const key = stripped(fba.allocator(), text, false) catch return null;
        return db.by_key.get(key);
    }

    pub fn search(db: *const Db, gpa: std.mem.Allocator, query: []const u8) ![]u32 {
        var lowered: [256]u8 = undefined;
        const q = lowerAscii(&lowered, query);
        var hits: std.ArrayList(Hit) = .empty;
        defer hits.deinit(gpa);
        for (db.emoji.items, 0..) |e, i| {
            if (score(e.name, e.keywords, q)) |s| try hits.append(gpa, .{ .index = @intCast(i), .score = s });
        }
        std.mem.sort(Hit, hits.items, {}, Hit.lessThan);
        const out = try gpa.alloc(u32, hits.items.len);
        for (hits.items, out) |h, *o| o.* = h.index;
        return out;
    }
};

const Hit = struct {
    index: u32,
    score: u32,
    fn lessThan(_: void, l: Hit, r: Hit) bool {
        return if (l.score != r.score) l.score < r.score else l.index < r.index;
    }
};

fn toneOf(cp: u21) ?u8 {
    return if (cp >= 0x1F3FB and cp <= 0x1F3FF) @intCast(cp - 0x1F3FB + 1) else null;
}

/// Drops FE0F, and the skin tone modifiers when `drop_tones`, so lookups ignore presentation.
fn stripped(a: std.mem.Allocator, text: []const u8, drop_tones: bool) ![]const u8 {
    var out: std.ArrayList(u8) = .empty;
    var it = (try std.unicode.Utf8View.init(text)).iterator();
    while (it.nextCodepointSlice()) |slice| {
        const cp = try std.unicode.utf8Decode(slice);
        if (cp == 0xFE0F) continue;
        if (drop_tones and toneOf(cp) != null) continue;
        try out.appendSlice(a, slice);
    }
    return out.toOwnedSlice(a);
}

/// The name after "E<version> " in an emoji-test.txt comment.
fn commentName(comment: []const u8) []const u8 {
    var words = std.mem.tokenizeScalar(u8, comment, ' ');
    while (words.next()) |w| {
        if (w.len > 1 and w[0] == 'E' and std.ascii.isDigit(w[1])) return std.mem.trim(u8, words.rest(), " ");
    }
    return "";
}

fn parseAnnotations(map: *std.StringHashMapUnmanaged(Annotation), a: std.mem.Allocator, xml: []const u8) !void {
    const open = "<annotation cp=\"";
    var lines = std.mem.splitScalar(u8, xml, '\n');
    while (lines.next()) |raw| {
        const line = std.mem.trim(u8, raw, " \t\r");
        if (!std.mem.startsWith(u8, line, open)) continue;
        const rest = line[open.len..];
        const quote = std.mem.findScalar(u8, rest, '"') orelse continue;
        const gt = std.mem.findScalar(u8, rest, '>') orelse continue;
        const end = std.mem.find(u8, rest, "</annotation>") orelse continue;
        const key = try stripped(a, try unescape(a, rest[0..quote]), false);
        const value = try unescape(a, rest[gt + 1 .. end]);
        const tts = std.mem.find(u8, rest[quote..gt], "type=\"tts\"") != null;
        const gop = try map.getOrPut(a, key);
        if (!gop.found_existing) gop.value_ptr.* = .{};
        if (tts) gop.value_ptr.name = value else gop.value_ptr.keywords = value;
    }
}

fn unescape(a: std.mem.Allocator, s: []const u8) ![]const u8 {
    if (std.mem.findScalar(u8, s, '&') == null) return s;
    const entities = [_][2][]const u8{ .{ "&amp;", "&" }, .{ "&lt;", "<" }, .{ "&gt;", ">" }, .{ "&quot;", "\"" }, .{ "&apos;", "'" } };
    var out: std.ArrayList(u8) = .empty;
    var i: usize = 0;
    outer: while (i < s.len) {
        for (entities) |e| {
            if (std.mem.startsWith(u8, s[i..], e[0])) {
                try out.appendSlice(a, e[1]);
                i += e[0].len;
                continue :outer;
            }
        }
        try out.append(a, s[i]);
        i += 1;
    }
    return out.toOwnedSlice(a);
}

fn lowerAscii(buf: []u8, s: []const u8) []const u8 {
    const n = @min(buf.len, s.len);
    return std.ascii.lowerString(buf[0..n], s[0..n]);
}

/// Lower is better; null when some query word matches nothing. Every word must match.
pub fn score(name: []const u8, keywords: []const u8, query: []const u8) ?u32 {
    var total: u32 = 0;
    var any = false;
    var words = std.mem.tokenizeScalar(u8, query, ' ');
    while (words.next()) |w| {
        any = true;
        total += wordScore(name, keywords, w) orelse return null;
    }
    return if (any) total else null;
}

fn wordScore(name: []const u8, keywords: []const u8, w: []const u8) ?u32 {
    if (std.ascii.startsWithIgnoreCase(name, w)) return if (name.len == w.len) 0 else 1;
    if (wordPrefix(name, w)) return 2;
    var kws = std.mem.splitSequence(u8, keywords, " | ");
    var best: ?u32 = null;
    while (kws.next()) |k| {
        if (std.ascii.eqlIgnoreCase(k, w)) return 3;
        if (std.ascii.startsWithIgnoreCase(k, w) or wordPrefix(k, w)) best = 4;
    }
    if (best) |b| return b;
    if (std.ascii.findIgnoreCase(name, w) != null) return 5;
    return null;
}

fn wordPrefix(text: []const u8, w: []const u8) bool {
    var i: usize = 0;
    while (std.ascii.findIgnoreCasePos(text, i, w)) |at| : (i = at + 1) {
        if (at == 0 or !std.ascii.isAlphanumeric(text[at - 1])) return true;
    }
    return false;
}

var global: ?Db = null;
var last_search: []u32 = &.{};
var symbol_hits: std.ArrayList(u32) = .empty;
var name_buf: [256]u8 = undefined;

fn the() *Db {
    return &global.?;
}

export fn gp_init() bool {
    if (global != null) return true;
    global = Db.init(std.heap.c_allocator) catch return false;
    return true;
}

export fn gp_emoji_group_count() u32 {
    return @intCast(the().groups.items.len);
}

export fn gp_emoji_group_name(g: u32) [*:0]const u8 {
    return the().groups.items[g].ptr;
}

export fn gp_emoji_count() u32 {
    return @intCast(the().emoji.items.len);
}

export fn gp_emoji_text(i: u32, tone: u32) [*:0]const u8 {
    return the().toned(i, tone).ptr;
}

export fn gp_emoji_name(i: u32) [*:0]const u8 {
    return the().emoji.items[i].name.ptr;
}

export fn gp_emoji_group(i: u32) u32 {
    return the().emoji.items[i].group;
}

export fn gp_emoji_has_tones(i: u32) bool {
    return the().emoji.items[i].tones != no_tones;
}

export fn gp_emoji_find(text: [*:0]const u8) i32 {
    const i = the().find(std.mem.span(text)) orelse return -1;
    return @intCast(i);
}

/// Results stay valid until the next search.
export fn gp_emoji_search(query: [*:0]const u8, out: *[*]const u32) u32 {
    std.heap.c_allocator.free(last_search);
    last_search = the().search(std.heap.c_allocator, std.mem.span(query)) catch &.{};
    out.* = last_search.ptr;
    return @intCast(last_search.len);
}

export fn gp_symbol_group_count() u32 {
    return symbols.groups.len;
}

export fn gp_symbol_group_name(g: u32) [*:0]const u8 {
    return symbols.groups[g].name;
}

export fn gp_symbol_group_codepoints(g: u32, out: *[*]const u32) u32 {
    const cps = symbols.groups[g].codepoints;
    out.* = cps.ptr;
    return @intCast(cps.len);
}

/// The CLDR name of one codepoint, or an empty string. Valid until the next call.
export fn gp_symbol_name(cp: u32) [*:0]const u8 {
    var utf8: [4]u8 = undefined;
    const n = std.unicode.utf8Encode(@intCast(cp), &utf8) catch return "";
    const ann = the().names.get(utf8[0..n]) orelse return "";
    const len = @min(ann.name.len, name_buf.len - 1);
    @memcpy(name_buf[0..len], ann.name[0..len]);
    name_buf[len] = 0;
    return @ptrCast(&name_buf);
}

/// Codepoints from every symbol group whose CLDR name or keywords match. Valid until the next call.
export fn gp_symbol_search(query: [*:0]const u8, out: *[*]const u32) u32 {
    const a = std.heap.c_allocator;
    symbol_hits.clearRetainingCapacity();
    var lowered: [256]u8 = undefined;
    const q = lowerAscii(&lowered, std.mem.span(query));
    for (symbols.groups) |g| {
        for (g.codepoints) |cp| {
            var utf8: [4]u8 = undefined;
            const n = std.unicode.utf8Encode(@intCast(cp), &utf8) catch continue;
            const ann = the().names.get(utf8[0..n]) orelse continue;
            if (score(ann.name, ann.keywords, q) == null) continue;
            if (std.mem.findScalar(u32, symbol_hits.items, cp) != null) continue;
            symbol_hits.append(a, cp) catch break;
        }
    }
    out.* = symbol_hits.items.ptr;
    return @intCast(symbol_hits.items.len);
}

export fn gp_kaomoji_group_count() u32 {
    return kaomoji.groups.len;
}

export fn gp_kaomoji_group_name(g: u32) [*:0]const u8 {
    return kaomoji.groups[g].name;
}

export fn gp_kaomoji_group_size(g: u32) u32 {
    return @intCast(kaomoji.groups[g].items.len);
}

export fn gp_kaomoji_text(g: u32, i: u32) [*:0]const u8 {
    return kaomoji.groups[g].items[i];
}

test "groups skip Component and keep Unicode order" {
    var d = try Db.init(std.testing.allocator);
    defer d.deinit();
    try std.testing.expectEqualStrings("Smileys & Emotion", d.groups.items[0]);
    try std.testing.expectEqualStrings("Flags", d.groups.items[d.groups.items.len - 1]);
    for (d.groups.items) |g| try std.testing.expect(!std.mem.eql(u8, g, "Component"));
}

test "names come from CLDR and tones attach to their base" {
    var d = try Db.init(std.testing.allocator);
    defer d.deinit();
    const thumbs = d.find("👍").?;
    try std.testing.expectEqualStrings("thumbs up", d.emoji.items[thumbs].name);
    try std.testing.expectEqualStrings("👍🏽", d.toned(thumbs, 3));
    try std.testing.expectEqual(thumbs, d.find("👍🏿").?);
    const grin = d.find("😀").?;
    try std.testing.expectEqualStrings("😀", d.toned(grin, 3));
}

test "FE0F does not matter for lookups" {
    var d = try Db.init(std.testing.allocator);
    defer d.deinit();
    const heart = d.find("❤").?;
    try std.testing.expectEqual(heart, d.find("❤️").?);
    try std.testing.expectEqualStrings("❤️", d.emoji.items[heart].text);
}

test "multi-person sequences take a uniform tone" {
    var d = try Db.init(std.testing.allocator);
    defer d.deinit();
    const shake = d.find("🤝").?;
    try std.testing.expectEqualStrings("🤝🏻", d.toned(shake, 1));
    const couple = d.find("🧑‍🤝‍🧑").?;
    try std.testing.expectEqualStrings("🧑🏾‍🤝‍🧑🏾", d.toned(couple, 4));
}

test "search ranks names before keywords" {
    var d = try Db.init(std.testing.allocator);
    defer d.deinit();
    const hits = try d.search(std.testing.allocator, "pickle");
    defer std.testing.allocator.free(hits);
    try std.testing.expectEqualStrings("🫝", d.emoji.items[hits[0]].text);

    const happy = try d.search(std.testing.allocator, "HAPPY");
    defer std.testing.allocator.free(happy);
    try std.testing.expect(happy.len > 3);

    const both = try d.search(std.testing.allocator, "cat face");
    defer std.testing.allocator.free(both);
    try std.testing.expectEqualStrings("🐱", d.emoji.items[both[0]].text);

    const none = try d.search(std.testing.allocator, "zzqqxx");
    defer std.testing.allocator.free(none);
    try std.testing.expectEqual(0, none.len);
}

test "symbols and kaomoji are well formed" {
    for (symbols.groups) |g| try std.testing.expect(g.codepoints.len > 0);
    for (kaomoji.groups) |g| {
        try std.testing.expect(g.items.len > 0);
        for (g.items) |k| _ = try std.unicode.Utf8View.init(std.mem.span(k));
    }
}
