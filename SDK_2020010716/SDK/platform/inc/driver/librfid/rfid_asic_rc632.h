#ifndef _RFID_ASIC_RC632_H
#define _RFID_ASIC_RC632_H

struct rfid_asic_transport_handle;

#include <librfid/rfid.h>
#include <librfid/rfid_asic.h>
#include <librfid/rfid_layer2.h>

struct rfid_asic_rc632_transport {
	struct {
		int (*reg_write)(struct rfid_asic_transport_handle *rath,
				 u_int8_t reg,
				 u_int8_t value);
		int (*reg_read)(struct rfid_asic_transport_handle *rath,
				u_int8_t reg,
				u_int8_t *value);
		int (*fifo_write)(struct rfid_asic_transport_handle *rath,
				  u_int8_t len,
				  const u_int8_t *buf,
				  u_int8_t flags);
		int (*fifo_read)(struct rfid_asic_transport_handle *rath,
				 u_int8_t len,
				 u_int8_t *buf);
	} fn;
};

struct rfid_asic_handle;

struct iso14443a_atqa;
struct iso14443a_anticol_cmd;
struct iso15693_anticol_cmd;

struct rfid_asic_rc632 {
	struct {
		int (*power)(struct rfid_asic_handle *h, int on);
		int (*rf_power)(struct rfid_asic_handle *h, int on);
		int (*init)(struct rfid_asic_handle *h, enum rfid_layer2_id);
		int (*transceive)(struct rfid_asic_handle *h,
				  enum rfid_frametype,
				  const u_int8_t *tx_buf,
				  unsigned int tx_len,
				  u_int8_t *rx_buf,
				  unsigned int *rx_len,
				  u_int64_t timeout,
				  unsigned int flags);
		struct {
			int (*transceive_sf)(struct rfid_asic_handle *h,
					     u_int8_t cmd,
					     struct iso14443a_atqa *atqa);
			int (*transceive_acf)(struct rfid_asic_handle *h,
					      struct iso14443a_anticol_cmd *cmd,
					      unsigned int *bit_of_col);
			int (*set_speed)(struct rfid_asic_handle *h,
					 unsigned int tx,
					 unsigned int speed);
		} iso14443a;
		struct {
			int (*transceive_ac)(struct rfid_asic_handle *h,
					     const struct iso15693_anticol_cmd *acf,
					     unsigned int acf_len,
					     struct iso15693_anticol_resp *resp,
					     unsigned int *rx_len, char *bit_of_col);
		} iso15693;
		struct {
			int (*setkey)(struct rfid_asic_handle *h,
				      const unsigned char *key);
			int (*auth)(struct rfid_asic_handle *h, u_int8_t cmd, 
				    u_int32_t serno, u_int8_t block);
		} mifare_classic;
	} fn;
};

struct rc632_transport_handle {
  unsigned int dump;
};

/* A handle to a specific RC632 chip */
struct rfid_asic_rc632_handle {
	struct rc632_transport_handle th;
};

struct rfid_asic_rc632_impl_proto {
	u_int8_t mod_conductance;
	u_int8_t cw_conductance;
	u_int8_t bitphase;
	u_int8_t threshold;
};

struct rfid_asic_rc632_impl {
	u_int32_t mru;		/* maximum receive unit (PICC->PCD) */
	u_int32_t mtu;		/* maximum transmit unit (PCD->PICC) */
	//struct rfid_asic_rc632_impl_proto proto[NUM_RFID_PROTOCOLS];
};

extern struct rfid_asic_handle * rc632_open(struct rfid_asic_transport_handle *th);
extern void rc632_close(struct rfid_asic_handle *h);

#endif
