#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "common.h"
#include "sdkconfig.h"

#include "MelodyPlayer.h"

// Whole note duration
const int wh = 2000;

// The melody is derived from this source: https://onlinesequencer.net/3243312
constexpr const MelodyPlayer::note_t melody[] =
{
    {  988, wh / 8 },  // B5  [1/8]
    {    0, wh / 8 },  //     [1/8]

    { 1480, wh / 8 },  // F6# [1/8]
    {  988, wh / 8 },  // B5  [1/8]
    { 1568, wh / 8 },  // G6  [1/8]
    {    0, wh / 8 },  //     [1/8]

    { 1480, wh / 8 },  // F6# [1/8]
    { 1319, wh / 8 },  // E6  [1/8]
    { 1480, wh / 8 },  // F6# [1/8]
    {    0, wh / 8 },  //     [1/8]

    { 1319, wh / 8 },  // E6  [1/8]
    { 1480, wh / 8 },  // F6# [1/8]
    { 1568, wh / 8 },  // G6  [1/8]
    { 1568, wh / 8 },  // G6  [1/8]
    { 1480, wh / 8 },  // F6# [1/8]
    { 1319, wh / 8 },  // E6  [1/8]
    {  988, wh / 8 },  // B5  [1/8]
    {    0, wh / 8 },  //     [1/8]

    { 1480, wh / 8 },  // F6# [1/8]
    {  988, wh / 8 },  // B5  [1/8]
    { 1568, wh / 8 },  // G6  [1/8]
    {    0, wh / 8 },  //     [1/8]

    { 1480, wh / 8 },  // F6# [1/8]
    { 1319, wh / 8 },  // E6  [1/8]
    { 1175, wh / 8 },  // D6  [1/8]
    {    0, wh / 8 },  //     [1/8]

    { 1319, wh / 8 },  // E6  [1/8]
    { 1175, wh / 8 },  // D6  [1/8]
    { 1109, wh / 8 },  // C6# [1/8]
    { 1109, wh / 8 },  // C6# [1/8]
    { 1175, wh / 8 },  // D6  [1/8]
    { 1109, wh / 8 },  // C6# [1/8]
    {  988, wh / 8 },  // B5  [1/8]
    {    0, wh / 8 },  //     [1/8]
};

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Now playing: Pain - Shut Your Mouth");
    MelodyPlayer player(static_cast<gpio_num_t>(CONFIG_PIN_BUZZER));

    player.Play(melody, melody + countof(melody));

    for (int i = 0; i < countof(melody); i = player.Progress()) {
        ESP_LOGI(TAG, "Playing note #%d", i);
        vTaskDelay(pdMS_TO_TICKS(wh / 8));
    }

    ESP_LOGI(TAG, "Thanks for listening!");
}

