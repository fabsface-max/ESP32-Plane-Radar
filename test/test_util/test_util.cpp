/**
 * Host tests for the parsers that sit behind the Wi-Fi setup portal.
 *
 * These run on the build machine (`pio test -e native`), so every portal input
 * can be exercised — including the malformed and hostile ones — without a
 * board, a network, or a flash cycle.
 */
#include <unity.h>

#include "util/aircraft_class.h"
#include "util/callsign.h"
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

// --- fixed-choice number field ----------------------------------------------

void test_one_of_accepts_listed_values() {
  const uint8_t allowed[] = {0, 3, 5, 7};
  uint8_t value = 99;
  TEST_ASSERT_TRUE(util::portal::oneOf("0", allowed, 4, &value));
  TEST_ASSERT_EQUAL_UINT8(0, value);
  TEST_ASSERT_TRUE(util::portal::oneOf("7", allowed, 4, &value));
  TEST_ASSERT_EQUAL_UINT8(7, value);
}

void test_one_of_rejects_everything_else() {
  const uint8_t allowed[] = {0, 3, 5, 7};
  uint8_t value = 5;
  TEST_ASSERT_FALSE(util::portal::oneOf("4", allowed, 4, &value));
  TEST_ASSERT_FALSE(util::portal::oneOf("70", allowed, 4, &value));
  TEST_ASSERT_FALSE(util::portal::oneOf("", allowed, 4, &value));
  TEST_ASSERT_FALSE(util::portal::oneOf(nullptr, allowed, 4, &value));
  TEST_ASSERT_FALSE(util::portal::oneOf("3s", allowed, 4, &value));
  TEST_ASSERT_FALSE(util::portal::oneOf("-3", allowed, 4, &value));
  TEST_ASSERT_EQUAL_UINT8(5, value);
}

// --- callsign ---------------------------------------------------------------

void test_callsign_extracts_airline_prefix() {
  char code[4] = {};
  TEST_ASSERT_TRUE(util::callsign::icaoPrefix("DLH4AB", code));
  TEST_ASSERT_EQUAL_STRING("DLH", code);
  TEST_ASSERT_TRUE(util::callsign::icaoPrefix("baw12", code));
  TEST_ASSERT_EQUAL_STRING("BAW", code);
}

void test_callsign_rejects_non_airline_shapes() {
  char code[4] = {};
  // Registrations and hex ids must keep showing verbatim rather than being
  // attributed to whichever airline shares their first three characters.
  TEST_ASSERT_FALSE(util::callsign::icaoPrefix("D-EABC", code));
  TEST_ASSERT_FALSE(util::callsign::icaoPrefix("DLH", code));
  TEST_ASSERT_FALSE(util::callsign::icaoPrefix("ABCDEF", code));
  TEST_ASSERT_FALSE(util::callsign::icaoPrefix("4X2ABC", code));
  TEST_ASSERT_FALSE(util::callsign::icaoPrefix("", code));
  TEST_ASSERT_FALSE(util::callsign::icaoPrefix(nullptr, code));
}

// --- aircraft class ---------------------------------------------------------

void test_class_prefers_emitter_category() {
  using util::aircraft::Klass;
  TEST_ASSERT_EQUAL(Klass::kRotor, util::aircraft::classify("B738", "A7"));
  TEST_ASSERT_EQUAL(Klass::kHeavy, util::aircraft::classify("", "A5"));
  TEST_ASSERT_EQUAL(Klass::kLight, util::aircraft::classify("", "A1"));
}

void test_class_falls_back_to_type_code() {
  using util::aircraft::Klass;
  TEST_ASSERT_EQUAL(Klass::kRotor, util::aircraft::classify("EC35", ""));
  TEST_ASSERT_EQUAL(Klass::kRotor, util::aircraft::classify("R44", ""));
  TEST_ASSERT_EQUAL(Klass::kHeavy, util::aircraft::classify("A388", ""));
  TEST_ASSERT_EQUAL(Klass::kHeavy, util::aircraft::classify("B744", ""));
  TEST_ASSERT_EQUAL(Klass::kJet, util::aircraft::classify("A320", ""));
  TEST_ASSERT_EQUAL(Klass::kJet, util::aircraft::classify("", ""));
  TEST_ASSERT_EQUAL(Klass::kJet, util::aircraft::classify(nullptr, nullptr));
}

void test_class_does_not_confuse_bombers_with_bells() {
  using util::aircraft::Klass;
  // Prefix matching would read the B-29 and the B-47 as Bell helicopters.
  TEST_ASSERT_EQUAL(Klass::kJet, util::aircraft::classify("B29", ""));
  TEST_ASSERT_EQUAL(Klass::kJet, util::aircraft::classify("B47", ""));
  TEST_ASSERT_EQUAL(Klass::kRotor, util::aircraft::classify("B47G", ""));
}

void test_alert_flags() {
  using namespace util::aircraft;
  TEST_ASSERT_EQUAL_UINT8(kFlagNotable, alertFlags("A388", "none", "", 0));
  TEST_ASSERT_EQUAL_UINT8(kFlagMilitary, alertFlags("A320", "none", "1000",
                                                    kDbFlagMilitary));
  TEST_ASSERT_EQUAL_UINT8(kFlagEmergency, alertFlags("A320", "", "7700", 0));
  TEST_ASSERT_EQUAL_UINT8(kFlagEmergency, alertFlags("A320", "general", "", 0));
  TEST_ASSERT_EQUAL_UINT8(0, alertFlags("A320", "none", "1000", 0));
  TEST_ASSERT_EQUAL_UINT8(0, alertFlags(nullptr, nullptr, nullptr, 0));
}

void test_emergency_squawks() {
  TEST_ASSERT_TRUE(util::aircraft::isEmergencySquawk("7500"));
  TEST_ASSERT_TRUE(util::aircraft::isEmergencySquawk("7600"));
  TEST_ASSERT_TRUE(util::aircraft::isEmergencySquawk("7700"));
  TEST_ASSERT_FALSE(util::aircraft::isEmergencySquawk("7000"));
  TEST_ASSERT_FALSE(util::aircraft::isEmergencySquawk("77000"));
  TEST_ASSERT_FALSE(util::aircraft::isEmergencySquawk(nullptr));
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
  RUN_TEST(test_one_of_accepts_listed_values);
  RUN_TEST(test_one_of_rejects_everything_else);
  RUN_TEST(test_callsign_extracts_airline_prefix);
  RUN_TEST(test_callsign_rejects_non_airline_shapes);
  RUN_TEST(test_class_prefers_emitter_category);
  RUN_TEST(test_class_falls_back_to_type_code);
  RUN_TEST(test_class_does_not_confuse_bombers_with_bells);
  RUN_TEST(test_alert_flags);
  RUN_TEST(test_emergency_squawks);
  return UNITY_END();
}
