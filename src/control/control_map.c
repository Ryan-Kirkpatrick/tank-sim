#include "control_map.h"

#include <stdio.h>
#include "util/helpers.h"
#include "util/tank_assert.h"

const char* input_gear_to_str(input_gear_t gear) {
    switch (gear) {
        case INPUT_FORWARDS:
            return "INPUT_FORWARDS";
        case INPUT_REVERSE:
            return "INPUT_REVERSE";
    }
    TANK_ASSERT_M(false, "Unexpected input_gear_t");
    return "";
}

void input_report_print(const input_report_t* report) {
    printf("{\r\n");
    printf("  \"left_tiller\": %f,\r\n", report->left_tiller);
    printf("  \"right_tiller\": %f,\r\n", report->right_tiller);
    printf("  \"accelerator\": %f,\r\n", report->accelerator);
    printf("  \"gear\": %s\r\n", input_gear_to_str(report->gear));
    printf("}\r\n");
}

typedef struct tiller_output {
    float side_pwm;
    float handbrake_pwm;
} tiller_output_t;

tiller_output_t map_tiller_input_to_output(const control_settings_t* config, const float tiller_input) {
    tiller_output_t output = {.side_pwm = 0.0, .handbrake_pwm = 0.0};

    // Apply deadzone
    if (tiller_input < config->tiller_deadzone) {
        return output;
    }

    // Apply turning
    // Turning is on the range from the deadzone to the max turn threshold
    output.side_pwm =
        (tiller_input - config->tiller_deadzone) / (config->tiller_max_turn_threshold - config->tiller_deadzone);
    output.side_pwm = MIN_OF(output.side_pwm, 1.0);

    // Apply hand brake
    if (tiller_input > config->tiller_handbrake_threshold_begin) {
        output.handbrake_pwm = (tiller_input - config->tiller_handbrake_threshold_begin) /
                               (config->tiller_handbrake_threshold_end - config->tiller_handbrake_threshold_begin);
        output.handbrake_pwm = MIN_OF(output.handbrake_pwm, 1.0);
    }

    return output;
}

typedef struct pedal_output {
    float pwm;
} pedal_output_t;

pedal_output_t map_pedal_input_to_output(const control_settings_t* config, const float pedal_input) {
    pedal_output_t output = {.pwm = 0.0};

    // Apply dead zone
    if (pedal_input < config->pedal_deadzone) {
        return output;
    }

    // Apply pedal
    output.pwm = (pedal_input - config->pedal_deadzone) / (1.0 - config->pedal_deadzone);
    output.pwm = MIN_OF(output.pwm, 1.0);

    return output;
}

keyboard_output_t map_input_to_output(const control_settings_t* config, const input_report_t* input) {
    const pedal_output_t accelerator = map_pedal_input_to_output(config, input->accelerator);
    const tiller_output_t left_tiller = map_tiller_input_to_output(config, input->left_tiller);
    const tiller_output_t right_tiller = map_tiller_input_to_output(config, input->right_tiller);

    // TODO reverse
    keyboard_output_t output = {.forward_duty_cycle = accelerator.pwm,
                                .left_duty_cycle = left_tiller.side_pwm,
                                .right_duty_cycle = right_tiller.side_pwm,
                                .hand_brake_duty_cycle = MAX_OF(left_tiller.handbrake_pwm, right_tiller.handbrake_pwm)};

    return output;
}
