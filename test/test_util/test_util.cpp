/**
 * Host tests for the parsers that sit behind the Wi-Fi setup portal.
 *
 * These run on the build machine (`pio test -e native`), so every portal input
 * can be exercised — including the malformed and hostile ones — without a
 * board, a network, or a flash cycle.
 */
#include <unity.h>

#include "util/coords.h"
#include "util/portal_input.h"

void setUp() {}
void tearDown() {}

// --- checkbox ---------------------------------------------------------------

void test_checkbox_accepts_ticked_spellings() {
  TEST_ASSERT_TRUE(util::portal::checkboxChecked("T"));
  TEST_ASSERT_TRUE(util::portal::checkboxChecked("t"));
  TEST_ASSERT_TRUE(util::portal::checkboxChecked("1"));
  TEST_ASSERT_TRUE(util::portal::checkboxChecked("on"));
  TEST_ASSERT_TRUE(util::portal::checkboxChecked("true"));
}

void test_checkbox_unticked_is_false() {
  TEST_ASSERT_FALSE(util::portal::checkboxChecked(""));
  TEST_ASSERT_FALSE(util::portal::checkboxChecked(nullptr));
}

void test_checkbox_rejects_unexpected_values() {
  // An earlier revision treated "F" as ticked, which silently inverted a
  // setting; anything that is not an affirmative spelling must read as off.
  TEST_ASSERT_FALSE(util::portal::checkboxChecked("F"));
  TEST_ASSERT_FALSE(util::portal::checkboxChecked("f"));
  TEST_ASSERT_FALSE(util::portal::checkboxChecked("0"));
  TEST_ASSERT_FALSE(util::portal::checkboxChecked("off"));
  TEST_ASSERT_FALSE(util::portal::checkboxChecked("TRUE"));
  TEST_ASSERT_FALSE(util::portal::checkboxChecked("<script>"));
}

// --- step number ------------------------------------------------------------

void test_step_index_maps_one_based_to_zero_based() {
  uint8_t step = 99;
  TEST_ASSERT_TRUE(util::portal::stepIndex("1", 3, &step));
  TEST_ASSERT_EQUAL_UINT8(0, step);
  TEST_ASSERT_TRUE(util::portal::stepIndex("3", 3, &step));
  TEST_ASSERT_EQUAL_UINT8(2, step);
}

void test_step_index_rejects_out_of_range() {
  uint8_t step = 7;
  TEST_ASSERT_FALSE(util::portal::stepIndex("0", 3, &step));
  TEST_ASSERT_FALSE(util::portal::stepIndex("4", 3, &step));
  TEST_ASSERT_FALSE(util::portal::stepIndex("999", 3, &step));
  TEST_ASSERT_EQUAL_UINT8(7, step);  // caller's value survives a rejection
}

void test_step_index_rejects_malformed() {
  uint8_t step = 7;
  TEST_ASSERT_FALSE(util::portal::stepIndex("", 3, &step));
  TEST_ASSERT_FALSE(util::portal::stepIndex(nullptr, 3, &step));
  TEST_ASSERT_FALSE(util::portal::stepIndex("2x", 3, &step));
  TEST_ASSERT_FALSE(util::portal::stepIndex(" 2", 3, &step));
  TEST_ASSERT_FALSE(util::portal::stepIndex("-1", 3, &step));
  TEST_ASSERT_FALSE(util::portal::stepIndex("+2", 3, &step));
  TEST_ASSERT_FALSE(util::portal::stepIndex("2.0", 3, &step));
  TEST_ASSERT_FALSE(util::portal::stepIndex("99999999999999", 3, &step));
  TEST_ASSERT_EQUAL_UINT8(7, step);
}

// --- coordinates ------------------------------------------------------------

void test_coordinate_parses_plain_decimals() {
  double value = 0.0;
  TEST_ASSERT_TRUE(util::coords::parseCoordinate("52.3676", &value));
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 52.3676, value);
  TEST_ASSERT_TRUE(util::coords::parseCoordinate("-4.9041", &value));
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, -4.9041, value);
  TEST_ASSERT_TRUE(util::coords::parseCoordinate("0", &value));
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, value);
}

void test_coordinate_rejects_trailing_junk() {
  double value = 1.5;
  TEST_ASSERT_FALSE(util::coords::parseCoordinate("52.3676N", &value));
  TEST_ASSERT_FALSE(util::coords::parseCoordinate("52,3676", &value));
  TEST_ASSERT_FALSE(util::coords::parseCoordinate("", &value));
  TEST_ASSERT_FALSE(util::coords::parseCoordinate(nullptr, &value));
  TEST_ASSERT_FALSE(util::coords::parseCoordinate("abc", &value));
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.5, value);
}

void test_coordinate_rejects_non_finite() {
  // strtod happily accepts these spellings; the radar's projection would then
  // produce NaN pixel coordinates for every aircraft.
  double value = 1.5;
  TEST_ASSERT_FALSE(util::coords::parseCoordinate("nan", &value));
  TEST_ASSERT_FALSE(util::coords::parseCoordinate("inf", &value));
  TEST_ASSERT_FALSE(util::coords::parseCoordinate("-inf", &value));
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.5, value);
}

void test_lat_lon_range() {
  TEST_ASSERT_TRUE(util::coords::validLatLon(52.3676, 4.9041));
  TEST_ASSERT_TRUE(util::coords::validLatLon(-90.0, -180.0));
  TEST_ASSERT_TRUE(util::coords::validLatLon(90.0, 180.0));
  TEST_ASSERT_FALSE(util::coords::validLatLon(90.1, 0.0));
  TEST_ASSERT_FALSE(util::coords::validLatLon(-90.1, 0.0));
  TEST_ASSERT_FALSE(util::coords::validLatLon(0.0, 180.1));
  TEST_ASSERT_FALSE(util::coords::validLatLon(0.0, -180.1));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_checkbox_accepts_ticked_spellings);
  RUN_TEST(test_checkbox_unticked_is_false);
  RUN_TEST(test_checkbox_rejects_unexpected_values);
  RUN_TEST(test_step_index_maps_one_based_to_zero_based);
  RUN_TEST(test_step_index_rejects_out_of_range);
  RUN_TEST(test_step_index_rejects_malformed);
  RUN_TEST(test_coordinate_parses_plain_decimals);
  RUN_TEST(test_coordinate_rejects_trailing_junk);
  RUN_TEST(test_coordinate_rejects_non_finite);
  RUN_TEST(test_lat_lon_range);
  return UNITY_END();
}
