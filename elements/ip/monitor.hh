#ifndef MONITOR_HH
#define MONITOR_HH

/*
 * =c
 * Monitor(THRESH, [, SD1 VAR1 [, SD2 VAR2 [, ... [, SDn VARn]]]])
 * =d
 * Input: IP packets (no ether header).
 *
 * THRESH is "amount per second" (see explanation below). Integer.
 * SDx is a string ("SRC" or "DST").
 * VARx is an integer.
 *
 * Monitors traffic by counting the number of packets going to/coming from
 * (a cluster of) IP addresses.
 *
 * In its simplest form (i.e. "Monitor(DST, 1)"), Monitor uses the first byte of
 * the destination IP address of each packet to index into a table (with, of
 * course, 256 records) and increases the value in that record by 1. In other
 * words, the Monitor clusters destination addresses by the their first byte. As
 * soon as the value associated with such a cluster increases with more than
 * THRESH per second, then the entry is marked and subsequent packets to that
 * cluster are split on the 2nd byte of the destination IP address in a similar
 * table. This can go up to the 4th byte.
 *
 * Everytime a packet passes the monitor, the sibling annotation is set denoting
 * the number of packets from the same cluster that preceded this packet. Block
 * drops packets based on the sibling annotation.
 *
 * THRESH is a value denoting an amount of packets per second. If the value
 * associated with a cluster of IP addresses increases with more than THRESH per
 * second, it is split.
 *
 * The number of inputs for Monitor equals n in VARn. Each SDx and VARx are
 * related to one input (i.e. x). The "SRC" or "DST" tells the Monitor to use
 * either the source or the destination IP address to index into the described
 * table(s). VARx is the amount by which the value associated with a cluster
 * should be increased or decreased.
 *
 * Monitor should be used together with Classifier to count packets with
 * specific features.
 *
 * =h look (read)
 * Returns the number of packets counted to/from a cluster of IP addresses.
 * 
 * =h thresh (read-write)
 * Used to read/write THRESH value.
 *
 * =h reset (write)
 * Resets all entries to the supplied value.
 *
 * =h since (read)
 * Time of last reset.
 *
 * =e
 *
 * For example,
 *
 * = c :: Classifier(SYN, SYN-ACK);
 * =
 * = ... -> c;
 *
 * = m :: Monitor(10, DST 1, DST -1);
 * =
 * = c[0] -> [0]m -> ...
 * = c[1] -> [1]m -> ...
 *
 * makes m count packets based on the destination IP address. For every
 * SYN-packet, the value is raised by 1, for every SYN-ACK packet the value is
 * lowered by 1.
 *
 * (Classifier has been simplified in this example)
 *
 * =a Classifier
 * =a Block
 */
#include "glue.hh"
#include "click_ip.h"
#include "element.hh"
#include "monitor.hh"
#include "vector.hh"


class Monitor : public Element {
public:
  Monitor();
  ~Monitor();
  
  const char *class_name() const		{ return "Monitor"; }
  const char * default_processing() const	{ return AGNOSTIC; }
  int configure(const String &conf, ErrorHandler *errh);

  // XXX: Do we want this?
  // bool can_live_reconfigure() const		{ return true; }
  Monitor *clone() const;
  
  void push(int port, Packet *p);

private:

// XXX: Is this somewhere defined already?
#if IPVERSION == 4
#define BYTES 4
#endif

#define SRC 0
#define DST 1

  // One of these associated with each input.
  struct _inp {
    int change;
    unsigned char srcdst;
  };

  struct _stats;

  // For each (cluster of) IP address(es).
  struct _counter {
    unsigned char flags;
#define SPLIT     0x0001

    union {
      int value;
      struct _stats *next_level;
    };

    int last_update;
  };

  struct _stats {
    int base;                           // value of previous level when split
    struct _counter counter[256];
  };

  int _thresh;
  Vector<struct _inp *> _inputs;        // value associated with each input
  struct _stats *_base;                 // base struct for monitoring
  long unsigned int _since;             // time of last reset

  void clean(_stats *s, int value = 0, bool recurse = false);
  void set_since();
  int update(IPAddress a, int val);
  String print(_stats *s, String ip = "");

  void add_handlers();
  static String thresh_read_handler(Element *e, void *);
  static String look_read_handler(Element *e, void *);
  static String since_read_handler(Element *e, void *);

  static int thresh_write_handler(const String &conf, Element *e, void *, ErrorHandler *errh);
  static int reset_write_handler(const String &conf, Element *e, void *, ErrorHandler *errh);
};

#endif /* MONITOR_HH */
