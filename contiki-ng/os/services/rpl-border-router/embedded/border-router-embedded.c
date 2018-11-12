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
/**
 * \file
 *         border-router
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Nicolas Tsiftes <nvt@sics.se>
 */

#include "contiki.h"
#include "net/routing/routing.h"
#if PLATFORM_SUPPORTS_BUTTON_HAL
#include "dev/button-hal.h"
#else
#include "dev/button-sensor.h"
#endif
#include "dev/slip.h"
#include "rpl-border-router.h"

#include "net/routing/rpl-lite/rpl.h"
#include "net/routing/rpl-lite/rpl-neighbor.h"
#include "lib/list.h"
#include "net/link-stats.h"
#include "net/linkaddr.h"
#include "net/packetbuf.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-ds6-nbr.h"
#include "net/ipv6/uip-nd6.h"
#include "net/routing/routing.h"
#include "net/ipv6/uiplib.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-sr.h"


/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "BR"
#define LOG_LEVEL LOG_LEVEL_INFO

#define DEBUG DEBUG_PRINT
#include "net/ipv6/uip-debug.h"

void request_prefix(void);

/*---------------------------------------------------------------------------*/
/*
static void
print_rpl_neighbors(void)
{
	static uip_ds6_nbr_t *nbr;
	LOG_INFO("RPL Neighbors:\n");
	for(nbr = nbr_table_head(rpl_neighbors); nbr != NULL;nbr = nbr_table_next(rpl_neighbors, nbr)) {
		LOG_INFO("\n");
    uip_debug_ipaddr_print(&nbr->ipaddr);
		LOG_INFO("\n");
	}
	LOG_INFO("\n");
}
*/
/*---------------------------------------------------------------------------*/
/*
static void
print_neighbors(void)
{
	static uip_ds6_nbr_t *nbr;
	LOG_INFO("Neighbors:\n");
	for(nbr = nbr_table_head(ds6_neighbors); nbr != NULL;nbr = nbr_table_next(ds6_neighbors, nbr)) {
		LOG_INFO("\n");
    uip_debug_ipaddr_print(&nbr->ipaddr);
		LOG_INFO("\n");
	}
	LOG_INFO("\n");
}
*/
/*---------------------------------------------------------------------------*/
static void
print_routing_link(void)
{
    static uip_sr_node_t *link;
    LOG_INFO("Routing Links:\n");
    for(link = uip_sr_node_head(); link != NULL; link = uip_sr_node_next(link)) {
      if(link->parent != NULL) {
        LOG_INFO("\n");
        uip_ipaddr_t child_ipaddr;
        uip_ipaddr_t parent_ipaddr;
        NETSTACK_ROUTING.get_sr_node_ipaddr(&child_ipaddr, link);
        NETSTACK_ROUTING.get_sr_node_ipaddr(&parent_ipaddr, link->parent);
        uip_debug_ipaddr_print(&child_ipaddr);
        LOG_INFO(" Parent -> ");
        uip_debug_ipaddr_print(&parent_ipaddr);
        LOG_INFO(") %us", (unsigned int)link->lifetime);
        LOG_INFO("\n");        
      }
    }
    LOG_INFO("\n");
}
/*---------------------------------------------------------------------------*/
PROCESS(border_router_process, "Border router process");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(border_router_process, ev, data)
{
  static struct etimer et;
  static struct etimer et1;
  
  PROCESS_BEGIN();

/* While waiting for the prefix to be sent through the SLIP connection, the future
 * border router can join an existing DAG as a parent or child, or acquire a default
 * router that will later take precedence over the SLIP fallback interface.
 * Prevent that by turning the radio off until we are initialized as a DAG root.
 */
  prefix_set = 0;
  NETSTACK_MAC.off();

  PROCESS_PAUSE();

#if !PLATFORM_SUPPORTS_BUTTON_HAL
  SENSORS_ACTIVATE(button_sensor);
#endif

  LOG_INFO("RPL-Border router started\n");

  /* Request prefix until it has been received */
  while(!prefix_set) {
    etimer_set(&et, CLOCK_SECOND);
    request_prefix();
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    LOG_INFO("Waiting for prefix\n");
	LOG_INFO(" ------------------------------------ \n");
  }

  NETSTACK_MAC.on();
  
  LOG_INFO("Printing Local Addresses ----- \n");
  print_local_addresses();

  while(1) {
		etimer_set(&et1, 500);
    LOG_INFO(" ------------------------------------ \n");
    rpl_neighbor_print_list("RPL");
    LOG_INFO(" ------------------------------------ \n");
    //print_neighbors();
    print_routing_link();
    LOG_INFO("\n");
    LOG_INFO(" ------------------------------------ \n");
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et1));
	}

  while(1) {
    PROCESS_YIELD();
#if PLATFORM_SUPPORTS_BUTTON_HAL
    if(ev == button_hal_release_event) {
#else
    if(ev == sensors_event && data == &button_sensor) {
#endif
      LOG_INFO("Initiating global repair\n");
      NETSTACK_ROUTING.global_repair("Button press");
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
