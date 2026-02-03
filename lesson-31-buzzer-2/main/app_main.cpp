#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "common.h"
#include "sdkconfig.h"

#include "MelodyPlayer.h"

// The melody is derived from this source: https://onlinesequencer.net/3243312
constexpr const MelodyPlayer::note_t melody[] =
{
     988,  // B5  [1/8]
       0,  //     [1/8]

    1480,  // F6# [1/8]
     988,  // B5  [1/8]
    1568,  // G6  [1/8]
       0,  //     [1/8]

    1480,  // F6# [1/8]
    1319,  // E6  [1/8]
    1480,  // F6# [1/8]
       0,  //     [1/8]

    1319,  // E6  [1/8]
    1480,  // F6# [1/8]
    1568,  // G6  [1/8]
    1568,  // G6  [1/8]
    1480,  // F6# [1/8]
    1319,  // E6  [1/8]
     988,  // B5  [1/8]
       0,  //     [1/8]

    1480,  // F6# [1/8]
     988,  // B5  [1/8]
    1568,  // G6  [1/8]
       0,  //     [1/8]

    1480,  // F6# [1/8]
    1319,  // E6  [1/8]
    1175,  // D6  [1/8]
       0,  //     [1/8]

    1319,  // E6  [1/8]
    1175,  // D6  [1/8]
    1109,  // C6# [1/8]
    1109,  // C6# [1/8]
    1175,  // D6  [1/8]
    1109,  // C6# [1/8]
     988,  // B5  [1/8]
       0,  //     [1/8]
};

extern "C" void app_main(void)
{
    MelodyPlayer player(static_cast<gpio_num_t>(CONFIG_PIN_BUZZER));

    ESP_LOGI(TAG, "Now playing: Pain - Shut Your Mouth");
    player.Play(melody, melody + countof(melody), 250);

    while (player.IsPlaying())
        vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Thanks for listening!");
}

