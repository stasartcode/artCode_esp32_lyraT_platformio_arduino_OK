/* Control with a touch pad playing MP3 files from SD Card

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// FIXME BUG I2S_MODE_MASTER_TX_RX -> po aktualizacji PlatformIO zmienić w i2s.h:
// FIXME OK -> C++ / Arduino / Platformio -> a value of type "int" cannot be used to initialize an entity of type "i2s_mode_t" 
// -> change in .platformio\packages\framework-arduinoespressif32\tools\sdk\include\driver\driver\i2s.h - i2s_mode_t - I2S_MODE_MASTER_TX_RX = 13, 

// TODO FIXME - w nazwie pliku mp3 - 2 polskie znaki obok siebie jak óż zawieszają odtwarzanie -> utwór: Róże europy.mp3 -> prawdopodobnie chodzi o max długoc nazwy która chociaż automatycznie skrócona wizualnie to binarnie jest zbyt duża 
// FIXME nowe esp_http_client i tcp_transport z ADF/components nie kompilują się poprawnie tutaj

// Arduino - aby rozbudować łatwiej -> Konfiguracja plytki Lyrat w pins_arduino.h, po dodaniu :
// C:\Users\stas\.platformio\boards\esp32_lyrat_artCode.json
// C:\Users\stas\.platformio\packages\framework-arduinoespressif32\variants\esp32_lyrat_artCode\pins_arduino.h

// TODO - uproscic
// i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();

// ********************************************************************************
// artcode esp32Lyrat SD_MMC arduinio -> OK gdy SD 4-line Mode ustawione na plytce

#include <Arduino.h>
#include "adf_includes.h" // / --> artcode -> Espressif ADF IDF // -> IntelliSense BUG after Plathormio update - Rebuild IntelliSense Index - close files and restart IDE

// artcode test OK
#include <dirent.h>

// TEST  OK -> czyste C++ w kompilatorze Arduino - pominięcie Arduino Setup() i Loop()
// extern "C" {
//    void app_main();
// }

// ADF
static const char *TAG = "Lyrat_SD_MP3_TOUCH_OK";
#define CURRENT 0
#define NEXT 1
// artCode
static ushort file_index_max = 1024; // 0 to 65,535 files ; globalnie dla różnych funkcji? - OK

// TEST - OK - listowanie katalogu
int dir_list_test(void)
{
    DIR *sciezka;
    struct dirent *plik;
    ;
    sciezka = opendir("/");

    if (sciezka != NULL)
    {
        while ((plik = readdir(sciezka)) != NULL)
        {
            printf("Filename: %s\t\t Location in Directory Stream: %ld\n", plik->d_name, telldir(sciezka));
            // dla ksrty SD w formacie fat - zwraca 1,2,3,4 ... ( w innych formatach moze być co innego np. co 32 albo offsety plików)
            file_index_max++; // +1
        }
        printf("Plikow w katalogu  (file_index_max): %d \n\n", file_index_max);

        seekdir(sciezka, 2); // TEST seekdir
        printf("test Seekdir: = 2 (Location in Directory Stream = 2+1 = 3 ) \n");
        plik = readdir(sciezka); // FIXME // jak nie ma seekdir  Location in Directory Stream: 1 -  jak jest to 2
        printf("test Readdir po Seekdir: %s\t\t Location in Directory Stream: %ld\n\n", plik->d_name, telldir(sciezka));

        (void)closedir(sciezka);
    }
    else
        perror("Couldn't open the directory");

    return 0;
}

// FIXME odtwarza tylko pierwszy plik zamiast nastepnego
static FILE *get_file(int next_file)
{
    static FILE *file;
    // static int file_index = 0; // ponowne wywolanie funkcji nie zeruje file_index zgodnie z deklaracja przydziela ostatnia wartosc !
    static ushort file_index = 0;
    DIR *sciezka;
    struct dirent *plik;

    ///////////////////////////////////////////
    if (next_file != CURRENT)
    {
        // advance to the next file
        if (++file_index == file_index_max)
        {
            file_index = 0;
        }

        if (file != NULL)
        {
            fclose(file);
            file = NULL;
        }
    } //  if (next_file != CURRENT)

    // return a handle to the current file
    if (file == NULL)
    {
        sciezka = opendir("/");
        //  FIXME - sparawdzić file_index_MAX
        seekdir(sciezka, file_index); // FIXME  -> file_index musi być prawidłowy tj. plik o tym indeksie musi istnieć

        printf("Seekdir (file_index): %d (+1 funkcjonalnie -> Location in Directory Stream) ", file_index);

        printf("Plikow w katalogu  (file_index_max): %d \n\n", file_index_max);

        plik = readdir(sciezka); // FIXME // jak nie ma seekdir  Location in Directory Stream: 1 -  jak jest to 2
        printf("readdir po seekdir: %s\t\t Location in Directory Stream: %ld\n", plik->d_name, telldir(sciezka));

        //////////////////////////////////////
        file = fopen(plik->d_name, "r");
        // ESP_LOGI(TAG, "[ * ] Current file name -> %d", plik->d_name);
        printf(" Current file name -> %s)\n\n\t", plik->d_name);

        if (!file)
        {
            ESP_LOGE(TAG, "Error opening file name -> %s", plik->d_name);
            closedir(sciezka); // OK
            return NULL;
        }
    }
    return file;
}

// OK FIXME // -> typedef audio_element_err_t (*stream_func)(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context);
/*
 * Callback function to feed audio data stream from sdcard to mp3 decoder element
 */

 int my_sdcard_read_cb(audio_element_handle_t el, char *buf, int len, TickType_t wait_time, void *ctx)
 {
    int read_len = fread(buf, 1, len, get_file(CURRENT));
    if (read_len == 0)
    {
        read_len = AEL_IO_DONE;
    }
    return read_len;
};

// Arduino OK
void setup()
{

    // aktywacja COM z poziomu ADF tez dziala ->  ESP_LOGI - OK
    Serial.begin(115200); //arduino OK
}

void loop() // Arduino
// void app_main() // C++ 
{
    //artcode OK
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t i2s_stream_writer, mp3_decoder;
    //    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);

    ESP_LOGI(TAG, "[1.0] Initialize peripherals management");
        esp_periph_config_t periph_cfg = {0};
        esp_periph_init(&periph_cfg);

    // OK FIXME -> powoduje blad kompilacji
    ESP_LOGI(TAG, "[1.1] Start SD card peripheral ->> FiXME !!!!!");

        // OK - Niby bardziej poprawny zapis w C++
        periph_sdcard_cfg_t sdcard_cfg = {
            card_detect_pin : SD_CARD_INTR_GPIO, // GPIO_NUM_34
            root : ""                            // root "/"
        };
        esp_periph_handle_t sdcard_handle = periph_sdcard_init(&sdcard_cfg);

    ESP_LOGI(TAG, "[1.2] Start SD card peripheral");
        esp_periph_start(sdcard_handle);

        // Wait until sdcard was mounted
        while (!periph_sdcard_is_mounted(sdcard_handle))
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        // artcode
        dir_list_test(); // OK
                        // <-- artcode

    ESP_LOGI(TAG, "[1.3] Initialize Touch peripheral");
        periph_touch_cfg_t touch_cfg = {
            .touch_mask = TOUCH_SEL_SET | TOUCH_SEL_PLAY | TOUCH_SEL_VOLUP | TOUCH_SEL_VOLDWN,
            .tap_threshold_percent = 70,
        };
        esp_periph_handle_t touch_periph = periph_touch_init(&touch_cfg);

    ESP_LOGI(TAG, "[1.4] Start touch peripheral");
        esp_periph_start(touch_periph);

    // OK
    ESP_LOGI(TAG, "[ 2 ] Start codec chip");
        audio_hal_codec_config_t audio_hal_codec_cfg = AUDIO_HAL_ES8388_DEFAULT();
        audio_hal_codec_cfg.i2s_iface.samples = AUDIO_HAL_44K_SAMPLES;
        audio_hal_handle_t hal = audio_hal_init(&audio_hal_codec_cfg, 0);
        audio_hal_ctrl_codec(hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);
        int player_volume;
        audio_hal_get_volume(hal, &player_volume);
    // OK
    ESP_LOGI(TAG, "[3.0] Create audio pipeline for playback");
        audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
        pipeline = audio_pipeline_init(&pipeline_cfg);
        mem_assert(pipeline);

    // FIXME -> w C++ -> a value of type "int" cannot be used to initialize an entity of type "i2s_mode_t"
    ESP_LOGI(TAG, "[3.1] Create i2s stream to write data to codec chip");
        // FIXME artcode
        // typedef enum {     I2S_MODE_MASTER_TX_RX = 13,
        // .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        // .i2s_port = I2S_NUM_0,

        // TODO - uproscic
        //mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX,
        // .bits_per_sample = 16,      
        // .i2s_port = 0,  


            i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();

        // i2s_stream_cfg_t i2s_cfg = {};
        //     i2s_cfg.type = AUDIO_STREAM_WRITER;
        //     i2s_cfg.task_prio = I2S_STREAM_TASK_PRIO;
        //     i2s_cfg.task_core = I2S_STREAM_TASK_CORE;
        //     i2s_cfg.task_stack = I2S_STREAM_TASK_STACK;
        //     i2s_cfg.out_rb_size = I2S_STREAM_RINGBUFFER_SIZE;
        //     // FIXME
        //     // .i2s_config = {
        //     // .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX // powoduje blad w C++
        //     // i2s_cfg.i2s_config.mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX;
        //     // OK FIXME artcode -> dodana deklaracja w i2s_mode_t -> I2S_MODE_MASTER_TX_RX jako suma trybow I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX
        //     i2s_cfg.i2s_config.mode = I2S_MODE_MASTER_TX_RX;
        //     i2s_cfg.i2s_config.sample_rate = 44100;
        //     i2s_cfg.i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
        //     i2s_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
        //     i2s_cfg.i2s_config.communication_format = I2S_COMM_FORMAT_I2S;
        //     i2s_cfg.i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL2;
        //     i2s_cfg.i2s_config.dma_buf_count = 3;
        //     i2s_cfg.i2s_config.dma_buf_len = 300;
        //     i2s_cfg.i2s_config.use_apll = 1;
        //     i2s_cfg.i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL2;

        //     i2s_cfg.i2s_pin_config.bck_io_num = IIS_SCLK;
        //     i2s_cfg.i2s_pin_config.ws_io_num = IIS_LCLK;
        //     i2s_cfg.i2s_pin_config.data_out_num = IIS_DSIN;
        //     i2s_cfg.i2s_pin_config.data_in_num = IIS_DOUT;
        //     i2s_cfg.i2s_port = I2S_NUM_0;

    //////////////////////////////////////////////
        i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(TAG, "[3.2] Create mp3 decoder to decode mp3 file");
        mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
        mp3_decoder = mp3_decoder_init(&mp3_cfg);
        // FIXME --> my_sdcard_read_cb -> stream_func
        // FIXME ->   audio_element_set_read_cb(mp3_decoder, my_sdcard_read_cb, NULL);
        //   audio_element_set_read_cb(mp3_decoder, my_sdcard_read_cb, NULL); // ESP_OK
        // TEST OK w audio_element.h ->> // typedef audio_element_err_t (*stream_func)(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context);
        // zmieniona na : /// typedef int (*stream_func)(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context);
        audio_element_set_read_cb(mp3_decoder, my_sdcard_read_cb, NULL); // ESP_OK
        // <-- FIXME

    ESP_LOGI(TAG, "[3.3] Register all elements to audio pipeline");
        audio_pipeline_register(pipeline, mp3_decoder, "mp3");
        audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");

    ESP_LOGI(TAG, "[3.4] Link it together [my_sdcard_read_cb]-->mp3_decoder-->i2s_stream-->[codec_chip]");
        // FIXME - > C++ taking address of temporary array
        //   audio_pipeline_link(pipeline, (const char *[]) {"mp3", "i2s"}, 2);
        // FIX OK ->
        const char *array_cpp_test[] = {"mp3", "i2s"};
        audio_pipeline_link(pipeline, array_cpp_test, 2); // ok

    ESP_LOGI(TAG, "[4.0] Setup event listener");
        audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
        audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[4.1] Listen for all pipeline events");
        audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGI(TAG, "[4.2] Listening event from peripherals");
        audio_event_iface_set_listener(esp_periph_get_event_iface(), evt);

    ESP_LOGW(TAG, "[ 5 ] Tap touch buttons to control music player:");
    ESP_LOGW(TAG, "      [Play] to start, pause and resume, [Set] next song.");
    ESP_LOGW(TAG, "      [Vol-] or [Vol+] to adjust volume.");

          // TEST OK -> AUTO PLAY mp3
            audio_pipeline_run(pipeline);

    while (1)
    {
        // Handle event interface messages from pipeline to set music info and to advance to the next song
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
   
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT)
        {
            // Set music info for a new song to be played
            if (msg.source == (void *)mp3_decoder && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
            {
                audio_element_info_t music_info = {0};
                audio_element_getinfo(mp3_decoder, &music_info);
                ESP_LOGI(TAG, "[ * ] Received music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                         music_info.sample_rates, music_info.bits, music_info.channels);
                audio_element_setinfo(i2s_stream_writer, &music_info);
                i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
                continue;
            }

            
            // Advance to the next song when previous finishes
            if (msg.source == (void *)i2s_stream_writer && msg.cmd == AEL_MSG_CMD_REPORT_STATUS)
            {
                audio_element_state_t el_state = audio_element_get_state(i2s_stream_writer);
                if (el_state == AEL_STATE_FINISHED)
                {
                    ESP_LOGI(TAG, "[ * ] Finished, advancing to the next song");
                    audio_pipeline_stop(pipeline);
                    audio_pipeline_wait_for_stop(pipeline);
                    get_file(NEXT);
                    audio_pipeline_run(pipeline);
                }
                continue;
            }
        }

        // Handle touch pad events to start, pause, resume, finish current song and adjust volume
        if (msg.source_type == PERIPH_ID_TOUCH && msg.cmd == PERIPH_TOUCH_TAP && msg.source == (void *)touch_periph)
        {
                 
            if ((int)msg.data == TOUCH_PLAY)
            {
                ESP_LOGI(TAG, "[ * ] [Play] touch tap event");
                audio_element_state_t el_state = audio_element_get_state(i2s_stream_writer);
                switch (el_state)
                {
                case AEL_STATE_INIT:
                    ESP_LOGI(TAG, "[ * ] Starting audio pipeline");

                    // FIXME z #include "SD_MMC.h"
                    audio_pipeline_run(pipeline);
                    break;

                case AEL_STATE_RUNNING:
                    ESP_LOGI(TAG, "[ * ] Pausing audio pipeline");
                    audio_pipeline_pause(pipeline);
                    break;
                case AEL_STATE_PAUSED:
                    ESP_LOGI(TAG, "[ * ] Resuming audio pipeline");
                    audio_pipeline_resume(pipeline);
                    break;
                default:
                    ESP_LOGI(TAG, "[ * ] Not supported state %d", el_state);
                }
            }
            else if ((int)msg.data == TOUCH_SET)
            {
                ESP_LOGI(TAG, "[ * ] [Set] touch tap event");
                audio_pipeline_terminate(pipeline);
                ESP_LOGI(TAG, "[ * ] Stopped, advancing to the next song");
                get_file(NEXT);
                audio_pipeline_run(pipeline);
            }
            else if ((int)msg.data == TOUCH_VOLUP)
            {
                ESP_LOGI(TAG, "[ * ] [Vol+] touch tap event");
                player_volume += 10;
                if (player_volume > 100)
                {
                    player_volume = 100;
                }
                audio_hal_set_volume(hal, player_volume);
                ESP_LOGI(TAG, "[ * ] Volume set to %d %%", player_volume);
            }
            else if ((int)msg.data == TOUCH_VOLDWN)
            {
                ESP_LOGI(TAG, "[ * ] [Vol-] touch tap event");
                player_volume -= 10;
                if (player_volume < 0)
                {
                    player_volume = 0;
                }
                audio_hal_set_volume(hal, player_volume);
                ESP_LOGI(TAG, "[ * ] Volume set to %d %%", player_volume);
            }
        }
    }

    ESP_LOGI(TAG, "[ 6 ] Stop audio_pipeline");
    audio_pipeline_terminate(pipeline);

    // Terminate the pipeline before removing the listener
    audio_pipeline_remove_listener(pipeline);

    // Stop all peripherals before removing the listener
    esp_periph_stop_all();
    audio_event_iface_remove_listener(esp_periph_get_event_iface(), evt);

    // Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface
    audio_event_iface_destroy(evt);

    // Release all resources
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(i2s_stream_writer);
    audio_element_deinit(mp3_decoder);
    esp_periph_destroy();

} // main loop