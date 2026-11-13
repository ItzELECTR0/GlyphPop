#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool gp_init(void);

uint32_t gp_emoji_group_count(void);
const char *gp_emoji_group_name(uint32_t group);
uint32_t gp_emoji_count(void);
// tone 0 is the default yellow, 1 to 5 are light to dark.
const char *gp_emoji_text(uint32_t index, uint32_t tone);
const char *gp_emoji_name(uint32_t index);
uint32_t gp_emoji_group(uint32_t index);
bool gp_emoji_has_tones(uint32_t index);
int32_t gp_emoji_find(const char *text);
uint32_t gp_emoji_search(const char *query, const uint32_t **out);

uint32_t gp_symbol_group_count(void);
const char *gp_symbol_group_name(uint32_t group);
uint32_t gp_symbol_group_codepoints(uint32_t group, const uint32_t **out);
const char *gp_symbol_name(uint32_t codepoint);
uint32_t gp_symbol_search(const char *query, const uint32_t **out);

uint32_t gp_kaomoji_group_count(void);
const char *gp_kaomoji_group_name(uint32_t group);
uint32_t gp_kaomoji_group_size(uint32_t group);
const char *gp_kaomoji_text(uint32_t group, uint32_t index);

#ifdef __cplusplus
}
#endif
