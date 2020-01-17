#include <RF24Network.h>
#include <NRF24L01Library.h>
#include <Arduino.h>

#define SPI_CE  33
#define SPI_CS  26

#define NRF_ADDRESS_SERVER    0
#define NRF_ADDRESS_CLIENT    1

RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);

NRF24L01Lib nrf24;

uint16_t controller_id;

bool nrf_setup()
{
  nrf24.begin(&radio, &network, NRF_ADDRESS_SERVER, controller_packet_available_cb);
  return true;
}

void nrf_update()
{
  nrf24.update();
}  

void nrf_read(uint8_t *data, uint8_t data_len)
{
  nrf24.read_into(data, data_len);
}

uint8_t send_with_retries(uint16_t to, uint8_t *data, uint8_t data_len, uint8_t num_retries)
{
  uint8_t success, retries = 0;
  do
  {
    success = nrf24.send_packet(to, /*type*/ 0, data, data_len);
    if (success == false)
    {
      vTaskDelay(1);
    }
  } while (!success && retries++ < num_retries);

  return retries;
}

#define NUM_SEND_RETRIES   10

bool nrf_send_to_controller()
{
  uint8_t bs[sizeof(VescData)];
  memcpy(bs, &board_packet, sizeof(VescData));

  uint8_t retries = send_with_retries(controller_id, bs, sizeof(VescData), NUM_SEND_RETRIES);

  bool success = retries < NUM_SEND_RETRIES;

#ifdef LOG_RETRIES

  retry_logger.log(retries);

  if (retries > 0 || board_packet.id & 20 == 0)
  {
    float retry_rate = retry_logger.get_retry_rate();
    DEBUGVAL(retry_logger.get_sum_retries(), retry_rate, success, board_packet.id);
  }
#endif

  return success;
}

