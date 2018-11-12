/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip-ds6.h"
#include "dev/dht22.h"
#include "dev/leds.h"
#include "dev/adc-sensors.h"
//logs para visualização na consola
#include "sys/log.h"
#define LOG_MODULE "UDP"
#define LOG_LEVEL LOG_LEVEL_INFO
//PIN para leitura analógica do sensor de luz
#define ADC_PIN 5
//ports para comunicação com o servidor NodeJs
#define UDP_CLIENT_PORT	8000
#define UDP_SERVER_PORT	10000
//intervalo de leitura
#define READ_INTERVAL (60 * CLOCK_SECOND)
//estrutura para o timer
static struct etimer et;
//estrutura para a conexão udp
static struct simple_udp_connection udp_conn;
//endereço da interface
uip_ipaddr_t server_ipaddr;
//mensagem vinda do servidor NodeJs
static char *message;
//inicio do processo
PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);
//callback usado para a escuta de mensagens vindas do servidor NodeJs
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  //estrair mensagem
  message = (char *)data;
  LOG_INFO("Received request %s from ", message);
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");
  char ledson[20];
  sprintf(ledson,"led:1");
  char ledsoff[20];
  sprintf(ledsoff,"led:0");
  //comparar mensagem e ativar/desativar o atuador LED
  if(strcmp(message,ledson) == 0){
    LOG_INFO("Leds ON\n");
    leds_toggle(LEDS_RED);
  }else if(strcmp(message,ledsoff) == 0){
    LOG_INFO("Leds OFF\n");
    leds_off(LEDS_ALL);
  }else{
    LOG_INFO("Invalid message\n");
  }
}
//funcao para criconstruir  o endereço fd00::1 
/*---------------------------------------------------------------------------*/
static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;
  uip_ip6addr(&ipaddr, 0xfd00, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
  uip_ip6addr(&server_ipaddr, 0xfd00, 0, 0, 0, 0, 0, 0, 1);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
  //variáveis para a recolha dos valores dos sensores
  int16_t temperature = 0, humidity = 0, light = 0;
  //variável que contém mensagem a enviar para o servidor NodeJs com os dados dos sensores
  char buffer[40];
  leds_off(LEDS_ALL);
  PROCESS_BEGIN();
  //inicializar a conexão UDP
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL, UDP_CLIENT_PORT, udp_rx_callback);
  //Inicializar o endereço do servidor NodeJs - fd00::1
  set_global_address();
  PROCESS_PAUSE();
  //Configurar o PIN 5 para o sensor analógico GROVE LIGHT
  adc_sensors.configure(ANALOG_GROVE_LIGHT, 5);
  //Activar o sensor DHT22 (temperatura e humidade)
  SENSORS_ACTIVATE(dht22);
  while(1) {
    //definir o tempo do timer
    etimer_set(&et, READ_INTERVAL);
    //leitura do sensor DHT22
    dht22_read_all(&temperature, &humidity);
    //leitura do sensor GROVE LIGHT
    light = adc_sensors.value(ANALOG_GROVE_LIGHT);
    //se o endereço do servidor for alcançável, enviar os dados
    if(NETSTACK_ROUTING.node_is_reachable()) {
      sprintf(buffer,"temp:%02d.%02d,humi:%02d.%02d,light:%d,", temperature / 10, temperature % 10, humidity / 10, humidity % 10, light);
      LOG_INFO("Sending -- temp:%02d.%02d,humi:%02d.%02d,light:%d -- to -- ", temperature / 10, temperature % 10, humidity / 10, humidity % 10, light);
      LOG_INFO_6ADDR(&server_ipaddr);
      LOG_INFO_("\n");
      //envio dos dados dos sensores para o servidor NodeJs
      simple_udp_sendto(&udp_conn, &buffer, sizeof(buffer), &server_ipaddr);
    } else {
      LOG_INFO("Server not reachable \n");
    }
  LOG_INFO(" ------------------------------------------------------ \n");
  //aguardar o timer
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
