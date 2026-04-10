#include "heading_indicator.h"

#include "aircraft_math.h"

#include <stddef.h>

void HeadingIndicator_Init(HeadingData *heading,
                           float magnetic_declination_deg)
{
    if (heading == NULL) {
        return;
    }

    heading->magnetic_heading_deg = 0.0f;
    heading->true_heading_deg = AircraftMath_Normalize360(magnetic_declination_deg);
    heading->display_heading_deg = AircraftMath_HeadingToCard(heading->true_heading_deg);
    heading->magnetic_valid = 0u;
}

void HeadingIndicator_Update(HeadingData *heading,
                             float yaw_deg,
                             uint8_t magnetic_valid,
                             float magnetic_declination_deg,
                             float dt_s,
                             float smoothing_tau_s)
{
    const float magnetic_heading = AircraftMath_Normalize360(yaw_deg);
    const float true_heading = AircraftMath_Normalize360(magnetic_heading + magnetic_declination_deg);

    if (heading == NULL) {
        return;
    }

    if (magnetic_valid != 0u) {
        if (heading->magnetic_valid == 0u) {
            heading->magnetic_heading_deg = magnetic_heading;
            heading->true_heading_deg = true_heading;
        } else {
            heading->magnetic_heading_deg =
                AircraftMath_LowPassAngle360(heading->magnetic_heading_deg,
                                             magnetic_heading,
                                             dt_s,
                                             smoothing_tau_s);
            heading->true_heading_deg =
                AircraftMath_LowPassAngle360(heading->true_heading_deg,
                                             true_heading,
                                             dt_s,
                                             smoothing_tau_s);
        }
    }

    heading->magnetic_valid = magnetic_valid;
    heading->display_heading_deg = AircraftMath_HeadingToCard(heading->true_heading_deg);
}
