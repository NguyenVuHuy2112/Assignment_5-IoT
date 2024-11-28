#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h" // Random library
#include "dev/button-sensor.h"
#include <stdio.h>
#include <string.h>

#define ROOT_ID 1
#define UNICAST_CHANNEL 140
#define BROADCAST_CHANNEL 128
#define DATA_INTERVAL 15
#define BEACON_INTERVAL 8
#define MAX_NODES 15
#define PARENT_STRATEGY 2 // 1: HOP, 2: RSSI, 3: PRR

typedef struct {
  uint8_t id;
  int8_t rssi;
  uint8_t prr;
  uint16_t hop;
} neighbor_t;

static neighbor_t neighbors[MAX_NODES];
static uint8_t neighbor_count = 0;
static uint8_t parent_id = 0;
static uint16_t parent_hop = 0xFFFF; // Large value initially

static struct broadcast_conn broadcast;
static struct unicast_conn unicast;
static struct etimer beacon_timer, data_timer;

PROCESS(tree_routing_process, "Tree Routing Process");
PROCESS(data_sending_process, "Data Sending Process");
AUTOSTART_PROCESSES(&tree_routing_process, &data_sending_process);

static void swap(neighbor_t *a, neighbor_t *b) {
  neighbor_t temp = *a;
  *a = *b;
  *b = temp;
}

static void bubble_sort(neighbor_t arr[], int n) {
  for (int i = 0; i < n - 1; i++) {
    for (int j = 0; j < n - i - 1; j++) {
      if (arr[j].hop > arr[j + 1].hop) {
        swap(&arr[j], &arr[j + 1]);
      }
    }
  }
}

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {
  uint8_t id = from->u8[0];
  int8_t rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
  uint16_t hop = *(uint16_t *)packetbuf_dataptr();

  for (int i = 0; i < neighbor_count; i++) {
    if (neighbors[i].id == id) {
      neighbors[i].rssi = rssi;
      neighbors[i].hop = hop;
      neighbors[i].prr++;
      return;
    }
  }

  if (neighbor_count < MAX_NODES) {
    neighbors[neighbor_count].id = id;
    neighbors[neighbor_count].rssi = rssi;
    neighbors[neighbor_count].hop = hop;
    neighbors[neighbor_count].prr = 1;
    neighbor_count++;
  }

  if (linkaddr_node_addr.u8[0] != ROOT_ID) {
    uint16_t new_hop = hop + 1;
    packetbuf_copyfrom(&new_hop, sizeof(new_hop));
    broadcast_send(&broadcast);
    printf("Node %d forwarding broadcast with hop %d\n", linkaddr_node_addr.u8[0], new_hop);
  }
}

static void unicast_recv(struct unicast_conn *c, const linkaddr_t *from) {
  printf("Received data from node %d\n", from->u8[0]);
}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static const struct unicast_callbacks unicast_call = {unicast_recv};

static void update_parent() {
  if (linkaddr_node_addr.u8[0] == ROOT_ID) {
    parent_id = 0; // Root node does not have a parent
    parent_hop = 0;
    printf("Node %d: Root node does not select a parent\n", linkaddr_node_addr.u8[0]);
    return;
  }

  parent_id = 0;
  parent_hop = 0xFFFF; // Reset parent hop to a large value

  for (int i = 0; i < neighbor_count; i++) {
    if (neighbors[i].hop < parent_hop || (neighbors[i].hop == parent_hop && neighbors[i].rssi > neighbors[parent_id].rssi)) {
      parent_hop = neighbors[i].hop;
      parent_id = neighbors[i].id;
    }
  }

  if (parent_id != 0) {
    printf("Node %d: Selected parent is Node %d with hop %d\n", linkaddr_node_addr.u8[0], parent_id, parent_hop);
  } else {
    printf("Node %d: No parent available\n", linkaddr_node_addr.u8[0]);
  }
}

PROCESS_THREAD(tree_routing_process, ev, data) {
  PROCESS_BEGIN();

  broadcast_open(&broadcast, BROADCAST_CHANNEL, &broadcast_call);
  unicast_open(&unicast, UNICAST_CHANNEL, &unicast_call);

  etimer_set(&beacon_timer, CLOCK_SECOND * BEACON_INTERVAL);
  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&beacon_timer));

    if (linkaddr_node_addr.u8[0] == ROOT_ID) {
      uint16_t hop = 1;
      packetbuf_copyfrom(&hop, sizeof(hop));
      broadcast_send(&broadcast);
      printf("Root node broadcasting hop 1\n");
    }

    update_parent();

    printf("Top 5 neighbors of Node %d:\n", linkaddr_node_addr.u8[0]);
    for (int i = 0; i < neighbor_count && i < 5; i++) {
      printf("Neighbor %d: RSSI=%d, PRR=%d, HOPS=%d\n",
             neighbors[i].id, neighbors[i].rssi, neighbors[i].prr, neighbors[i].hop);
    }

    etimer_reset(&beacon_timer);
  }

  PROCESS_END();
}

PROCESS_THREAD(data_sending_process, ev, data) {
  static uint8_t data_message = 0;
  PROCESS_BEGIN();

  etimer_set(&data_timer, CLOCK_SECOND * DATA_INTERVAL);
  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&data_timer));

    if (parent_id != 0 && linkaddr_node_addr.u8[0] != ROOT_ID) {
      packetbuf_copyfrom(&data_message, sizeof(data_message));
      linkaddr_t parent_addr = {.u8 = {parent_id, 0}};
      unicast_send(&unicast, &parent_addr);
      printf("Node %d sent data %d to parent Node %d\n", linkaddr_node_addr.u8[0], data_message, parent_id);
      data_message++;
    }

    etimer_reset(&data_timer);
  }

  PROCESS_END();
}
