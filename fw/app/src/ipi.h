#ifndef __IPI_H__
#define __IPI_H__

#include "ioxp_emu2pc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize and start IPI service
 *
 * @return int
 */
int ipi_init(void);

/**
 * @brief enqueue a CAM Message for transmission
 *
 * @param msg CAN Message to be transmitted
 * @return int
 */
int ipi_send_can_msg(const rvc_msg_t* msg);

#ifdef __cplusplus
}
#endif

#endif
