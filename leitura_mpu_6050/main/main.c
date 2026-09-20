#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#define I2C_SDA_GPIO 8
#define I2C_SCL_GPIO 9
#define I2C_PORT I2C_NUM_0

#define MPU6050_ADDR 0x68

#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B

static const char *TAG = "MPU6050";

static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t mpu_handle;

static esp_err_t mpu6050_write_register(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};

    return i2c_master_transmit(
        mpu_handle,
        data,
        sizeof(data),
        -1
    );
}

static esp_err_t mpu6050_read_registers(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(
        mpu_handle,
        &reg,
        1,
        data,
        len,
        -1
    );
}

static void i2c_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(
        i2c_new_master_bus(
            &bus_config,
            &bus_handle
        )
    );

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_ADDR,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(
            bus_handle,
            &dev_config,
            &mpu_handle
        )
    );
}

static void mpu6050_init(void)
{
    // O MPU6050 inicia em modo sleep.
    // Escrever 0 no registrador PWR_MGMT_1 acorda o sensor.
    ESP_ERROR_CHECK(
        mpu6050_write_register(
            MPU6050_REG_PWR_MGMT_1,
            0x00
        )
    );

    ESP_LOGI(TAG, "MPU6050 inicializado com sucesso");
}

void app_main(void)
{
    // i2c_init();
    // mpu6050_init();

    printf("\n\n*** APP_MAIN INICIOU ***\n");
    fflush(stdout);

    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("*** INICIANDO I2C ***\n");
    fflush(stdout);

    i2c_init();

    printf("*** I2C INICIALIZADO ***\n");
    fflush(stdout);

    mpu6050_init();

    uint8_t data[14];

    while (1) {

        esp_err_t ret = mpu6050_read_registers(
            MPU6050_REG_ACCEL_XOUT_H,
            data,
            sizeof(data)
        );

        if (ret == ESP_OK) {

            int16_t accel_x = (data[0] << 8) | data[1];
            int16_t accel_y = (data[2] << 8) | data[3];
            int16_t accel_z = (data[4] << 8) | data[5];

            int16_t temp_raw = (data[6] << 8) | data[7];

            int16_t gyro_x = (data[8] << 8) | data[9];
            int16_t gyro_y = (data[10] << 8) | data[11];
            int16_t gyro_z = (data[12] << 8) | data[13];

            float ax = accel_x / 16384.0f;
            float ay = accel_y / 16384.0f;
            float az = accel_z / 16384.0f;

            float gx = gyro_x / 131.0f;
            float gy = gyro_y / 131.0f;
            float gz = gyro_z / 131.0f;

            float temperature =
                (temp_raw / 340.0f) + 36.53f;

            printf("\n==============================================\n");

            // printf("Acelerometro\n");
            // printf("X: %.2f g\n", ax);
            // printf("Y: %.2f g\n", ay);
            // printf("Z: %.2f g\n", az);

            // printf("\nGiroscopio\n");
            // printf("X: %.2f graus/s\n", gx);
            // printf("Y: %.2f graus/s\n", gy);
            //printf("Z: %.2f graus/s\n", gz);

            // printf("\nTemperatura: %.2f C\n", temperature);

            printf("\n");
            printf("%-15s %-12s %-12s %-12s\n",
                "Sensor", "X", "Y", "Z");

            printf("----------------------------------------------\n");

            printf("%-15s %-12.2f %-12.2f %-12.2f\n",
                "Acelerometro", ax, ay, az);

            printf("%-15s %-12.2f %-12.2f %-12.2f\n",
                "Giroscopio", gx, gy, gz);

            printf("%-15s %.2f ºC\n",
                "Temperatura", temperature);

            // printf("==================================\n");

        } else {

            ESP_LOGE(
                TAG,
                "Erro ao ler dados do MPU6050"
            );
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
