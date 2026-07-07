// #include "cloud_sync.h"
// #include "ss_rtc.h"
// #include "esp8266.h"
// #include "sensor_data.h"
// #include "system_config.h"

// uint16_t Suitable_temp = 25.8;      // 适宜温度
// uint16_t Suitable_humi = 35;      // 适宜湿度
// uint16_t Suitable_light = 100.2;     // 适宜光照

// void upload_sensor_data(void)
// {
//     char json[512];
//     sprintf(json,
//         "{"
//         "\"temp\":%.1f,"
//         "\"humid\":%.1f,"
//         "\"light\":%.1f,"
//         "\"mode\":%d,"
//         "\"fan\":%d,"
//         "\"curtain\":%d"
//         "}",
//         g_sensor_data.temperature,
//         g_sensor_data.humidity,
//         g_sensor_data.light,
//         0,
//         0,
//         0
//         // g_config.system_mode,
//         // g_config.fan_level,
//         // g_config.curtain_state
//     );
    
//     printf("JSON: %s\r\n", json);  // 打印看看格式对不对
    
//     char url[80];
//     sprintf(url, "http://%s:%d/api/hardware/init", SERVER_IP, SERVER_PORT);
//     ESP8266_HttpPost(url, json);
// }
